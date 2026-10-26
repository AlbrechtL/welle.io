/*
 *    Copyright (C) 2026
 *    Fridrich Strba (fridrich.strba@bluewin.ch)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "ui_manager.hpp"
#include "utils.hpp"

#include <algorithm>

/* Screens animate (DLS marquee) from Frame::now: the tick only has to be
 * finer than their steps. */
static const auto TICK = std::chrono::milliseconds(50);

/* The volume level stays on the last line for a second after the last key, so
 * that it is there while the key repeats and goes away after */
static const uint64_t VOLUME_MS = 1000;

/* U+2588 FULL BLOCK: the filled part of the volume bar, the rest is empty */
static const char* const BAR_FULL = "\xe2\x96\x88";

std::string volumeLine(int cols, int percent, bool muted)
{
    if (cols <= 0) {
        return std::string();
    }
    if (muted) {
        return utf8Fit("Muted", (size_t)cols);
    }
    percent = std::max(0, std::min(100, percent));
    /* the whole line is the bar; a level above zero keeps its cell */
    size_t full = ((size_t)cols * (size_t)percent + 50) / 100;
    if (percent > 0 && full == 0) {
        full = 1;
    }
    std::string line;
    for (size_t i = 0; i < (size_t)cols; ++i) {
        line += i < full ? BAR_FULL : " ";
    }
    return line;
}

UIManager::UIManager(std::unique_ptr<IDisplay> display)
    : m_display(std::move(display)), m_exit(false), m_screenChanged(false), m_lastInputTime(now_ms())
{
    m_display->backlightOn();
    m_thread = std::thread(&UIManager::run, this);
    m_watchdogThread = std::thread(&UIManager::watchdogRun, this);
}

UIManager::~UIManager() {
    m_exit = true;
    m_cond.notify_one();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_watchdogThread.joinable()) {
        m_watchdogThread.join();
    }
    Frame blank(m_display->cols(), m_display->rows(), now_ms());
    for (auto& line : blank.lines) {
        line = utf8Fit(line, blank.cols);
    }
    m_display->present(blank);
}

void UIManager::setScreen(std::shared_ptr<UIScreen> newScreen) {
    resetToRoot(newScreen);
}

/* The screen hooks run outside the lock: a hook may render, load a file or ask
 * the manager about the stack, and holding the mutex through that stalls the
 * display thread (and deadlocks a hook that calls back in). */
void UIManager::pushScreen(std::shared_ptr<UIScreen> screen) {
    if (!screen) return;
    std::shared_ptr<UIScreen> paused;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        if (!m_historyStack.empty()) {
            paused = m_historyStack.back();
        }
        m_historyStack.push_back(screen);
        m_screenChanged = true;
    }
    if (paused) {
        paused->onPause();
    }
    screen->onPush();
    m_cond.notify_one();
}

bool UIManager::popScreen() {
    std::shared_ptr<UIScreen> popped, resumed;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        if (m_historyStack.size() <= 1) {
            return false;
        }
        popped = m_historyStack.back();
        m_historyStack.pop_back();
        if (!m_historyStack.empty()) {
            resumed = m_historyStack.back();
        }
        m_screenChanged = true;
    }
    popped->onPop();
    if (resumed) {
        resumed->onResume();
    }
    m_cond.notify_one();
    return true;
}

void UIManager::resetToRoot(std::shared_ptr<UIScreen> rootScreen) {
    if (!rootScreen) return;
    std::vector<std::shared_ptr<UIScreen>> popped;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        while (!m_historyStack.empty()) {
            popped.push_back(m_historyStack.back());
            m_historyStack.pop_back();
        }
        m_historyStack.push_back(rootScreen);
        m_screenChanged = true;
    }
    for (const auto& screen : popped) {
        screen->onPop();
    }
    rootScreen->onPush();
    m_cond.notify_one();
}

void UIManager::resetToRoot() {
    std::shared_ptr<UIScreen> root;
    std::vector<std::shared_ptr<UIScreen>> popped;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        if (m_historyStack.size() <= 1) {
            return;
        }
        while (m_historyStack.size() > 1) {
            popped.push_back(m_historyStack.back());
            m_historyStack.pop_back();
        }
        root = m_historyStack.back();
        m_screenChanged = true;
    }
    for (const auto& screen : popped) {
        screen->onPop();
    }
    root->onResume();
    m_cond.notify_one();
}

void UIManager::processInput(InputAction action) {
    /* HOME is the one action the screens do not see: it returns to the root
     * screen (the radio) from wherever the user is. */
    if (action == InputAction::HOME) {
        resetToRoot();
        return;
    }

    std::shared_ptr<UIScreen> screen;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        screen = m_historyStack.empty() ? nullptr : m_historyStack.back();
    }
    if (screen) {
        screen->handleInput(action);
        m_screenChanged = true;
        m_cond.notify_one();
    }
}

void UIManager::showVolume(int percent, bool muted) {
    m_volumePercent = percent;
    m_volumeMuted = muted;
    m_volumeUntil = now_ms() + VOLUME_MS;
    m_screenChanged = true;
    m_cond.notify_one();
}

void UIManager::run() {
    while (!m_exit) {
        std::shared_ptr<UIScreen> screen;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait_for(lock, TICK, [this]() { return m_exit || m_screenChanged; });
            if (m_exit) break;
            screen = m_historyStack.empty() ? nullptr : m_historyStack.back();
            m_screenChanged = false;
        }
        if (screen) {
            Frame frame(m_display->cols(), m_display->rows(), now_ms());
            screen->render(frame);
            /* the volume level takes the last line of whatever is on top */
            if (frame.now < m_volumeUntil) {
                frame.set(frame.rows - 1,
                          volumeLine(frame.cols, m_volumePercent, m_volumeMuted));
            }
            for (auto& line : frame.lines) {
                line = utf8Fit(line, frame.cols);
            }
            m_display->present(frame);
        }
    }
}

void UIManager::watchdogRun() {
    while (!m_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (m_exit) break;

        bool shouldReset = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_historyStack.size() > 1 && (now_ms() - m_lastInputTime > 10000)) {
                shouldReset = true;
            }
        }
        if (shouldReset) {
            resetToRoot();
        }
    }
}

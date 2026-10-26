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

/* Screens animate (DLS marquee) from Frame::now: the tick only has to be
 * finer than their steps. */
static const auto TICK = std::chrono::milliseconds(50);

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

void UIManager::pushScreen(std::shared_ptr<UIScreen> screen) {
    if (!screen) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        if (!m_historyStack.empty()) {
            m_historyStack.back()->onPause();
        }
        m_historyStack.push_back(screen);
        screen->onPush();
        m_screenChanged = true;
    }
    m_cond.notify_one();
}

bool UIManager::popScreen() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        if (m_historyStack.size() <= 1) {
            return false;
        }
        m_historyStack.back()->onPop();
        m_historyStack.pop_back();
        if (!m_historyStack.empty()) {
            m_historyStack.back()->onResume();
        }
        m_screenChanged = true;
    }
    m_cond.notify_one();
    return true;
}

void UIManager::resetToRoot(std::shared_ptr<UIScreen> rootScreen) {
    if (!rootScreen) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        while (!m_historyStack.empty()) {
            m_historyStack.back()->onPop();
            m_historyStack.pop_back();
        }
        m_historyStack.push_back(rootScreen);
        rootScreen->onPush();
        m_screenChanged = true;
    }
    m_cond.notify_one();
}

void UIManager::resetToRoot() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastInputTime = now_ms();
        if (m_historyStack.size() <= 1) {
            return;
        }
        while (m_historyStack.size() > 1) {
            m_historyStack.back()->onPop();
            m_historyStack.pop_back();
        }
        m_historyStack.back()->onResume();
        m_screenChanged = true;
    }
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

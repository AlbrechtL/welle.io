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
#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include "ui_screen.hpp"
#include "display_interface.hpp"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

class UIManager {
    friend struct UIManagerTestHelper;
public:
    explicit UIManager(std::unique_ptr<IDisplay> display);
    ~UIManager();

    void setScreen(std::shared_ptr<UIScreen> newScreen);
    
    // Pushes a new screen onto the stack, pausing the previous top
    void pushScreen(std::shared_ptr<UIScreen> screen);

    // Pops the top screen, resuming the screen below it. Returns false if stack is empty.
    bool popScreen();

    // Clears the history stack and resets back to the root
    void resetToRoot(std::shared_ptr<UIScreen> rootScreen);
    void resetToRoot();

    void processInput(InputAction action);

    // The volume keys: the last line shows the level for a moment (a second
    // after the last key), then the screen has its line back
    void showVolume(int percent, bool muted);

private:
    void run();
    void watchdogRun();

    std::unique_ptr<IDisplay> m_display;
    std::vector<std::shared_ptr<UIScreen>> m_historyStack;
    std::atomic<bool> m_exit;
    std::atomic<bool> m_screenChanged;  // render now, not at the next tick
    std::atomic<uint64_t> m_lastInputTime;
    // The volume keys: the level on the last line until this time
    std::atomic<int> m_volumePercent{-1};
    std::atomic<bool> m_volumeMuted{false};
    std::atomic<uint64_t> m_volumeUntil{0};
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::thread m_thread;
    std::thread m_watchdogThread;
};

/* The line the volume keys put over the last line of the screen: a bar of
 * block characters as wide as the screen, or "Muted"; exactly cols code
 * points. */
std::string volumeLine(int cols, int percent, bool muted);

#endif // UI_MANAGER_HPP

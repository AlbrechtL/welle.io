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
#include "ui_screen.hpp"
#include "display_interface.hpp"
#include "utils.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <chrono>
#include <thread>
#include <mutex>

// Helper to access private fields of UIManager in the test
struct UIManagerTestHelper {
    static void setLastInputTime(UIManager& ui, uint64_t time) {
        ui.m_lastInputTime = time;
    }
    static size_t getStackSize(UIManager& ui) {
        std::lock_guard<std::mutex> lock(ui.m_mutex);
        return ui.m_historyStack.size();
    }
    static std::shared_ptr<UIScreen> getTopScreen(UIManager& ui) {
        std::lock_guard<std::mutex> lock(ui.m_mutex);
        return ui.m_historyStack.empty() ? nullptr : ui.m_historyStack.back();
    }
};

// Mock Display that does nothing but satisfies interface
class MockDisplay : public IDisplay {
public:
    int cols() const override { return 16; }
    int rows() const override { return 2; }
    void present(const Frame&) override {}
};

// Mock Screen that logs lifecycle transitions
class MockScreen : public UIScreen {
public:
    std::string name;
    std::vector<std::string>& log;

    MockScreen(std::string n, std::vector<std::string>& l) : name(std::move(n)), log(l) {}

    void render(Frame&) override {}
    void handleInput(InputAction) override {}

    void onPush() override {
        log.push_back(name + ":onPush");
    }

    void onPop() override {
        log.push_back(name + ":onPop");
    }

    void onPause() override {
        log.push_back(name + ":onPause");
    }

    void onResume() override {
        log.push_back(name + ":onResume");
    }
};

// Display that keeps the last frame, for the volume line
class RecordingDisplay : public IDisplay {
public:
    int cols() const override { return 16; }
    int rows() const override { return 2; }
    void present(const Frame& frame) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lines = frame.lines;
    }
    std::string line(size_t row) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return row < m_lines.size() ? m_lines[row] : std::string();
    }

private:
    std::mutex m_mutex;
    std::vector<std::string> m_lines;
};

// Screen with a program line and a DLS line, like the radio screen
class TextScreen : public UIScreen {
public:
    void render(Frame& frame) override {
        frame.set(0, "radio");
        frame.set(1, "DLS text");
    }
    void handleInput(InputAction) override {}
};

int main() {
    std::vector<std::string> log;

    std::cout << "Starting Navigation Stack Tests..." << std::endl;

    auto display = std::make_unique<MockDisplay>();
    UIManager ui(std::move(display));

    auto root = std::make_shared<MockScreen>("Root", log);
    auto child1 = std::make_shared<MockScreen>("Child1", log);
    auto child2 = std::make_shared<MockScreen>("Child2", log);

    // Test 1: Initial resetToRoot
    std::cout << "Test 1: Initial resetToRoot" << std::endl;
    ui.resetToRoot(root);
    assert(UIManagerTestHelper::getStackSize(ui) == 1);
    assert(UIManagerTestHelper::getTopScreen(ui) == root);
    assert(log.size() == 1);
    assert(log[0] == "Root:onPush");
    log.clear();

    // Test 2: pushScreen
    std::cout << "Test 2: Push Screen" << std::endl;
    ui.pushScreen(child1);
    assert(UIManagerTestHelper::getStackSize(ui) == 2);
    assert(UIManagerTestHelper::getTopScreen(ui) == child1);
    // Previous root should be paused (and therefore interrupted), child1 should be pushed
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Allow rendering loop to run/settle
    assert(log.size() >= 2);
    assert(log[0] == "Root:onPause");
    assert(log[1] == "Child1:onPush");
    log.clear();

    // Test 3: popScreen
    std::cout << "Test 3: Pop Screen" << std::endl;
    bool popped = ui.popScreen();
    (void)popped;
    assert(popped == true);
    assert(UIManagerTestHelper::getStackSize(ui) == 1);
    assert(UIManagerTestHelper::getTopScreen(ui) == root);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(log.size() >= 2);
    assert(log[0] == "Child1:onPop");
    assert(log[1] == "Root:onResume");
    log.clear();

    // Test 4: Cannot pop root screen
    std::cout << "Test 4: Cannot pop root screen" << std::endl;
    popped = ui.popScreen();
    (void)popped;
    assert(popped == false);
    assert(UIManagerTestHelper::getStackSize(ui) == 1);
    assert(UIManagerTestHelper::getTopScreen(ui) == root);
    assert(log.empty());

    // Test 5: push multiple and resetToRoot (no-param)
    std::cout << "Test 5: Push multiple and resetToRoot" << std::endl;
    ui.pushScreen(child1);
    ui.pushScreen(child2);
    assert(UIManagerTestHelper::getStackSize(ui) == 3);
    log.clear();

    ui.resetToRoot();
    assert(UIManagerTestHelper::getStackSize(ui) == 1);
    assert(UIManagerTestHelper::getTopScreen(ui) == root);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // child2 is popped, child1 is popped, root is resumed
    assert(log.size() >= 3);
    assert(log[0] == "Child2:onPop");
    assert(log[1] == "Child1:onPop");
    assert(log[2] == "Root:onResume");
    log.clear();

    // Test 6: resetToRoot to a new root screen
    std::cout << "Test 6: resetToRoot to a new root" << std::endl;
    auto newRoot = std::make_shared<MockScreen>("NewRoot", log);
    ui.resetToRoot(newRoot);
    assert(UIManagerTestHelper::getStackSize(ui) == 1);
    assert(UIManagerTestHelper::getTopScreen(ui) == newRoot);
    // root should be popped, newRoot should be pushed
    assert(log.size() >= 2);
    assert(log[0] == "Root:onPop");
    assert(log[1] == "NewRoot:onPush");
    log.clear();

    // Test 7: Centralized Inactivity Timeout
    std::cout << "Test 7: Centralized Inactivity Timeout" << std::endl;
    ui.pushScreen(child1);
    assert(UIManagerTestHelper::getStackSize(ui) == 2);
    log.clear();

    // Manually advance the inactivity time to >10 seconds
    UIManagerTestHelper::setLastInputTime(ui, now_ms() - 11000);
    
    // Wait for the watchdog thread (runs every 500ms) to trigger and reset back to root
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // Stack size should be back to 1 (newRoot is active)
    assert(UIManagerTestHelper::getStackSize(ui) == 1);
    assert(UIManagerTestHelper::getTopScreen(ui) == newRoot);
    assert(log.size() >= 2);
    assert(log[0] == "Child1:onPop");
    assert(log[1] == "NewRoot:onResume");

    std::cout << "Test 8: Volume level on the last line" << std::endl;
    // the whole line is the bar: U+2588 FULL BLOCK for the filled part,
    // spaces for the rest
    const std::string full = "\xe2\x96\x88";
    auto repeat = [](const std::string& s, size_t n) {
        std::string r;
        for (size_t i = 0; i < n; ++i) r += s;
        return r;
    };
    assert(volumeLine(16, 0, false) == std::string(16, ' '));
    assert(volumeLine(16, 45, false) == repeat(full, 7) + std::string(9, ' '));
    assert(volumeLine(16, 100, false) == repeat(full, 16));
    assert(volumeLine(16, 150, false) == volumeLine(16, 100, false));
    assert(volumeLine(16, -5, false) == volumeLine(16, 0, false));
    // a level above zero keeps a cell, so that the line is not mistaken for
    // a line without the level
    assert(volumeLine(16, 1, false) == full + std::string(15, ' '));
    assert(volumeLine(16, 45, true) == "Muted" + std::string(11, ' '));
    assert(volumeLine(4, 50, false) == repeat(full, 2) + std::string(2, ' '));
    assert(volumeLine(0, 50, false).empty());
    assert(utf8Length(volumeLine(5, 100, false)) == 5);

    // it takes the last line of the screen for a second and then gives it back
    auto recording = std::unique_ptr<RecordingDisplay>(new RecordingDisplay());
    RecordingDisplay* shown = recording.get();
    UIManager volumeUi(std::move(recording));
    volumeUi.setScreen(std::make_shared<TextScreen>());
    auto waitLine = [&](size_t row, const std::string& want) {
        for (int i = 0; i < 40; ++i) {
            if (shown->line(row) == want) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        return false;
    };
    assert(waitLine(1, "DLS text        "));
    volumeUi.showVolume(45, false);
    assert(waitLine(1, volumeLine(16, 45, false)));
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    assert(waitLine(1, "DLS text        "));
    volumeUi.showVolume(45, true);
    assert(waitLine(1, volumeLine(16, 45, true)));

    std::cout << "All Navigation Stack Tests PASSED successfully!" << std::endl;
    return 0;
}
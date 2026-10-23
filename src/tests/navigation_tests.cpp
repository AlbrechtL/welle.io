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

    std::cout << "All Navigation Stack Tests PASSED successfully!" << std::endl;
    return 0;
}
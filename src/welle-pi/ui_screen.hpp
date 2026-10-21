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
#ifndef UI_SCREEN_HPP
#define UI_SCREEN_HPP

#include "input_queue.hpp"
#include "display_interface.hpp"

class UIScreen {
public:
    virtual ~UIScreen() = default;

    // UI thread, every tick while on top of the stack. Must not block.
    virtual void render(Frame& frame) = 0;
    virtual void handleInput(InputAction action) = 0;

    // Called when pushed to the top of the stack (first activation)
    virtual void onPush() {}

    // Called when popped off the stack (destruction)
    virtual void onPop() {}

    // Called when a child screen is pushed over this screen (suspended)
    virtual void onPause() {}

    // Called when a child screen is popped, returning focus to this screen
    virtual void onResume() {}
};

#endif // UI_SCREEN_HPP

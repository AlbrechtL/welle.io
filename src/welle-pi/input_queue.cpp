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
#include "input_queue.hpp"

void InputQueue::push(InputAction action) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.push(action);
    m_cond.notify_one();
}

InputAction InputQueue::pop(int timeout_ms) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_cond.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return !m_queue.empty(); })) {
        InputAction action = m_queue.front();
        m_queue.pop();
        return action;
    }
    return InputAction::NONE;
}

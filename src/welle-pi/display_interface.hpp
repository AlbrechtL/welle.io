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
#ifndef DISPLAY_INTERFACE_HPP
#define DISPLAY_INTERFACE_HPP

#include <algorithm>
#include <climits>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/* One screenful of text. Screens fill it, UIManager fits every line to
 * exactly cols code points, the display shows it. */
struct Frame {
    Frame(int cols, int rows, uint64_t now) : cols(cols), rows(rows), now(now), lines(rows) {}
    void set(int row, const std::string& text) {
        if (row >= 0 && row < rows) lines[row] = text;
    }
    /* Ask the display to draw this row inverted, for the focused item.
     * Returns false when the display cannot do that: the screen marks the
     * item in the text instead. */
    bool mark(int row) {
        if (!highlights || row < 0 || row >= rows) return false;
        highlight = row;
        return true;
    }

    int cols;
    int rows;
    uint64_t now;                   // now_ms() of this tick; screens animate from it
    std::vector<std::string> lines; // UTF-8
    bool highlights = false;        // UIManager: the display draws mark() itself
    int highlight = -1;             // the row a screen marked, -1 none
};

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual int cols() const = 0;
    virtual int rows() const = 0;
    // Called every UI tick, mostly with the same frame: skip what is unchanged.
    virtual void present(const Frame& frame) = 0;
    // Whether present() draws Frame::highlight inverted instead of the
    // screens wrapping the focused item in > and <.
    virtual bool canHighlight() const { return false; }
    virtual void backlightOn() {}
    virtual void backlightOff() {}
};

// The same frame on several displays (LCD + terminal), at the smallest size.
class MultiDisplay : public IDisplay {
public:
    explicit MultiDisplay(std::vector<std::unique_ptr<IDisplay>> displays) : m_displays(std::move(displays)) {}
    int cols() const override {
        int n = INT_MAX;
        for (const auto& d : m_displays) n = std::min(n, d->cols());
        return n;
    }
    int rows() const override {
        int n = INT_MAX;
        for (const auto& d : m_displays) n = std::min(n, d->rows());
        return n;
    }
    void present(const Frame& frame) override {
        for (auto& d : m_displays) d->present(frame);
    }
    // Only when every display can: the mixed pair keeps the text markers, so
    // that both show the same thing.
    bool canHighlight() const override {
        bool all = true;
        for (const auto& d : m_displays) all = all && d->canHighlight();
        return all;
    }
    void backlightOn() override {
        for (auto& d : m_displays) d->backlightOn();
    }
    void backlightOff() override {
        for (auto& d : m_displays) d->backlightOff();
    }

private:
    std::vector<std::unique_ptr<IDisplay>> m_displays;
};

#endif // DISPLAY_INTERFACE_HPP

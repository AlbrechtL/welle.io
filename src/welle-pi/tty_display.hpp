/******************************************************************************\
 * Description:
 * Text display on a terminal (or file): debugging without LCD hardware
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#ifndef TTY_DISPLAY_HPP
#define TTY_DISPLAY_HPP

#include "display_interface.hpp"
#include <cstdio>

/* Draws each changed frame in a box. On a terminal it clears the screen
 * first, so stderr messages get wiped: redirect them (2>log). out nullptr
 * discards everything (no LCD, no terminal). */
class TtyDisplay : public IDisplay {
public:
    TtyDisplay(int cols, int rows, std::FILE* out = stdout);

    int cols() const override { return m_cols; }
    int rows() const override { return m_rows; }
    // a terminal has reverse video; a redirected output has no attributes
    bool canHighlight() const override { return m_ansi; }
    void present(const Frame& frame) override;

private:
    int m_cols;
    int m_rows;
    std::FILE* m_out;
    bool m_ansi;
    int m_highlight = -1;
    std::vector<std::string> m_shown;
};

#endif // TTY_DISPLAY_HPP

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
#include "tty_display.hpp"

#include <unistd.h>

TtyDisplay::TtyDisplay(int cols, int rows, std::FILE* out)
    : m_cols(cols), m_rows(rows), m_out(out), m_ansi(out && isatty(fileno(out))) {}

void TtyDisplay::present(const Frame& frame) {
    if (!m_out || frame.lines == m_shown) return;
    m_shown = frame.lines;

    // frame size, not ours: mirrored next to a smaller LCD
    const std::string border = "+" + std::string(frame.cols, '-') + "+\n";
    std::string out = m_ansi ? "\033[H\033[2J" : "";
    out += border;
    for (const auto& line : frame.lines) {
        out += "|" + line + "|\n";
    }
    out += border;
    std::fputs(out.c_str(), m_out);
    std::fflush(m_out);
}

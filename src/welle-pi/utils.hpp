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
#ifndef UTILS_HPP
#define UTILS_HPP

#include <chrono>
#include <cstdint>
#include <string>

inline uint64_t now_ms() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

static const uint64_t SIGNAL_LOST_MS = 3000;

/* UTF-8 helpers counting code points, not bytes: DAB labels arrive as UTF-8
 * and must fit the display's columns. */
inline size_t utf8Length(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) n += (c & 0xC0) != 0x80;
    return n;
}

inline std::string utf8Substr(const std::string& s, size_t start, size_t count) {
    size_t i = 0, cp = 0;
    auto advance = [&]() { do ++i; while (i < s.size() && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80); };
    while (i < s.size() && cp < start) { advance(); ++cp; }
    const size_t from = i;
    for (cp = 0; i < s.size() && cp < count; ++cp) advance();
    return s.substr(from, i - from);
}

// Truncated or space-padded to exactly width code points.
inline std::string utf8Fit(const std::string& s, size_t width) {
    std::string r = utf8Substr(s, 0, width);
    r.append(width - utf8Length(r), ' ');
    return r;
}

#endif // UTILS_HPP

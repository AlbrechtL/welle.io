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
#include "screens.hpp"
#include "ui_manager.hpp"
#include "utils.hpp"
#include <algorithm>
#include <functional>
#include <map>
#include <vector>

// Lines of at most width code points, broken at spaces; longer words are cut.
static std::vector<std::string> wrapWords(const std::string& text, size_t width)
{
    std::vector<std::string> lines;
    std::string line;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find(' ', start);
        if (end == std::string::npos) end = text.size();
        std::string word = text.substr(start, end - start);
        start = end + 1;
        while (utf8Length(word) > width) {
            if (!line.empty()) lines.push_back(line), line.clear();
            lines.push_back(utf8Substr(word, 0, width));
            word = utf8Substr(word, width, std::string::npos);
        }
        if (word.empty()) continue;
        if (line.empty()) line = word;
        else if (utf8Length(line) + 1 + utf8Length(word) <= width) line += " " + word;
        else lines.push_back(line), line = word;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// Header on row 0, the list below with the focused item in the middle.
static void renderList(Frame& frame,
                       const std::string& header,
                       size_t itemsCount,
                       size_t selectedIndex,
                       const std::function<std::string(size_t index, bool isFocused, int width)>& getItemString)
{
    frame.set(0, header);

    if (itemsCount == 0) {
        frame.set(1, "Empty!");
        return;
    }

    const int width = frame.cols;
    const int list_lines = std::max(frame.rows - 1, 1);
    const int focused_line = list_lines / 2; // e.g. for height=4 (list_lines=3), focused_line=1
    const int count = (int)itemsCount;
    // fewer items than lines: each once, not repeated by the wrap round
    const int first_offset = -std::min(focused_line, (count - 1) / 2);

    for (int line = 0; line < list_lines; ++line) {
        const int offset = line - focused_line;
        if (offset < first_offset || offset >= first_offset + count) continue;
        const size_t item_index = (((int)selectedIndex + offset) % count + count) % count;

        std::string option = getItemString(item_index, offset == 0, width);
        if (offset == 0) {
            option = width < 16 ? ">" + option : ">" + utf8Fit(option, width - 2) + "<";
        }
        frame.set(1 + line, option);
    }
}

// ScanningScreen implementation
void ScanningScreen::setScanningStatus(const std::string& channel, int foundCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channel = channel;
    m_found = foundCount;
}

void ScanningScreen::render(Frame& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    frame.set(0, "Tuning: " + m_channel + "...");
    frame.set(1, "Found: " + std::to_string(m_found));
}

void ScanningScreen::handleInput(InputAction action) {
    (void)action;
}

// RadioScreen implementation
RadioScreen::RadioScreen() : m_signalPresent(true), m_signalChanged(now_ms()) {}

void RadioScreen::setChannelName(const std::string& channel_name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_channelName = channel_name;
}

void RadioScreen::setProgramName(const std::string& program_name, const std::string& short_program) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_programName != program_name || m_shortProgramName != short_program) {
        m_programName = program_name;
        m_shortProgramName = short_program;
        m_dlsQueue.clear();
        m_dls.clear();
    }
}

void RadioScreen::setDLS(const std::string& dls) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_dlsQueue.empty() || m_dlsQueue.back() != dls) {
        m_dlsQueue.push_back(dls);
    }
}

void RadioScreen::setSignalPresent(bool present) {
    if (m_signalPresent.exchange(present) != present) {
        m_signalChanged = now_ms();
    }
}

bool RadioScreen::signalLost() const {
    return !m_signalPresent && (now_ms() - m_signalChanged > SIGNAL_LOST_MS);
}

void RadioScreen::render(Frame& frame) {
    const bool lost = signalLost();
    const size_t cols = frame.cols;
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string program = m_programName;
    if (utf8Length(program) > cols && !m_shortProgramName.empty()) {
        program = m_shortProgramName;
    }
    frame.set(0, program);
    if (frame.rows > 2) {
        frame.set(1, m_channelName);
    }
    if (frame.rows < 2) {
        return;
    }

    // 5+ rows: the DLS word-wrapped one blank row below the channel, else a one-row marquee
    const size_t area = frame.rows > 4 ? frame.rows - 3 : 1;
    // One pass of the message: the marquee once round, or one DLS_LINE_MS per line
    auto passMs = [&]() -> uint64_t {
        if (area > 1) {
            return wrapWords(m_dls, cols).size() * DLS_LINE_MS;   // time to read, also if it fits
        }
        const size_t len = utf8Length(m_dls);
        return len > cols ? (len + utf8Length(DLS_GAP)) * DLS_STEP_MS : 0;
    };
    // The next message waits until the current one went round once.
    if (!lost && (m_dls.empty() || frame.now - m_dlsStart >= passMs() + DLS_REST_MS)) {
        if (!m_dlsQueue.empty()) {
            m_dls = m_dlsQueue.front();
            m_dlsQueue.pop_front();
        }
        m_dlsStart = frame.now;
    }

    if (area > 1) {
        if (lost) {
            frame.set(frame.rows - 1, NO_SIGNAL_TEXT);
            return;
        }
        // first window stays until its lines are read, then one line up per DLS_LINE_MS
        const auto lines = wrapWords(m_dls, cols);
        const size_t read = (frame.now - m_dlsStart) / DLS_LINE_MS + 1;
        const size_t last = lines.size() > area ? lines.size() - area : 0;
        const size_t first = std::min(read > area ? read - area : 0, last);
        const size_t shown = std::min(area, lines.size());
        for (size_t i = 0; i < shown; ++i) {   // bottom-aligned
            frame.set(frame.rows - shown + i, lines[first + i]);
        }
        return;
    }

    std::string text;
    if (lost) {
        text = NO_SIGNAL_TEXT;
    } else if (passMs() == 0) {
        text = m_dls;
    } else {
        const size_t step = std::min<uint64_t>((frame.now - m_dlsStart) / DLS_STEP_MS, utf8Length(m_dls) + utf8Length(DLS_GAP));
        text = utf8Substr(m_dls + DLS_GAP + m_dls, step, cols);
    }
    // two lines: the channel only while there is no DLS
    frame.set(frame.rows - 1, text.empty() && frame.rows == 2 ? m_channelName : text);
}

void RadioScreen::handleInput(InputAction action) {
    if (m_inputCallback) {
        m_inputCallback(action);
    }
}

void RadioScreen::setInputCallback(std::function<void(InputAction)> cb) {
    m_inputCallback = cb;
}

// StationListScreen implementation
StationListScreen::StationListScreen(const std::vector<ConfigManager::Station>& stations, UIManager* uiManager)
    : m_stations(stations), m_uiManager(uiManager), m_index(0), m_selected(false)
{
    std::map<std::string, int> count;
    for (const auto& s : m_stations) ++count[s.program];
    for (const auto& s : m_stations) m_showChannel.push_back(count[s.program] > 1);
}

void StationListScreen::render(Frame& frame) {
    const size_t index = m_index;
    const int width = frame.cols;

    std::string header = "Select: " + std::to_string(index + 1) + "/" + std::to_string(m_stations.size());
    if (header.length() > (size_t)width && width >= 4) {
        header = std::to_string(index + 1) + "/" + std::to_string(m_stations.size());
    }

    renderList(frame, header, m_stations.size(), index, [&](size_t idx, bool isFocused, int w) {
        const auto& station = m_stations[idx];
        std::string name = isFocused && w < 16 && !station.short_program.empty() ? station.short_program : station.program;
        const size_t room = !isFocused ? w : w >= 16 ? w - 2 : w - 1;   // focused: ">name<" or ">name"
        // name listed more than once, whatever the SIDs: channel right-aligned
        const std::string channel = m_showChannel[idx] ? " " + station.channel : "";
        if (room <= channel.size()) return name;
        const size_t nameRoom = room - channel.size();
        const size_t len = utf8Length(name);
        // focused and too long: its end every other NAME_SWITCH_MS
        if (isFocused && len > nameRoom && frame.now / NAME_SWITCH_MS % 2) {
            name = utf8Substr(name, len - nameRoom, nameRoom);
        }
        return channel.empty() ? name : utf8Fit(name, nameRoom) + channel;
    });
}

void StationListScreen::handleInput(InputAction action) {
    if (m_stations.empty()) return;
    if (action == InputAction::UP) {
        m_index = m_index == 0 ? m_stations.size() - 1 : m_index - 1;
    } else if (action == InputAction::DOWN) {
        m_index = m_index == m_stations.size() - 1 ? 0 : m_index + 1;
    } else if (action == InputAction::ENTER) {
        m_selected = true;
        if (m_selectCallback) {
            m_selectCallback(m_stations[m_index]);
        }
    } else if (action == InputAction::LEFT || action == InputAction::MENU || action == InputAction::QUIT) {
        if (m_uiManager) {
            m_uiManager->popScreen();
        }
    }
}

void StationListScreen::setSelectCallback(std::function<void(const ConfigManager::Station&)> cb) {
    m_selectCallback = cb;
}

void StationListScreen::setIndex(size_t index) {
    if (index < m_stations.size()) {
        m_index = index;
    }
}

bool StationListScreen::isSelected() const { return m_selected; }
void StationListScreen::resetSelected() { m_selected = false; }

// MenuScreen implementation
MenuScreen::MenuScreen(UIManager* uiManager) : m_uiManager(uiManager), m_index(0) {
    m_options.push_back("Manual Rescan");
    m_options.push_back("Back to Radio");
    m_options.push_back("Exit");
}

void MenuScreen::render(Frame& frame) {
    renderList(frame, "Settings Menu", m_options.size(), m_index, [&](size_t idx, bool, int) {
        return m_options[idx];
    });
}

void MenuScreen::handleInput(InputAction action) {
    if (action == InputAction::UP) {
        m_index = m_index == 0 ? m_options.size() - 1 : m_index - 1;
    } else if (action == InputAction::DOWN) {
        m_index = m_index == m_options.size() - 1 ? 0 : m_index + 1;
    } else if (action == InputAction::ENTER) {
        if (m_index == 1) { // Back to Radio
            if (m_uiManager) {
                m_uiManager->popScreen();
            }
        } else {
            if (m_selectCallback) {
                m_selectCallback(m_index);
            }
        }
    } else if (action == InputAction::MENU || action == InputAction::LEFT) {
        if (m_uiManager) {
            m_uiManager->popScreen();
        }
    }
}

void MenuScreen::setSelectCallback(std::function<void(int)> cb) {
    m_selectCallback = cb;
}

/******************************************************************************\
 * Description:
 * Tests for the frame-based display: UTF-8 fitting, screens, TtyDisplay, UIManager
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
#include "screens.hpp"
#include "tty_display.hpp"
#include "ui_manager.hpp"
#include "utils.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

static std::vector<std::string> render(UIScreen& screen, int cols, int rows, uint64_t now) {
    Frame frame(cols, rows, now);
    screen.render(frame);
    return frame.lines;
}

static void testUtf8() {
    assert(utf8Length("Österreich") == 10);
    assert(utf8Substr("aÖbc", 1, 2) == "Öb");
    assert(utf8Fit("Öx", 4) == "Öx  ");
    assert(utf8Fit("Ö123456", 3) == "Ö12");
}

static void testRadioMarquee() {
    const uint64_t T = 10000, STEP = RadioScreen::DLS_STEP_MS, REST = RadioScreen::DLS_REST_MS;
    RadioScreen radio;
    radio.setChannelName("11D");
    radio.setProgramName("Programme long name", "Prog");

    // 2 rows: short name if the long one does not fit, channel until DLS arrives
    auto lines = render(radio, 8, 2, T);
    assert(lines[0] == "Prog");
    assert(lines[1] == "11D");

    radio.setDLS("short");
    radio.setDLS("0123456789AB");   // 12 > 8 columns: scrolls
    assert(render(radio, 8, 2, T)[1] == "short");
    assert(render(radio, 8, 2, T + REST - 1)[1] == "short");    // shown at least REST
    const uint64_t t = T + REST;                                // next message
    assert(render(radio, 8, 2, t)[1] == "01234567");
    assert(render(radio, 8, 2, t + STEP)[1] == "12345678");
    assert(render(radio, 8, 2, t + 12 * STEP)[1] == "     012");  // gap, then text again
    assert(render(radio, 8, 2, t + 17 * STEP)[1] == "01234567");  // once round: rests at start
    assert(render(radio, 8, 2, t + 17 * STEP + REST - 1)[1] == "01234567");
    const uint64_t again = t + 17 * STEP + REST;   // restarts from the tick that noticed
    assert(render(radio, 8, 2, again)[1] == "01234567");
    assert(render(radio, 8, 2, again + STEP)[1] == "12345678");

    // 4 rows: channel on row 1, DLS marquee on the last row
    lines = render(radio, 20, 4, again);
    assert(lines[1] == "11D" && lines[2].empty() && lines[3] == "0123456789AB");

    // a new programme drops the DLS of the old one
    radio.setProgramName("Other");
    assert(render(radio, 8, 2, again)[1] == "11D");
}

static void testRadioWrap() {
    const uint64_t T = 10000, LINE = RadioScreen::DLS_LINE_MS, REST = RadioScreen::DLS_REST_MS;
    RadioScreen radio;
    radio.setProgramName("P");
    radio.setChannelName("11D");
    // 8 columns: "one two" / "three" / "four" / "five six" / "abcdefgh" / "ij Grüße"
    radio.setDLS("one two three four five six abcdefghij Grüße");
    radio.setDLS("next");
    auto dls = [&](uint64_t t) {
        auto lines = render(radio, 8, 6, t);   // blank row, 3 DLS rows
        assert(lines[0] == "P" && lines[1] == "11D" && lines[2].empty());
        return lines[3] + "|" + lines[4] + "|" + lines[5];
    };
    assert(dls(T) == "one two|three|four");
    assert(dls(T + 3 * LINE - 1) == "one two|three|four");   // until the 3 lines are read
    assert(dls(T + 3 * LINE) == "three|four|five six");    // then one line up per LINE
    assert(dls(T + 5 * LINE) == "five six|abcdefgh|ij Grüße");
    assert(dls(T + 6 * LINE + REST - 1) == "five six|abcdefgh|ij Grüße");   // last lines stay
    assert(dls(T + 6 * LINE + REST) == "||next");   // from the bottom
}

static void testMenu() {
    MenuScreen menu(nullptr);
    auto lines = render(menu, 20, 4, 0);
    assert(lines[0] == "Settings Menu");
    assert(lines[1] == "Exit");                     // wraps round above the focus
    assert(lines[2] == ">Manual Rescan     <");
    assert(lines[3] == "Back to Radio");
    menu.handleInput(InputAction::DOWN);
    assert(render(menu, 20, 4, 0)[2] == ">Back to Radio     <");
    assert(render(menu, 12, 2, 0)[1] == ">Back to Radio");   // narrow: no padding, display cuts
    // 7 lines, 3 items: each once around the focus
    const std::vector<std::string> tall = {"Settings Menu", "", "", "Manual Rescan", ">Back to Radio     <", "Exit", "", ""};
    assert(render(menu, 20, 8, 0) == tall);
}

static void testTty() {
    char* buf = nullptr;
    size_t size = 0;
    std::FILE* out = open_memstream(&buf, &size);
    {
        TtyDisplay tty(4, 2, out);
        Frame frame(4, 2, 0);
        frame.lines = {"ab  ", "Öx  "};
        tty.present(frame);
        tty.present(frame);     // unchanged: not drawn again
    }
    std::fclose(out);
    assert(std::string(buf) == "+----+\n|ab  |\n|Öx  |\n+----+\n");
    std::free(buf);
}

class RecordingDisplay : public IDisplay {
public:
    int cols() const override { return 10; }
    int rows() const override { return 2; }
    void present(const Frame& frame) override {
        std::lock_guard<std::mutex> lock(mutex);
        last = frame.lines;
    }
    std::vector<std::string> get() {
        std::lock_guard<std::mutex> lock(mutex);
        return last;
    }
    std::mutex mutex;
    std::vector<std::string> last;
};

static void testMirror() {
    char* buf = nullptr;
    size_t size = 0;
    std::FILE* out = open_memstream(&buf, &size);
    {
        std::vector<std::unique_ptr<IDisplay>> displays;
        displays.push_back(std::make_unique<RecordingDisplay>());   // 10 x 2
        auto recording = static_cast<RecordingDisplay*>(displays.back().get());
        displays.push_back(std::make_unique<TtyDisplay>(40, 4, out));
        MultiDisplay multi(std::move(displays));
        assert(multi.cols() == 10 && multi.rows() == 2);   // smallest wins
        Frame frame(10, 2, 0);
        frame.lines = {"0123456789", "abcdefghij"};
        multi.present(frame);
        assert(recording->get() == frame.lines);
    }
    std::fclose(out);
    // the terminal draws the frame's size, not its own 40 x 4
    assert(std::string(buf) == "+----------+\n|0123456789|\n|abcdefghij|\n+----------+\n");
    std::free(buf);
}

static void testUIManager() {
    auto owned = std::make_unique<RecordingDisplay>();
    RecordingDisplay* display = owned.get();
    UIManager ui(std::move(owned));
    auto menu = std::make_shared<MenuScreen>(&ui);
    ui.resetToRoot(menu);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // lines fitted to exactly cols code points
    assert(display->get() == (std::vector<std::string>{"Settings M", ">Manual Re"}));
    ui.processInput(InputAction::DOWN);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    assert(display->get()[1] == ">Back to R");
}

int main() {
    testUtf8();
    testRadioMarquee();
    testRadioWrap();
    testMenu();
    testTty();
    testMirror();
    testUIManager();
    std::cout << "display_tests: OK" << std::endl;
    return 0;
}

/******************************************************************************\
 * Description:
 * Input source: terminal keys on stdin
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
#include "input_sources.hpp"

#include <cerrno>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

void runStdinReader(InputQueue& queue, const StopPredicate& stop_requested) {
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }

    while (!stop_requested()) {
        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;

        int r = poll(&pfd, 1, 500);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (r == 0) continue;

        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        if (c == '\033') { // Escape sequence (potential arrow key)
            struct pollfd pfd_next;
            pfd_next.fd = STDIN_FILENO;
            pfd_next.events = POLLIN;
            if (poll(&pfd_next, 1, 50) > 0) {
                char seq[2];
                if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
                    if (poll(&pfd_next, 1, 50) > 0) {
                        if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                            if (seq[1] == 'A') {
                                queue.push(InputAction::UP);
                                continue;
                            } else if (seq[1] == 'B') {
                                queue.push(InputAction::DOWN);
                                continue;
                            } else if (seq[1] == 'C') {
                                queue.push(InputAction::RIGHT);
                                continue;
                            } else if (seq[1] == 'D') {
                                queue.push(InputAction::LEFT);
                                continue;
                            }
                        }
                    }
                }
            }
            // Just the ESC key alone: map to MENU (cancel/back)
            queue.push(InputAction::MENU);
        } else if (c == '.' || c == 'q') {
            queue.push(InputAction::QUIT);
            break;
        } else if (c == 'u') {
            queue.push(InputAction::UP);
        } else if (c == 'd') {
            queue.push(InputAction::DOWN);
        } else if (c == 'l') {
            queue.push(InputAction::LEFT);
        } else if (c == 'r') {
            queue.push(InputAction::RIGHT);
        } else if (c == 'e' || c == '\n' || c == '\r') {
            queue.push(InputAction::ENTER);
        } else if (c == 'm') {
            queue.push(InputAction::MENU);
        }
    }

    if (tcgetattr(STDIN_FILENO, &newt) == 0) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

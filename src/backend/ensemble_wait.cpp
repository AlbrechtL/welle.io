/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
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

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "backend/ensemble_wait.h"
#include "backend/radio-receiver.h"

using namespace std;

/* Give up waiting for the rest of the ensemble after that long, and work with
 * the services we have. */
static const uint64_t ENSEMBLE_TIMEOUT_MS = 15000;

/* An ensemble that does not announce how many services it carries is
 * considered complete when no new service appeared for that long. */
static const uint64_t ENSEMBLE_STABLE_MS = 2000;

static uint64_t now_ms()
{
    return chrono::duration_cast<chrono::milliseconds>(
            chrono::steady_clock::now().time_since_epoch()).count();
}

/* A service the ensemble announced but whose label did not arrive yet has an
 * empty label, and DAB labels are padded with spaces. */
static bool has_label(const Service& s)
{
    return s.serviceLabel.utf8_label().find_first_not_of(' ') != string::npos;
}

void wait_for_complete_ensemble(RadioReceiver& rx,
        const std::function<bool()>& stop_requested)
{
    const auto stop = [&]() { return stop_requested and stop_requested(); };

    const uint64_t deadline = now_ms() + ENSEMBLE_TIMEOUT_MS;
    size_t seen = 0;
    uint64_t last_new_service = now_ms();

    while (not stop() and now_ms() < deadline) {
        const auto services = rx.getServiceList();
        const size_t labelled = count_if(services.begin(), services.end(),
                has_label);

        if (services.size() != seen) {
            seen = services.size();
            last_new_service = now_ms();
        }

        if (not services.empty() and labelled == services.size()) {
            /* FIG0/7 tells us how many services to expect. Ensembles that do
             * not signal it leave us with waiting for the list to settle. */
            const uint8_t announced = rx.getAnnouncedServiceCount();
            if (announced != 0 ? services.size() >= announced :
                    now_ms() - last_new_service > ENSEMBLE_STABLE_MS) {
                cerr << "Ensemble complete, " << services.size() <<
                    " services" << endl;
                return;
            }
        }

        this_thread::sleep_for(chrono::milliseconds(250));
    }

    if (not stop()) {
        const auto services = rx.getServiceList();
        const size_t labelled = count_if(services.begin(), services.end(),
                has_label);
        cerr << "Ensemble may be incomplete, " << services.size() <<
            " services announced " << (int)rx.getAnnouncedServiceCount() <<
            ", " << services.size() - labelled << " without a label" << endl;
    }
}

/*
 *    Copyright (C) 2026
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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

// Decodes a recording with all services and prints what the receiver
// reported as JSON. Used to create and review the expectations in
// manifest.json of the welle.io-test-data repository, see
// scripts/update_expectations.py there.

#include <iostream>

#include <QJsonDocument>

#include "recording_runner.h"

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <recording> [u8|s8|s16le|s16be|cf32|auto]" << std::endl;
        return 2;
    }

    const std::string format = argc == 3 ? argv[2] : "auto";
    const RunResult result = runRecording(argv[1], format);

    std::cout << QJsonDocument(toJson(result)).toJson().toStdString();
    return result.sync ? 0 : 1;
}

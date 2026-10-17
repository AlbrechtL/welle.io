/*
 *    Versions of the libraries welle.io is built with (audio decoders, FFT,
 *    SDR drivers), as reported by the libraries at run time where possible.
 *
 *    This file is part of the welle.io.
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
 *    This file was generated with the help of an AI assistant (Claude Code).
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

// Name and version of every library of the build, e.g. {"FFTW", "fftw-3.3.10"}
std::vector<std::pair<std::string, std::string>> getLibraryVersions();

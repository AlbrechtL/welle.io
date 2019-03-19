/*
 *    Copyright (C) 2019
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

#if defined(WITH_PROFILING)
#include <iostream>
#include <fstream>
#include <map>
#include <utility>
#include <cmath>

#include "various/profiling.h"

using namespace std;

static Profiler profiler;

Profiler& get_profiler() {
    return profiler;
}

#define MARK_TO_CSTR_CASE(m) case ProfilingMark::m: return #m;
const char* mark_to_cstr(const ProfilingMark& m) {
    switch (m) {
        MARK_TO_CSTR_CASE(NotSynced)
        MARK_TO_CSTR_CASE(SyncOnEndNull)
        MARK_TO_CSTR_CASE(SyncOnPhase)
        MARK_TO_CSTR_CASE(FindIndex)
        MARK_TO_CSTR_CASE(DataSymbols)
        MARK_TO_CSTR_CASE(PushAllSymbols)
        MARK_TO_CSTR_CASE(OnNewNull)
        MARK_TO_CSTR_CASE(DecodeTII)

        MARK_TO_CSTR_CASE(ProcessPRS)
        MARK_TO_CSTR_CASE(ProcessSymbol)
        MARK_TO_CSTR_CASE(Deinterleaver)
        MARK_TO_CSTR_CASE(FICHandler)
        MARK_TO_CSTR_CASE(MSCHandler)
        MARK_TO_CSTR_CASE(SymbolProcessed)

        MARK_TO_CSTR_CASE(DAGetMSCData)
        MARK_TO_CSTR_CASE(DADeinterleave)
        MARK_TO_CSTR_CASE(DADeconvolve)
        MARK_TO_CSTR_CASE(DADispersal)
        MARK_TO_CSTR_CASE(DADecode)
        MARK_TO_CSTR_CASE(DADone)
    }

    return "unknown";
}

Profiler::Profiler() {
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startup_time_cputime);
    clock_gettime(CLOCK_MONOTONIC, &startup_time_monotonic);
}

struct timespec& operator+=(struct timespec& t1, const struct timespec& t2) {
    t1.tv_sec += t2.tv_sec;
    t1.tv_nsec += t2.tv_nsec;

    if (t1.tv_nsec >= 1000000000ll) {
        t1.tv_nsec -= 1000000000ll;
        t1.tv_sec += 1;
    }

    return t1;
}

struct timespec operator-(struct timespec t1, struct timespec t2) {
    struct timespec t;

    if (t1.tv_sec < t2.tv_sec) {
        cerr << "******************* Timestamp difference negative" << endl;
        t.tv_sec = 0;
        t.tv_nsec = 0;
        return t;
    }

    t.tv_sec = t1.tv_sec - t2.tv_sec;

    if (t1.tv_nsec >= t2.tv_nsec) {
        t.tv_nsec = t1.tv_nsec - t2.tv_nsec;
    }
    else {
        t.tv_sec -= 1;
        t.tv_nsec = 1000000000ll + t1.tv_nsec - t2.tv_nsec;
    }
    return t;
}

std::ostream& operator<<(std::ostream& out, const struct timespec& ts)
{
    char nanos[32];
    snprintf(nanos, 31, "%09ld", ts.tv_nsec);
    return out << ts.tv_sec << "." << nanos;
}

Profiler::~Profiler() {
    struct timespec stop_time_cputime;
    struct timespec stop_time_monotonic;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop_time_cputime);
    clock_gettime(CLOCK_MONOTONIC, &stop_time_monotonic);

    ofstream dump("profiling_points.csv");
    dump << "thread_id,mark,time_sec,time_ns" << endl;
    for (const auto tp: m_timepoints) {
        for (const auto points : tp.second) {
            dump << tp.first << "," <<
                mark_to_cstr(points.p) << "," <<
                points.timestamp.tv_sec << "," <<
                points.timestamp.tv_nsec << endl;
        }
    }

    ofstream profiling("profiling_stats.csv");
    profiling << "cputime,start," << startup_time_cputime << endl;
    profiling << "cputime,stop," << stop_time_cputime << endl;
    profiling << "monotonic,start," << startup_time_monotonic << endl;
    profiling << "monotonic,stop," << stop_time_monotonic << endl;
    profiling << "cputime,diff," << stop_time_cputime - startup_time_cputime << endl;
    profiling << "monotonic,diff," << stop_time_monotonic - startup_time_monotonic << endl;
    profiling << "frames,decoded," << num_frames_decoded << endl;

    // See http://www.graphviz.org/documentation/
    ofstream graph("profiling.dot");

    graph << "digraph G { " << endl;

    size_t count = 0;

    for (const auto tp: m_timepoints) {
        if (tp.second.size() < 2) {
            continue;
        }

        graph << "subgraph cluster_" << tp.first << " { " << endl;
        graph << "colorscheme=\"gnbu8\";" << endl;
        graph << "bgcolor=" << (count % 8) + 1 << ";" << endl;
        count++;

        map<pair<ProfilingMark, ProfilingMark>, struct timespec> from_to_times;

        for (auto it = tp.second.begin(); next(it) != tp.second.end(); ++it) {
            from_to_times[make_pair(it->p, next(it)->p)] += next(it)->timestamp - it->timestamp;
        }

        double maxw = 0;
        for (auto& d : from_to_times) {
            double w = log10(1 + d.second.tv_sec * 1000 + d.second.tv_nsec / 1000000);
            if (w > maxw) maxw = w;
        }

        for (auto& d : from_to_times) {
            int w = (d.second.tv_sec * 1000 + d.second.tv_nsec / 1000000);

            char color[16];
            snprintf(color, 15, "#%02x%02x%02x", (int)(255 * log10(w+1)/maxw), 0, 0);

            graph << mark_to_cstr(d.first.first) << " -> " << mark_to_cstr(d.first.second) <<
                " [color=\"" << color << "\""
                " label=\"" << w << "ms\""
                "];" << endl;
        }
        graph << "}" << endl;
    }
    graph << "}" << endl;
}

void Profiler::save_time(const ProfilingMark m) {
    const auto id = this_thread::get_id();

    struct timespec now;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now);

    lock_guard<mutex> lock(m_mutex);
    m_timepoints[id].emplace_back(now, m);
}

void Profiler::frame_decoded() {
    num_frames_decoded++;
}

#endif // defined(WITH_PROFILING)

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

#include <cstdint>
#include <ctime>
#include <list>
#include <unordered_map>
#include <thread>
#include <mutex>

#define PROFILE(m) get_profiler().save_time(ProfilingMark::m)
#define PROFILE_FRAME_DECODED() get_profiler().frame_decoded()

enum class ProfilingMark {
    NotSynced,
    SyncOnEndNull,
    SyncOnPhase,
    FindIndex,
    DataSymbols,
    PushAllSymbols,
    OnNewNull,
    DecodeTII,

    ProcessPRS,
    ProcessSymbol,
    Deinterleaver,
    FICHandler,
    MSCHandler,
    SymbolProcessed,

    DAGetMSCData,
    DADeinterleave,
    DADeconvolve,
    DADispersal,
    DADecode,
    DADone,
};

struct ProfilingTimepoint {
    ProfilingTimepoint(
            struct timespec timestamp,
            ProfilingMark p) :
        timestamp(timestamp),
        p(p) { }

    struct timespec timestamp;
    ProfilingMark p;
};

class Profiler
{
    public:
        Profiler();
        Profiler(const Profiler&) = delete;
        Profiler& operator=(const Profiler&) = delete;
        ~Profiler();

        void save_time(const ProfilingMark m);
        void frame_decoded();
    private:
        std::mutex m_mutex;
        std::unordered_map<
            std::thread::id,
            std::list<ProfilingTimepoint> > m_timepoints;

        struct timespec startup_time_cputime;
        struct timespec startup_time_monotonic;
        size_t num_frames_decoded = 0;
};

Profiler& get_profiler(void);

#else
# define PROFILE(m)
# define PROFILE_FRAME_DECODED()
#endif // defined(WITH_PROFILING)


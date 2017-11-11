#include "Tools.h"

CTimeDuration::CTimeDuration()
{
    start();
}

void CTimeDuration::start()
{
    m_start = std::chrono::steady_clock::now();
}

void CTimeDuration::stop()
{
    m_stop = std::chrono::steady_clock::now();
}

float CTimeDuration::getDuration()
{
    auto dur = m_stop - m_start;
    return dur.count() / 1e6; // ms
}

#ifndef TOOLS_H
#define TOOLS_H

#include <chrono>

class CTimeDuration
{
public:
    CTimeDuration();
    void start(void);
    void stop(void);
    float getDuration(void);

private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
    std::chrono::time_point<std::chrono::steady_clock> m_stop;
};

#endif // TOOLS_H

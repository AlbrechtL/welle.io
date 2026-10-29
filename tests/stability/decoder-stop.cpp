#include "backend/dab-audio.h"
#include "welle-cli/webprogrammehandler.h"
#include <chrono>
#include <thread>
#include <iostream>
int main() {
    WebProgrammeHandler handler(1, OutputCodec::MP3);
    ProtectionSettings settings;
    for (int n = 0; n < 300; ++n) {
        DabAudio audio(AudioServiceComponentType::DABPlus, 48 * 64, 64, settings, handler, "");
        if (n % 3 == 0) std::this_thread::sleep_for(std::chrono::microseconds(100));
        else if (n % 3 == 1) std::this_thread::yield();
    }
    std::cout << "300 real decoder start/stop cycles: OK" << std::endl;
}

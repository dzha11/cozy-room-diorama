// Cozy Room Diorama - Computer Graphics Fundamentals final project.
// Entry point: parses a few optional command-line flags and runs the app.

#include "App.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    LaunchOptions opt;
    for (int i = 1; i < argc; ++i) {
        auto next = [&]() -> const char* { return i + 1 < argc ? argv[++i] : ""; };
        if (!std::strcmp(argv[i], "--capture")) opt.capturePath = next();
        else if (!std::strcmp(argv[i], "--frames")) opt.captureFrames = std::atoi(next());
        else if (!std::strcmp(argv[i], "--hour")) opt.hour = float(std::atof(next()));
        else if (!std::strcmp(argv[i], "--preset")) opt.preset = std::atoi(next());
        else if (!std::strcmp(argv[i], "--lights")) opt.lightsOn = true;
        else if (!std::strcmp(argv[i], "--ui")) opt.withUI = true;
        else if (!std::strcmp(argv[i], "--novsync")) opt.noVsync = true;
        else if (!std::strcmp(argv[i], "--size")) {
            opt.width = std::atoi(next());
            opt.height = std::atoi(next());
        } else {
            std::cout << "Usage: CozyRoom [--hour H] [--preset 0-3] [--lights] [--novsync]\n"
                         "                [--capture out.png --frames N [--ui]] [--size W H]\n";
            return 0;
        }
    }
    try {
        App app(opt);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}

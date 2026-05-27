#include "engine.h"

#include <exception>
#include <iostream>

// NOLINTNEXTLINE(bugprone-exception-escape) — every throw site is caught below.
int main() {
    try {
        Sylva::Engine engine;
        if (!engine.initialize()) {
            std::cerr << "Engine initialization failed!" << '\n';
            return 1;
        }
        engine.run();
        engine.shutdown();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Fatal: unknown exception" << '\n';
        return 1;
    }
}

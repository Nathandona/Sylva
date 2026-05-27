#include "engine.h"

#include <iostream>

int main() {
    Sylva::Engine engine;
    if (!engine.initialize()) {
        std::cerr << "Engine initialization failed!" << '\n';
        return 1;
    }
    engine.run();
    engine.shutdown();
    return 0;
}

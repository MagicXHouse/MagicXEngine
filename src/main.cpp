#include "Core/Logger.h"
#include "Engine.h"

#include <exception>
#include <string>

int main() {
    try {
        MagicXEngine::Engine engine;
        engine.Run();
        return 0;
    } catch (const std::exception& e) {
        MagicXEngine::Core::LogError(std::string("Fatal error: ") + e.what());
        return 1;
    }
}

#include "MagicXEngine/Core/Logger.h"
#include <cstdio>

namespace MagicXEngine::Core {

void Log(LogLevel level, const std::string& message) {
    const char* tag = "";
    switch (level) {
        case LogLevel::Debug: tag = "[DBG] "; break;
        case LogLevel::Info:  tag = "[INF] "; break;
        case LogLevel::Warn:  tag = "[WRN] "; break;
        case LogLevel::Error: tag = "[ERR] "; break;
    }
    std::FILE* out = (level == LogLevel::Error) ? stderr : stdout;
    std::fprintf(out, "%s%s\n", tag, message.c_str());
}

} // namespace MagicXEngine::Core


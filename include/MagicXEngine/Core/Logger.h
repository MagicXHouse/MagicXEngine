#pragma once
#include <string>

namespace MagicXEngine::Core {

enum class LogLevel { Debug, Info, Warn, Error };

void Log(LogLevel level, const std::string& message);

inline void LogDebug(const std::string& m) { Log(LogLevel::Debug, m); }
inline void LogInfo(const std::string& m)  { Log(LogLevel::Info, m); }
inline void LogWarn(const std::string& m)  { Log(LogLevel::Warn, m); }
inline void LogError(const std::string& m) { Log(LogLevel::Error, m); }

} // namespace MagicXEngine::Core


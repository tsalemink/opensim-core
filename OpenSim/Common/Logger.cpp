/* -------------------------------------------------------------------------- *
 *                           OpenSim:  Logger.cpp                             *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2019 Stanford University and the Authors                *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "Logger.h"

#include "Exception.h"
#include "IO.h"
#include "LogSink.h"

#include "spdlog/sinks/stdout_color_sinks.h"

using namespace OpenSim;

// cout logger will be initialized during static initialization time
static std::shared_ptr<spdlog::logger> cout_logger = []() {
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("cout");
    logger->set_level(spdlog::level::info);
    logger->set_pattern("%v");
    return logger;
}();

// default logger will be initialized during static initialization time
static std::shared_ptr<spdlog::logger> default_logger = []() {
    std::shared_ptr<spdlog::logger> logger = spdlog::default_logger();
    logger->set_level(spdlog::level::info);
    logger->set_pattern("[%l] %v");
    return logger;
}();

// this initialization will also happen during static initialization time
static bool other_init = []() {
    spdlog::flush_on(spdlog::level::info);
    return true;
}();

// the file log sink (e.g. `opensim.log`) is *not* necessarily initialized
// during static initialization time. It is only initialized when the first log
// message is about to be written. Users *may* disable this functionality
// before the first log message is written.
static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> m_filesink = nullptr;

// if a user calls `Logger::removeFileSink` before m_filesink is initialized, a
// flag should be set to ensure that the "first use" initialization does not
// subsequently happen.
static bool filesink_auto_initialization_disabled = false;

// when filesink initialization *may* happen, ensure it only happens exactly
// once globally by wrapping it in a function-local static.
static bool initFileLoggingAsNeeded() {
#ifdef OPENSIM_DISABLE_LOG_FILE
// software builders may want to statically ensure that automatic file logging
// *cannot* happen - even during static initialization. This compiler define
// outright disables the behavior, which is important in Windows applications
// that run multiple instances of OpenSim-linked binaries. There, the logs may
// collide and cause a "multiple processes cannot open the same file" error).
return true;
#else
    static bool initialized = []() {
        if (filesink_auto_initialization_disabled) {
            return true;
        }
        Logger::addFileSink();
        return true;
    }();

    return initialized;
#endif
}

// this function is only called when the caller is about to log something, so
// it should perform lazy initialization of the file sink
spdlog::logger& Logger::getCoutLogger() {
    initFileLoggingAsNeeded();
    return *cout_logger;
}

// this function is only called when the caller is about to log something, so
// it should perform lazy initialization of the file sink
spdlog::logger& Logger::getDefaultLogger() {
    initFileLoggingAsNeeded();
    return *default_logger;
}

static void addSinkInternal(std::shared_ptr<spdlog::sinks::sink> sink) {
    cout_logger->sinks().push_back(sink);
    default_logger->sinks().push_back(sink);
}

static void removeSinkInternal(const std::shared_ptr<spdlog::sinks::sink> sink)
{
    {
        auto& sinks = default_logger->sinks();
        auto new_end = std::remove(sinks.begin(), sinks.end(), sink);
        sinks.erase(new_end, sinks.end());
    }
    {
        auto& sinks = cout_logger->sinks();
        auto new_end = std::remove(sinks.begin(), sinks.end(), sink);
        sinks.erase(new_end, sinks.end());
    }
}

void Logger::setLevel(Level level) {
    switch (level) {
    case Level::Off:
        spdlog::set_level(spdlog::level::off);
        break;
    case Level::Critical:
        spdlog::set_level(spdlog::level::critical);
        break;
    case Level::Error:
        spdlog::set_level(spdlog::level::err);
        break;
    case Level::Warn:
        spdlog::set_level(spdlog::level::warn);
        break;
    case Level::Info:
        spdlog::set_level(spdlog::level::info);
        break;
    case Level::Debug:
        spdlog::set_level(spdlog::level::debug);
        break;
    case Level::Trace:
        spdlog::set_level(spdlog::level::trace);
        break;
    default:
        OPENSIM_THROW(Exception, "Internal error.");
    }
    Logger::info("Set log level to {}.", getLevelString());
}

Logger::Level Logger::getLevel() {
    switch (default_logger->level()) {
    case spdlog::level::off: return Level::Off;
    case spdlog::level::critical: return Level::Critical;
    case spdlog::level::err: return Level::Error;
    case spdlog::level::warn: return Level::Warn;
    case spdlog::level::info: return Level::Info;
    case spdlog::level::debug: return Level::Debug;
    case spdlog::level::trace: return Level::Trace;
    default:
        OPENSIM_THROW(Exception, "Internal error.");
    }
}

void Logger::setLevelString(std::string str) {
    Level level;
    str = IO::Lowercase(str);
    if (str == "off") level = Level::Off;
    else if (str == "critical") level = Level::Critical;
    else if (str == "error") level = Level::Error;
    else if (str == "warn") level = Level::Warn;
    else if (str == "info") level = Level::Info;
    else if (str == "debug") level = Level::Debug;
    else if (str == "trace") level = Level::Trace;
    else {
        OPENSIM_THROW(Exception,
                "Expected log level to be Off, Critical, Error, "
                "Warn, Info, Debug, or Trace; got {}.",
                str);
    }
    setLevel(level);
}

std::string Logger::getLevelString() {
    const auto level = getLevel();
    switch (level) {
    case Level::Off: return "Off";
    case Level::Critical: return "Critical";
    case Level::Error: return "Error";
    case Level::Warn: return "Warn";
    case Level::Info: return "Info";
    case Level::Debug: return "Debug";
    case Level::Trace: return "Trace";
    default:
        OPENSIM_THROW(Exception, "Internal error.");
    }
}

bool Logger::shouldLog(Level level) {
    spdlog::level::level_enum spdlogLevel;
    switch (level) {
    case Level::Off: spdlogLevel = spdlog::level::off; break;
    case Level::Critical: spdlogLevel = spdlog::level::critical; break;
    case Level::Error: spdlogLevel = spdlog::level::err; break;
    case Level::Warn: spdlogLevel = spdlog::level::warn; break;
    case Level::Info: spdlogLevel = spdlog::level::info; break;
    case Level::Debug: spdlogLevel = spdlog::level::debug; break;
    case Level::Trace: spdlogLevel = spdlog::level::trace; break;
    default:
        OPENSIM_THROW(Exception, "Internal error.");
    }
    return default_logger->should_log(spdlogLevel);
}

void Logger::addFileSink(const std::string& filepath) {
    if (m_filesink) {
        default_logger->warn("Already logging to file '{}'; log file not added. Call "
             "removeFileSink() first.", m_filesink->filename());
        return;
    }

    // check if file can be opened at the specified path if not return meaningful
    // warning rather than bubble the exception up.
    try {
        m_filesink =
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath);
    }
    catch (...) {
        default_logger->warn("Can't open file '{}' for writing. Log file will not be created. "
             "Check that you have write permissions to the specified path.",
                filepath);
        return;
    }
    addSinkInternal(m_filesink);
}

void Logger::removeFileSink() {
    if (m_filesink == nullptr) {
        // the user called `removeFileSink` before any messages passed through
        // the logger (which would initialize it) and before calling
        // `addFileSink` themselves (which would also initialize it), so they
        // *probably* want to completely disable automatic initialization.
        filesink_auto_initialization_disabled = true;
        return;
    }

    removeSinkInternal(
            std::static_pointer_cast<spdlog::sinks::sink>(m_filesink));
    m_filesink.reset();
}

void Logger::addSink(const std::shared_ptr<LogSink> sink) {
    addSinkInternal(std::static_pointer_cast<spdlog::sinks::sink>(sink));
}

void Logger::removeSink(const std::shared_ptr<LogSink> sink) {
    removeSinkInternal(std::static_pointer_cast<spdlog::sinks::sink>(sink));
}



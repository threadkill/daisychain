// DaisyChain - a node-based dependency graph for file processing.
// Copyright (C) 2015  Stephen J. Parker
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "logger.h"

INITIALIZE_EASYLOGGINGPP // NOLINT(cert-err58-cpp)


void
configureLogger (el::Level level)
{
    el::Configurations defaultConf;
    defaultConf.setToDefault();

    el::Helpers::setThreadName ("main");
    // %datetime{%H:%m:%s.%g}
    defaultConf.setGlobally (el::ConfigurationType::MaxLogFileSize, "100000000");
    defaultConf.set (el::Level::Info, el::ConfigurationType::Format, "(%thread)[   INFO] - %msg");
    defaultConf.set (el::Level::Debug, el::ConfigurationType::Format, "(%thread)[  DEBUG] - %msg   {%fbase:%line}");
    defaultConf.set (el::Level::Error, el::ConfigurationType::Format, "(%thread)[  ERROR] - %msg");
    defaultConf.set (el::Level::Warning, el::ConfigurationType::Format, "(%thread)[WARNING] - %msg");
    el::Loggers::addFlag (el::LoggingFlag::ColoredTerminalOutput);
    el::Loggers::addFlag (el::LoggingFlag::HierarchicalLogging);
    el::Loggers::setLoggingLevel (level);
    //el::Loggers::reconfigureAllLoggers (defaultConf);
    el::Loggers::setDefaultConfigurations (defaultConf, true);

    el::Configurations perfConf;
    perfConf.set (el::Level::Info, el::ConfigurationType::Format, "(%thread)[   PERF] - %msg");
    el::Logger* perfLogger = el::Loggers::getLogger ("performance");
    el::Loggers::reconfigureLogger (perfLogger, perfConf);

    el::Configurations testConf;
    testConf.set (el::Level::Info, el::ConfigurationType::Format, "(%thread)[   TEST] - %msg");
    el::Logger* testLogger = el::Loggers::getLogger ("test");
    el::Loggers::reconfigureLogger (testLogger, testConf);
} // configureLogger

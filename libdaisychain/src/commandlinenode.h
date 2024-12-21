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

#pragma once

#include "node.h"


namespace daisychain {
class CommandLineNode final : public Node
{
public:
    CommandLineNode();

    explicit CommandLineNode (string);

    void Initialize (json&, bool) override;

    bool Execute (vector<string>& inputs, const string& sandbox, json& vars) override;

    json Serialize() override;

    void Reset() override {
        environment_.clear();
        Node::Reset();
    }

    void set_command (const string& cmd);

    string command();

private:
    bool run_command (const string&, const string&);

#ifdef _WIN32

    bool run_cmdexe (const string& command, string& output);

    void set_variable (const string& name, const string& value) { environment_[name] = value; };

    string get_variable (const string& name) { return environment_[name]; };

    string get_environment() {
        string results;

        LPCH lpEnvStrings = GetEnvironmentStrings();
        if (lpEnvStrings == nullptr) {
            return "";
        }

        std::vector<string> envars;

        for (LPCH env = lpEnvStrings; *env != '\0'; env += strlen(env) + 1) {
            envars.emplace_back (env, env + strlen(env));
        }

        FreeEnvironmentStrings (lpEnvStrings);

        auto env = m_parse_envars (envars);
        env.update (environment_);

        for (auto& [key, value] : env.items()) {
            results += key + "=" + value.get<string>() + '\0';
        }

        LDEBUG_IF (false) << env.dump (4);

        results += '\0'; // double nul terminated shenanigans.

        return results;
    };

#endif

    [[nodiscard]] string shell_expand (const string&);

    string command_;

    json environment_;
};
} // namespace daisychain

// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

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

    bool create_process (const string& command, string& output);

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

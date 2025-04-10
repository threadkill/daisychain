// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#include "graph.h"
#include <iostream>
#include <string>
#include <tclap/CmdLine.h>


using namespace daisychain;


int
main (int argc, const char* argv[])
{
    string graph_file;
    string sandbox;
    json environ_;
    bool nocleanup = false;
    bool use_stdinput = false;
    string loglevel;
    vector<string> input_files;

    try {
        TCLAP::CmdLine cmd (
            "DaisyChain - node-based dependency graph for file processing.", ' ', DAISYCHAIN_VERSION);
        TCLAP::ValueArg<string> graph_arg (
            "g", "graph", "DaisyChain graph file to execute.", true, "", "graph *.dcg", cmd);
        TCLAP::ValueArg<string> sandbox_arg (
            "s", "sandbox", "Working directory used for I/O and available as a shell"
                            " variable during execution ${SANDBOX}.", false, "",
            "sandbox directory", cmd);
        TCLAP::MultiArg<string> environ_arg (
            "e", "environ", "Shell variables inherited by the execution process.", false,
            "key=value", cmd);
        TCLAP::SwitchArg keep_arg ("", "keep", "keep sandbox", cmd, false);
        TCLAP::SwitchArg stdinput_arg ("", "stdin", "read from STDIN", cmd, false);
        TCLAP::ValueArg<string> loglevel_arg (
            "l", "loglevel", "off, info, warn, error, debug", false, "error", "level", cmd);
        TCLAP::UnlabeledMultiArg<string> inputs_arg (
            "inputs", "Inputs (typically files).", false, "filename", cmd);

        cmd.parse (argc, argv);

        graph_file = graph_arg.getValue();
        sandbox = sandbox_arg.getValue();
        environ_ = m_parse_envars (environ_arg.getValue());
        nocleanup = keep_arg.getValue();
        use_stdinput = stdinput_arg.getValue();
        loglevel = loglevel_arg.getValue();
        input_files = inputs_arg.getValue();
    }
    catch (TCLAP::ArgException& e) {
        std::cout << "error: " << e.error() << " for arg " << e.argId() << '\n';
    }

    configureLogger (loglevel);

    string stdinput;
    if (use_stdinput) {
        for (std::string line; std::getline (std::cin, line);) {
            stdinput += line;
        }
        stdinput += '\n';
    }
    stdinput.append (m_join (input_files, "\n"));

    std::cout << "DaisyChain " << DAISYCHAIN_VERSION << "\n";

    auto daisy_graph = Graph (graph_file);
    daisy_graph.set_sandbox (sandbox);
    daisy_graph.set_cleanup_flag (!nocleanup);

    bool stat = daisy_graph.Execute (stdinput, environ_);

    return !stat;
} // main

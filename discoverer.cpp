// Copyright 2023 Karma Krafts & associates
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * A simple command line application that is compiled on-demand
 * alongside EFITEST in a CMake project to provide simple parsing
 * of passed in sources files and filtering out a list of tests
 * to be compiled/ran by EFITEST in the including project.
 *
 * @author Alexander Hinze
 * @since 24/09/2023
 */

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <cxxopts.hpp>
#include <fmt/format.h>

struct Test {
    std::string name;
    size_t line_number = 0;
};

struct Target {
    std::filesystem::path source_path;
    std::filesystem::path header_path {};
    std::vector<Test> tests {};
};

using namespace std::string_literals;

static inline const std::string MACRO = "ETEST_DEFINE_TEST";
static inline const std::string INIT_FILE_NAME = "init.c";

template<typename... ARGS>
inline auto log(fmt::format_string<ARGS...> fmt, ARGS&&... args) noexcept -> void {
    fmt::println("-- {}", fmt::format(fmt, std::forward<ARGS>(args)...));
}

inline auto read_file(const std::filesystem::path& path) noexcept -> std::string {
    std::ifstream stream {path};
    std::string line {};
    std::string source {};
    while(std::getline(stream, line)) {
        source += line + '\n';
    }
    return source;
}

inline auto write_file(const std::filesystem::path& path) noexcept -> std::ofstream {
    std::ofstream stream {path};
    stream << "// ====================================\n";
    stream << "// GENERATED BY EFITEST - DO NOT MODIFY\n";
    stream << "// ====================================\n\n";
    return stream;
}

inline auto strip_extension(std::string& file_name) noexcept -> void {
    if(file_name.contains('.')) {
        auto begin = file_name.begin();
        auto end = file_name.end();
        auto current = file_name.begin();
        while(current != end && *current != '.') {
            ++current;
        }
        file_name = {begin, current};
    }
}

auto compute_function_name(const Target& target, const Test& test) noexcept -> std::string {
    auto prefix = target.source_path.filename().string();
    strip_extension(prefix);
    return fmt::format("__{}_{}", prefix, test.name);
}

auto compute_header_path(const std::filesystem::path& out_dir, Target& target) noexcept -> void {
    auto header_name = target.source_path.filename().string();
    strip_extension(header_name);
    header_name += ".h";
    target.header_path = out_dir / header_name;
}

template<typename I, typename F>
inline auto consume_until(I& current, I& end, F&& predicate) -> void {
    while(current != end && predicate(*current)) {
        ++current;
    }
    if(current == end) {
        throw std::runtime_error {"Unexpected EOF"};
    }
}

auto discover_tests(const std::filesystem::path& path) noexcept -> std::vector<Test> {// NOLINT
    auto source = read_file(path);
    std::vector<Test> tests {};
    auto current = source.begin();
    auto end = source.end();
    auto is_inline_comment = false;
    auto is_block_comment = false;

    while(current != end) {
        // Handle exiting block comment state
        if(is_block_comment) {
            const auto next = std::next(current);
            if(*current == '*' && next != end && *next == '/') {
                is_block_comment = false;
                ++current;
            }
            ++current;
            continue;
        }
        // Handle exiting inline comment state
        if(is_inline_comment) {
            if(*current == '\n') {
                is_inline_comment = false;
            }
            ++current;
            continue;
        }
        // Handle entering block- or inline comment state
        const auto next = std::next(current);
        if(*current == '/' && next != end) {
            switch(*next) {
                case '*':
                    is_block_comment = true;
                    current += 2;// Skip over /* to save some travel
                    continue;
                case '/':
                    is_inline_comment = true;
                    current += 2;// Skip over // to save some travel
                    continue;
            }
        }

        const std::string_view view {current, end};
        if(view.starts_with(MACRO)) {
            current += static_cast<ptrdiff_t>(MACRO.size());

            // clang-format off
            consume_until(current, end, [](auto x) { return x != '('; }); // NOLINT
            ++current;
            consume_until(current, end, [](auto x) { return x == ' '; }); // NOLINT
            const auto name_begin = current;
            consume_until(current, end, [](auto x) { return x == ' ' || x != ')'; }); // NOLINT
            // clang-format on

            const auto line_number = std::count(source.begin(), name_begin, '\n') + 1;
            std::string name {name_begin, current};
            log("Found test '{}' in {}", name, path.string());
            tests.emplace_back(std::move(name), line_number);
        }

        ++current;
    }

    for(auto& test : tests) {
        auto& name = test.name;
        while(name.contains('\n') || name.contains('\t')) {
            current = name.begin();
            end = name.end();
            while(current != end) {
                const auto current_char = *current;
                if(current_char == '\n' || current_char == '\t') {
                    name.erase(current);
                    break;
                }
                ++current;
            }
        }
    }

    return tests;
}

auto generate_target_header(const Target& target) noexcept -> void {
    const auto& path = target.header_path;
    if(std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }

    std::string source = "#pragma once\n\n";
    source += "#include <efitest/efitest.h>\n\n";

    const auto& tests = target.tests;
    for(const auto& test : tests) {
        source += fmt::format("void {}(EFITestContext* context);\n", compute_function_name(target, test));
    }

    write_file(path) << source;
}

auto inject_trampolines(const std::filesystem::path& out_dir, const std::vector<Target>& targets) noexcept -> void {
    for(const auto& target : targets) {
        const auto& source_path = target.source_path;
        const auto& tests = target.tests;
        const auto num_tests = tests.size();

        const auto out_path = out_dir / source_path.filename();
        auto stream = write_file(out_path);
        stream << read_file(source_path) << '\n';
        stream << "// ========== BEGIN INJECTED CODE ==========\n\n";
        stream << fmt::format("#include \"{}\"\n\n", target.header_path.filename().string());

        for(size_t index = 0; index < num_tests; ++index) {
            const auto& test = tests[index];
            const auto& test_name = test.name;
            auto function = fmt::format("void {}(EFITestContext* context) {{\n", compute_function_name(target, test));

            // Update context when trampoline is called
            function += fmt::format("\tcontext->test_name = \"{}\";\n", test_name);
            function += fmt::format("\tcontext->line_number = {};\n", test.line_number);
            function += fmt::format("\tcontext->group_index = {};\n", index);
            function += "\tcontext->failed = FALSE;\n";// Reset passed state

            // Add pre- and post-test hooks before and after bouncing the call
            function += "\tefitest_on_pre_run_test(context);\n";
            function += fmt::format("\t{}(context);\n", test_name);
            function += "\tefitest_on_post_run_test(context);\n";

            function += "}\n";
            stream << function << '\n';
        }
    }
}

auto generate_init_source(const std::filesystem::path& path, const std::vector<Target>& targets) noexcept -> void {
    if(std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }

    std::vector<std::string> includes {};
    std::string function = "void efitest_run_tests(EFITestContext* context) {\n";
    for(const auto& target : targets) {
        const auto& tests = target.tests;
        includes.push_back(fmt::format("#include \"{}\"", target.header_path.filename().string()));

        // Update per-target context information
        const auto& source_path = target.source_path;
        const auto file_name = source_path.filename().string();
        auto stripped_file_name = file_name;
        strip_extension(stripped_file_name);
        function += fmt::format("\tcontext->file_path = \"{}\";\n", source_path.string());
        function += fmt::format("\tcontext->file_name = \"{}\";\n", file_name);
        function += fmt::format("\tcontext->group_name = \"{}\";\n", stripped_file_name);
        function += fmt::format("\tcontext->group_size = {};\n", tests.size());

        function += "\tefitest_on_pre_run_group(context);\n";
        for(const auto& test : tests) {
            function += fmt::format("\t{}(context);\n", compute_function_name(target, test));
        }
        function += "\tefitest_on_post_run_group(context);\n";
    }
    function += '}';

    auto stream = write_file(path);
    for(const auto& include : includes) {
        stream << include << '\n';
    }
    stream << '\n';
    stream << function;
}

auto process_sources(const std::filesystem::path& out_dir, const std::vector<Target>& targets) noexcept -> void {
    if(!std::filesystem::exists(out_dir)) {
        std::filesystem::create_directories(out_dir);
    }

    for(const auto& target : targets) {
        generate_target_header(target);
    }

    generate_init_source(out_dir / INIT_FILE_NAME, targets);
    inject_trampolines(out_dir, targets);
}

auto main(int num_args, char** args) -> int {
    cxxopts::Options option_specs {"EFITEST Discoverer", "Test discovery service for the EFITEST framework"};
    // clang-format off
    option_specs.add_options()
            ("h,help", "Display a list of commands")
            ("v,version", "Display version information")
            ("o,out", "Specifies the path of the directory to generate sources into", cxxopts::value<std::string>())
            ("f,files", "Specifies the path to a file to scan for tests", cxxopts::value<std::vector<std::string>>());
    // clang-format on
    option_specs.parse_positional({"out", "files"});

    try {
        const auto options = option_specs.parse(num_args, args);

        if(options.count("help") > 0) {
            log("{}", option_specs.help());
            return 0;
        }

        if(options.count("version") > 0) {
            log("EFITEST Discoverer 1.0.0");
            return 0;
        }

        const auto& file_strings = options["files"].as<std::vector<std::string>>();
        std::vector<std::filesystem::path> files {};
        for(const auto& file_string : file_strings) {
            files.emplace_back(file_string);
        }

        const std::filesystem::path out_path {options["out"].as<std::string>()};
        std::vector<Target> targets {};

        const auto start_time = std::chrono::system_clock::now();

        for(const auto& file : files) {
            if(!std::filesystem::exists(file)) {
                log("File {} does not exist, skipping", file.string());
                continue;
            }

            Target target {file};
            compute_header_path(out_path, target);
            target.tests = discover_tests(file);

            targets.push_back(std::move(target));
        }

        const auto end_time = std::chrono::system_clock::now();
        const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        size_t num_tests = 0;
        for(const auto& target : targets) {
            num_tests += target.tests.size();
        }
        log("Discovered {} tests in {}ms", num_tests, time);

        process_sources(out_path, targets);
    }
    catch(...) {
        log("Could not parse arguments, try -h to get help");
        return 1;
    }

    return 0;
}
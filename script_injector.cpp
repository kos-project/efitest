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
 * @author Alexander Hinze
 * @since 26/09/2023
 */

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <cxxopts.hpp>
#include <fmt/format.h>

enum class MacroType : uint32_t {
    INCLUDE_DIRECTORIES,
    LINK_LIBRARIES,
    COMPILE_OPTIONS,
    COMPILE_DEFINITIONS,
    SET,
    UNSET
};

static inline const std::unordered_map<std::string, MacroType> FUNCTIONS = {
        {"efitest_include_directories", MacroType::INCLUDE_DIRECTORIES},
        {"efitest_link_libraries", MacroType::LINK_LIBRARIES},
        {"efitest_compile_options", MacroType::COMPILE_OPTIONS},
        {"efitest_compile_definitions", MacroType::COMPILE_DEFINITIONS},
        {"efitest_set", MacroType::SET},
        {"efitest_unset", MacroType::UNSET}};

static inline const std::unordered_map<MacroType, std::string> FUNCTION_NAMES = {
        {MacroType::INCLUDE_DIRECTORIES, "efitest_include_directories"},
        {MacroType::LINK_LIBRARIES, "efitest_link_libraries"},
        {MacroType::COMPILE_OPTIONS, "efitest_compile_options"},
        {MacroType::COMPILE_DEFINITIONS, "efitest_compile_definitions"},
        {MacroType::SET, "efitest_set"},
        {MacroType::UNSET, "efitest_unset"}};

static inline const std::unordered_map<MacroType, std::string> TRANSFORMED_FUNCTIONS = {
        {MacroType::INCLUDE_DIRECTORIES, "target_include_directories"},
        {MacroType::LINK_LIBRARIES, "target_link_libraries"},
        {MacroType::COMPILE_OPTIONS, "target_compile_options"},
        {MacroType::COMPILE_DEFINITIONS, "target_compile_definitions"},
        {MacroType::SET, "set"},
        {MacroType::UNSET, "unset"}};

// Type definitions
class Call {// NOLINT
    protected:
    // NOLINTBEGIN
    MacroType _type;
    // NOLINTEND
    explicit Call(MacroType type) noexcept :
            _type {type} {
    }

    public:
    virtual ~Call() noexcept = default;

    Call(const Call&) noexcept = delete;
    Call(Call&&) noexcept = delete;
    auto operator=(const Call&) noexcept -> Call& = delete;
    auto operator=(Call&&) noexcept -> Call& = delete;

    [[nodiscard]] virtual auto to_string() const noexcept -> std::string = 0;
    [[nodiscard]] virtual auto transform() const noexcept -> std::string = 0;

    [[nodiscard]] inline auto get_type() const noexcept -> MacroType {
        return _type;
    }
};

class VCall : public Call {
    protected:
    // NOLINTBEGIN
    std::vector<std::string> _variadic_args;
    // NOLINTEND
    public:
    explicit VCall(MacroType type, std::initializer_list<std::string> variadic_args = {}) noexcept :
            Call(type),
            _variadic_args {std::move(variadic_args)} {
    }

    ~VCall() noexcept override = default;

    VCall(const VCall&) noexcept = delete;
    VCall(VCall&&) noexcept = delete;
    auto operator=(const VCall&) noexcept -> VCall& = delete;
    auto operator=(VCall&&) noexcept -> VCall& = delete;

    [[nodiscard]] auto to_string() const noexcept -> std::string override {
        const auto fn_iter = FUNCTION_NAMES.find(_type);
        if(fn_iter == FUNCTION_NAMES.cend()) {
            return "";
        }
        const auto& function = fn_iter->second;
        if(_variadic_args.empty()) {
            return fmt::format("{}()", function);
        }

        auto result = fmt::format("{}(", function);
        for(auto iter = _variadic_args.cbegin(); iter < _variadic_args.cend(); ++iter) {
            result += *iter;
            if(iter != _variadic_args.cend() - 1) {
                result += ' ';
            }
        }
        result += ')';
        return result;
    }

    [[nodiscard]] auto transform() const noexcept -> std::string override {
        const auto fn_iter = TRANSFORMED_FUNCTIONS.find(_type);
        if(fn_iter == TRANSFORMED_FUNCTIONS.cend()) {
            return "";
        }
        const auto& function = fn_iter->second;
        if(_variadic_args.empty()) {
            return fmt::format("{}()", function);
        }

        auto result = fmt::format("{}(", function);
        for(auto iter = _variadic_args.cbegin(); iter < _variadic_args.cend(); ++iter) {
            result += *iter;
            if(iter != _variadic_args.cend() - 1) {
                result += ' ';
            }
        }
        result += ')';
        return result;
    }

    inline auto add_vararg(std::string value) noexcept -> void {
        _variadic_args.push_back(std::move(value));
    }

    [[nodiscard]] inline auto get_varargs() const noexcept -> const std::vector<std::string>& {
        return _variadic_args;
    }
};

class TargetedVCall : public VCall {
    protected:
    // NOLINTBEGIN
    std::string _target;
    std::string _access;
    // NOLINTEND

    public:
    explicit TargetedVCall(MacroType type, std::string target = {}, std::string access = {},
                           std::initializer_list<std::string> variadic_args = {}) noexcept :
            VCall(type, std::move(variadic_args)),
            _target {std::move(target)},
            _access {std::move(access)} {
    }
    ~TargetedVCall() noexcept override = default;

    TargetedVCall(const TargetedVCall&) noexcept = delete;
    TargetedVCall(TargetedVCall&&) noexcept = delete;
    auto operator=(const TargetedVCall&) noexcept -> TargetedVCall& = delete;
    auto operator=(TargetedVCall&&) noexcept -> TargetedVCall& = delete;

    inline auto set_target(const std::string& target) noexcept -> void {
        _target = target;
    }

    inline auto set_access(const std::string& access) noexcept -> void {
        _access = access;
    }

    [[nodiscard]] auto to_string() const noexcept -> std::string override {
        const auto fn_iter = FUNCTION_NAMES.find(_type);
        if(fn_iter == FUNCTION_NAMES.cend()) {
            return "";
        }
        const auto& function = fn_iter->second;
        if(_variadic_args.empty()) {
            return fmt::format("{}({} {})", function, _target, _access);
        }

        auto result = fmt::format("{}({} {} ", function, _target, _access);
        for(auto iter = _variadic_args.cbegin(); iter < _variadic_args.cend(); ++iter) {
            result += *iter;
            if(iter != _variadic_args.cend() - 1) {
                result += ' ';
            }
        }
        result += ')';
        return result;
    }

    [[nodiscard]] auto transform() const noexcept -> std::string override {
        const auto fn_iter = TRANSFORMED_FUNCTIONS.find(_type);
        if(fn_iter == TRANSFORMED_FUNCTIONS.cend()) {
            return "";
        }
        const auto& function = fn_iter->second;
        if(_variadic_args.empty()) {
            return fmt::format("{}({} {})", function, _target, _access);
        }

        auto result = fmt::format("{}({} {} ", function, _target, _access);
        for(auto iter = _variadic_args.cbegin(); iter < _variadic_args.cend(); ++iter) {
            result += *iter;
            if(iter != _variadic_args.cend() - 1) {
                result += ' ';
            }
        }
        result += ')';
        return result;
    }

    [[nodiscard]] inline auto get_target() const noexcept -> const std::string& {
        return _target;
    }

    [[nodiscard]] inline auto get_access() const noexcept -> const std::string& {
        return _access;
    }
};

[[nodiscard]] inline auto read_file(const std::filesystem::path& path) noexcept -> std::string {
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
    stream << "# ====================================\n";
    stream << "# GENERATED BY EFITEST - DO NOT MODIFY\n";
    stream << "# ====================================\n\n";
    return stream;
}

inline auto needs_transforming(std::string_view view) noexcept -> std::optional<std::pair<std::string, MacroType>> {
    for(const auto& function : FUNCTIONS) {
        if(!view.starts_with(function.first)) {
            continue;
        }
        return std::make_optional(function);
    }
    return std::nullopt;
}

template<typename I, typename F>
inline auto chomp(I& current, I& end, F&& predicate) noexcept -> typename I::value_type {
    while(current != end && !predicate(*current)) {
        ++current;
    }
    return *current;
}

// NOLINTBEGIN
// clang-format off
constexpr auto until_space = [](char x) -> bool { return x == ' '; };
constexpr auto until_no_space = [](char x) -> bool { return x != ' '; };
constexpr auto until_quote = [](char x) -> bool { return x == '"'; };
constexpr auto until_space_or_rparen = [](char x) -> bool { return x == ' ' || x == ')'; };
constexpr auto until_quote_or_rparen = [](char x) -> bool { return x == '"' || x == ')'; };
// clang-format on
// NOLINTEND

template<typename I, typename C>
inline auto parse_varargs(I& current, I& end, C& call) noexcept -> void {
    std::remove_reference_t<decltype(current)> begin;
    auto is_string = false;
    while(current != end && *current != ')') {
        chomp(current, end, until_no_space);// NOLINT
        begin = current;
        if(*current == '"') {
            is_string = true;
            ++current;
        }
        // clang-format off
        chomp(current, end, is_string ? until_quote_or_rparen : until_space_or_rparen);// NOLINT
        // clang-format on
        call->add_vararg({begin, is_string ? ++current : current});
        is_string = false;
    }
}

template<typename I>
inline auto parse_macro(MacroType type, I& current, I& end) noexcept -> std::unique_ptr<Call> {
    auto begin = current;

    auto is_string = false;
    const auto push_string_state = [&is_string, &current]() {
        if(*current == '"') {
            is_string = true;
            ++current;
        }
    };
    const auto pop_string_state = [&is_string, &current]() -> I& {
        if(is_string) {
            ++current;
            is_string = false;
        }
        return current;
    };

    switch(type) {// clang-format off
        case MacroType::INCLUDE_DIRECTORIES:
        case MacroType::LINK_LIBRARIES:
        case MacroType::COMPILE_OPTIONS:
        case MacroType::COMPILE_DEFINITIONS: {
            auto result = std::make_unique<TargetedVCall>(type);

            // Parse target
            push_string_state();
            chomp(current, end, is_string ? until_quote : until_space); // NOLINT
            result->set_target({begin, pop_string_state()});

            // Parse access
            chomp(current, end, until_no_space); // NOLINT
            begin = current;
            push_string_state();
            chomp(current, end, is_string ? until_quote : until_space); // NOLINT
            result->set_access({begin, pop_string_state()});

            // Parse varargs
            parse_varargs(current, end, result);
            return result;
        }
        case MacroType::SET: {
            auto result = std::make_unique<VCall>(type);
            // Parse name
            push_string_state();
            chomp(current, end, is_string ? until_quote : until_space); // NOLINT
            result->add_vararg({begin, pop_string_state()});

            // Parse varargs
            parse_varargs(current, end, result);
            return result;
        }
        case MacroType::UNSET: {
            auto result = std::make_unique<VCall>(type);
            // Parse name
            push_string_state();
            chomp(current, end, is_string ? until_quote : until_space); // NOLINT
            result->add_vararg({begin, pop_string_state()});
            return result;
        }
        default:
            return nullptr;
    }// clang-format on
}

auto inject_dependencies(const std::filesystem::path& source_path, const std::filesystem::path& in_path,
                         const std::filesystem::path& out_path) noexcept -> size_t {
    const auto source = read_file(source_path);
    auto current = source.begin();
    auto end = source.end();
    auto is_inline_comment = false;
    std::string injection {};
    size_t count = 0;

    while(current != end) {
        // Handle exiting inline comment state
        if(is_inline_comment) {
            if(*current == '\n') {
                is_inline_comment = false;
            }
            ++current;
            continue;
        }
        // Handle entering block- or inline comment state
        if(*current == '#') {
            is_inline_comment = true;
            ++current;
            continue;
        }
        // Find occurrences of macros
        const std::string_view view {current, end};
        if(auto result = needs_transforming(view); result) {
            const auto& function = result->first;
            fmt::println("Transforming macro of type '{}'", function);
            current += static_cast<ptrdiff_t>(function.size() + 1);
            const auto invocation = parse_macro(result->second, current, end);
            if(!invocation) {
                goto end;// NOLINT
            }
            injection += fmt::format("{}\n", invocation->transform());
            ++count;
            continue;
        }
        ++current;
    end:
    }

    auto stream = write_file(out_path) << read_file(in_path) << '\n';
    stream << "# ========== BEGIN INJECTED CODE ==========\n\n";
    stream << injection;

    return count;
}

auto main(int num_args, char** args) -> int {
    cxxopts::Options option_specs {"EFITEST Script Injector", "A utility for injecting dependencies into a sub-build"};
    // clang-format off
    option_specs.add_options()
            ("h,help", "Display a list of commands")
            ("v,version", "Display version information")
            ("s,source", "Specify the path to the CMakeLists file from which to extract dependencies", cxxopts::value<std::string>())
            ("i,in", "Specify the path to the input file", cxxopts::value<std::string>())
            ("o,out", "Specify the path to the output file", cxxopts::value<std::string>());
    // clang-format on
    option_specs.parse_positional({"in", "out"});

    try {
        const auto options = option_specs.parse(num_args, args);

        if(options.count("help") > 0) {
            fmt::println("{}", option_specs.help());
            return 0;
        }

        if(options.count("version") > 0) {
            fmt::println("EFITEST Script Injector 1.0.0");
            return 0;
        }

        const std::filesystem::path source_path {options["source"].as<std::string>()};
        const std::filesystem::path in_path {options["in"].as<std::string>()};
        const std::filesystem::path out_path {options["out"].as<std::string>()};

        if(!std::filesystem::exists(source_path)) {
            fmt::println("Source file {} does not exist", source_path.string());
            return 1;
        }
        if(!std::filesystem::exists(in_path)) {
            fmt::println("Input file {} does not exist", in_path.string());
            return 1;
        }

        const auto start_time = std::chrono::system_clock::now();
        const auto count = inject_dependencies(source_path, in_path, out_path);
        const auto time =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time)
                        .count();
        fmt::println("Injected {} entries in {}ms", count, time);
    }
    catch(...) {
        fmt::println("Could not parse arguments, try -h to get help");
        return 1;
    }

    return 0;
}
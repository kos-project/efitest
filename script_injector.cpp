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

#include <fmt/format.h>
#include <cxxopts.hpp>

auto main(int num_args, char** args) -> int {
    cxxopts::Options options{"EFITEST Script Injector", "A utility for injecting dependencies into a sub-build"};
    // clang-format off
    options.add_options()
            ("v,version", "Display version information")
            ("i,in", "Specify the path to the input file")
            ("o,out", "Specify the path to the output file");
    // clang-format on
}
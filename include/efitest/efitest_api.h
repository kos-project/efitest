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
 * @since 24/09/2023
 */

#pragma once

#define ETEST_INLINE __attribute__((always_inline))

#ifdef __cplusplus
#define ETEST_API_BEGIN extern "C" {
#define ETEST_API_END }
#else
#define ETEST_API_BEGIN
#define ETEST_API_END
#endif//__cplusplus

ETEST_API_BEGIN

#include <efi.h>
#include <efilib.h>

ETEST_API_END
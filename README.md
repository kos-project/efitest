# efitest
C framework (C++ compatible) for writing freestanding UEFI unit tests
using custom CMake modules & source generation.

### Code Example
```c
#include <efitest/efitest.h>

ETEST_DEFINE_TEST(hello_world_test) {
    ETEST_ASSERT(2 == (1 << 1));
    efitest_log(L"Hello World from EFITEST!");
}
```

### Building
In order to build EFITEST, you only need a compatible C compiler which supports C23. No standard library is required at all
apart from the headers provided by GNU-EFI.  
When you have cloned the repository, you can configure the project by running the following command:

```shell
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DETEST_BUILD_TESTS=ON
```

When the project is configured successfully, you can build the library and the associated unit tests by running the following command:

```shell
cmake --build cmake-build-debug [--target efitest-tests]
```

You can leave out the --target flag if you only need the library itself and not its test(s).

### Screenshots
EFITEST running in QEMU   
![image](https://github.com/kos-project/libefitest/assets/12082168/80ccde1c-5491-4451-b8c3-67b94f58f772)

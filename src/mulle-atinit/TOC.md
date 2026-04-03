# mulle-atinit Library Documentation for AI
<!-- Keywords: lifecycle, initialization -->

## 1. Introduction & Purpose

`mulle-atinit` is a C library that provides a deterministic, priority-based mechanism for running initializers, primarily for shared libraries. On many platforms (e.g., ELF-based systems like Linux), the execution order of constructor functions in different shared libraries is not guaranteed. This can lead to crashes if one library's initializer depends on another library that has not yet been initialized.

`mulle-atinit` solves this problem by providing a centralized registry where libraries can defer their initialization functions. These functions are then executed in a predictable order based on assigned priorities before `main` is called. This library is intended to be statically linked into the main executable to act as the single coordinator for all library initializations.

## 2. Key Concepts & Design Philosophy

- **Deferred Initialization:** Instead of executing code directly in a `__attribute__((constructor))`, a shared library calls `mulle_atinit` to register an initialization function. The actual execution is deferred.
- **Centralized Execution:** A constructor in the `mulle-atinit` library itself is responsible for collecting all registered initializers and running them. Because `mulle-atinit` is linked into the main executable, its constructor runs after all shared libraries have been loaded but before `main` begins.
- **Priority System:** Each registered initializer has a priority. Initializers are executed in ascending order of their priority value, allowing for explicit control over the initialization sequence. Libraries with lower priority numbers are initialized first.
- **Static Linking Requirement:** For the system to work, `mulle-atinit` must be statically linked into the final executable, and linker flags must be used to ensure its symbols are exported and the entire library is included. This guarantees there is only one central registry for initializers.

## 3. Core API & Data Structures

The API is minimal and is fully defined in `mulle-atinit.h`. It does not expose any public data structures.

### 3.1. `mulle-atinit.h`

#### Initializer Registration
- `mulle_atinit(function, userinfo, priority, comment)`: Registers an initializer function to be called before `main`.
  - `function`: A function pointer of type `void (*)(void *)` that will be executed.
  - `userinfo`: A `void *` pointer that will be passed as the argument to the `function`.
  - `priority`: An `int` that determines the execution order. Lower numbers execute first.
  - `comment`: A `char *` pointer for an optional comment/description of the initializer. Can be `NULL`.

#### Global Control
- `mulle_atinit_is_global(void)`: A weakly-linked function that allows libraries to detect if a global `mulle-atinit` instance is present in the main executable. This helps avoid conflicts if multiple versions are present.

## 4. Performance Characteristics

- **Registration:** Registering an initializer is a fast, thread-safe operation. It involves a mutex lock and an allocation to store the registration info.
- **Execution:** At application startup, there is a one-time cost to sort and execute the registered initializers. The time is proportional to `N * log(N)` for sorting (where N is the number of initializers) plus the execution time of the initializers themselves.
- **Runtime Overhead:** After the initial startup phase, the library has zero runtime overhead.
- **Thread-Safety:** The registration process is thread-safe.

## 5. AI Usage Recommendations & Patterns

- **Linking:** It is **critical** that `mulle-atinit` is statically linked into the main executable. Linker flags must be used to ensure the entire library is included and its symbols are exported.
  - **Linux:** `-Wl,--whole-archive -lmulle-atinit -Wl,--no-whole-archive -Wl,--export-dynamic`
  - **macOS:** `-Wl,-force_load,path/to/libmulle-atinit.a`
- **Shared Library Usage:** In a shared library that needs controlled initialization, create a standard constructor function. Inside this constructor, call `mulle_atinit` to register the *actual* initialization function.

  ```c
  #include <mulle-atinit/mulle-atinit.h>

  static void my_library_init(void *userinfo) {
      // Actual initialization code here...
  }

  __attribute__((constructor))
  static void register_initializer(void) {
      // Defer the real initialization with a specific priority.
      mulle_atinit(my_library_init, NULL, 100, NULL);
  }
  ```
- **Priority Management:** Establish a clear convention for priority numbers across your project's libraries. For example:
  - `0-99`: Core libraries with no dependencies.
  - `100-199`: Mid-level libraries.
  - `200+`: High-level or plugin libraries.
- **Main Executable:** The main executable itself does not need to do anything special other than linking correctly. The `mulle-atinit` constructor handles everything automatically before `main` is called.

## 6. Integration Examples

### Example 1: Initializing Libraries in a Specific Order

This example simulates three shared libraries (X, Y, Z) that need to be initialized in the order Z -> Y -> X. Library Z has the lowest priority, so it runs first.
*Source: `test/20_dynamic/`*

**Library Z (`z.c`)**
```c
#include <mulle-atinit/mulle-atinit.h>
#include <stdio.h>

static void init_z(void *userinfo) {
    printf("Initializing Z\n");
}

__attribute__((constructor))
static void register_init(void) {
    mulle_atinit(init_z, NULL, 100, NULL); // Priority 100
}
```

**Library Y (`y.c`)**
```c
#include <mulle-atinit/mulle-atinit.h>
#include <stdio.h>

static void init_y(void *userinfo) {
    printf("Initializing Y\n");
}

__attribute__((constructor))
static void register_init(void) {
    mulle_atinit(init_y, NULL, 200, NULL); // Priority 200
}
```

**Library X (`x.c`)**
```c
#include <mulle-atinit/mulle-atinit.h>
#include <stdio.h>

static void init_x(void *userinfo) {
    printf("Initializing X\n");
}

__attribute__((constructor))
static void register_init(void) {
    mulle_atinit(init_x, NULL, 300, NULL); // Priority 300
}
```

**Main Executable (`main.c`)**
```c
#include <stdio.h>

int main(void) {
    printf("main() called\n");
    return 0;
}
```

When compiled and run (with `libx`, `liby`, `libz` linked dynamically and `libmulle-atinit` linked statically to `main`), the output will be:

```
Initializing Z
Initializing Y
Initializing X
main() called
```

This demonstrates that the initializers were executed in the correct, priority-based order before `main` was entered.

## 7. Dependencies

- `mulle-thread`
- `mulle-dlfcn`


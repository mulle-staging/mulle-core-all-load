# mulle-atinit Library Documentation for AI
<!-- Keywords: init, callbacks, priority, constructor, mergesort, thread, C -->

## 1. Introduction & Purpose

- mulle-atinit provides a simple, deterministic mechanism to register and run "init" callbacks with explicit priorities. It ensures callbacks added across translation units and shared libraries are invoked in a stable, prioritized order at program/library startup (constructor phase), before `main()`.
- Solves ordering and portability problems with ELF shared-library constructors and static initializers, whose sequencing across separate shared objects is unreliable. It supplies a deterministic, sorted execution in the executable's own constructor.
- Key features: priority-based registration, stable mergesort at run time (equal priorities keep insertion order), thread-safe registration, test hooks (built with `MULLE_TEST`), and optional symbol lookup via `dlsym` for Windows dynamic builds.
- A part of `mulle-core`. Depends on `mulle-thread` (mutex/once) and `mulle-dlfcn` (dynamic symbol lookup). It must be **statically linked** into the executable.
- Current version: `MULLE__ATINIT_VERSION` is `0.3.2`.

## 2. Key Concepts & Design Philosophy

- **Registration vs. Execution:** Callers register a callback (function + `userinfo` + `priority` + optional `comment`) using `mulle_atinit(...)` from any translation unit. Execution happens later, in a single constructor run, which sorts all registered callbacks by priority and invokes them in descending priority order.
- **Priority semantics:** A **higher (larger) numeric priority runs earlier**. Priority `0` is the normal case; use positive priorities to run earlier, negative priorities to run later. Callbacks with equal priority preserve insertion (registration) order — the sort is stable.
- **Stability:** `_prioritized_callback_mergesort` is a stable mergesort; an equality check uses `<` (not `<=`) so equal priorities keep their relative order.
- **Thread-safety:** A mutex (`vars.lock`) protects the callback list. One-time initialization uses a thread-once mechanism. Callbacks that are added while callbacks are already running are appended and executed in the same run (the run loop is re-entered); they are not re-prioritized.
- **Late registration:** Once callbacks have already run, a newly registered callback is executed **immediately** (during the call), regardless of priority. In `DEBUG` builds a warning is printed if such a late callback has a non-zero priority.
- **Point of no return (running):** After the run, `vars.calls` is freed and set to `NULL`, `vars.n` is set to `0`, and `vars.size` is set to an impossible value (`-1`, i.e. `UINT_MAX`) as a marker that the callbacks already ran.
- **Minimal runtime:** A lightweight array of `struct prioritized_callback` grows via `realloc` (doubling, starting at 32). Sorting is deferred to execution time so registration stays fast.
- **Static linking requirement:** The library contains `#error "You can't make mulle-atinit part of a shared library"` when `MULLE_INCLUDE_DYNAMIC` is defined, because the `_mulle_atinit` symbol must resolve in the executable's "load" domain.

## 3. Core API & Data Structures

The public API lives in one header, `src/mulle-atinit.h`. The whole library is C (no Objective-C).

### 3.1. `mulle-atinit.h`

#### Version helpers
- `#define MULLE__ATINIT_VERSION  ((0UL << 20) | (3 << 8) | 2)` — packed version (major.minor.patch) `0.3.2`.

```
static inline unsigned int   mulle_atinit_get_version_major( void)
static inline unsigned int   mulle_atinit_get_version_minor( void)
static inline unsigned int   mulle_atinit_get_version_patch( void)
```
- `mulle_atinit_get_version_major( void)` — returns major number of the version.
- `mulle_atinit_get_version_minor( void)` — returns minor number of the version.
- `mulle_atinit_get_version_patch( void)` — returns patch number of the version.

```
MULLE__ATINIT_GLOBAL
uint32_t   mulle_atinit_get_version( void);
```
- `mulle_atinit_get_version( void)` — returns the packed version value `MULLE__ATINIT_VERSION`. `MULLE__ATINIT_GLOBAL` is `extern` (the library is always statically linked).

#### Registration function type

```
typedef void   mulle_atinit_function_t( void (*f)( void *),
                                        void *userinfo,
                                        int priority,
                                        char *comment);
```
- Function type of the registration function. Exposed so that on Windows (with `MULLE_INCLUDE_DYNAMIC`) the `_mulle_atinit` symbol can be resolved via `mulle_dlsym_exe`.

#### Core registration

```
MULLE__ATINIT_GLOBAL
void   _mulle_atinit( void (*f)( void *),
                      void *userinfo,
                      int priority,
                      char *comment);
```
- The real registration function. Appends the callback to the internal list in a thread-safe way (mutex-protected). If `f` is `NULL`, it only ensures the mutex is initialized and returns.
- If callbacks have already run, the new callback is invoked **immediately** (see "Late registration" in Section 2).

```
static inline void   mulle_atinit( void (*f)( void *),
                                   void *userinfo,
                                   int priority,
                                   char *comment)
```
- Preferred public API. Inline wrapper that calls `_mulle_atinit` directly; on Windows with `MULLE_INCLUDE_DYNAMIC` it resolves `_mulle_atinit` via `mulle_dlsym_exe` first and prints an error to `stderr` if it is not yet available.
- Register callbacks from anywhere: `mulle_atinit( f, userinfo, priority, NULL)`.

### 3.2. Test-only hooks (in `src/mulle-atinit.c`, only when built with `MULLE_TEST`)

These functions are **not** declared in the public header; tests declare them manually with `extern`.

- `void mulle_atinit_test_run_callbacks( void );` — runs the callback loop manually without waiting for the constructor (used by tests, e.g. `test/10-static/prios.c`).
- `void mulle_atinit_reset( void );` — resets internal state (`vars.size = 0`) so a fresh register/run cycle can be performed in a deterministic test.

### 3.3. Internal structures and functions (in `src/mulle-atinit.c`, not part of the public API)

#### `struct prioritized_callback`
- Fields: `int priority; void (*f)( void *); void *userinfo; char *comment;`
- Purpose: internal representation of one registered callback.

#### Module-scope static `vars`
- `mulle_thread_mutex_t lock; unsigned int n; unsigned int size; struct prioritized_callback *calls;`
- Lifecycle: initialized once via thread-once; `mulle_atinit_load` (constructor) runs the callbacks at startup.

#### Helper functions
- `_prioritized_callback_mergesort( struct prioritized_callback *array, size_t size)` — stable mergesort over priorities (O(n log n)); uses `malloc`/`free` for a temporary array.
- `mulle_atinit_add_callback( void (*f)( void *), void *userinfo, int priority, char *comment)` — low-level append with amortized O(1) `realloc` growth (doubling, initial capacity 32, `abort()` on OOM). Not declared in the public header, but present unconditionally.
- `mulle_atinit_run_callbacks( void )` — static; sorts the list under the mutex, then pops callbacks from the **end** of the sorted array (so highest priority runs first). Re-enters the loop if new callbacks were added during execution.
- `mulle_atinit_load( void )` — registered as an ELF constructor via `MULLE_C_CONSTRUCTOR( mulle_atinit_load)`. Calls `_mulle_atinit( 0, 0, 0, NULL)` (ensures "once" init and prevents linker dead-code elimination) and then `mulle_atinit_run_callbacks()`. Runs before `main()`.
- `mulle_atinit_trace( char *format, ... )` — trace output gated by the `MULLE_ATINIT_TRACE` environment variable.

## 4. Performance Characteristics

- **Registration:** Amortized O(1) appends; occasional O(n) copy on `realloc` growth (doubling from an initial capacity of 32).
- **Execution:** One-time O(n log n) stable mergesort plus O(n) invocations. Sorting happens only at run time (constructor), not at registration.
- **Memory:** Array-backed storage; the backing array is `free`d after the run. Trade-off: fast registration vs. a deferred sort at startup.
- **Thread-safety:** Registration and execution are mutex-protected; one-time init uses a thread-once. Callbacks run sequentially on the constructor thread. Not lock-free.
- The keys/values stored are only the `userinfo` pointer; no ownership transfer, no copying, no retention.

## 5. AI Usage Recommendations & Patterns

- **Best Practices:**
  - Use the public `mulle_atinit( f, userinfo, priority, comment)` for all registrations; do not call internal functions directly.
  - Use priority `0` normally. Use positive priorities to move an initializer ahead of (and negative priorities after) the pack. Equal priorities preserve registration order within the same run.
  - Always statically link the library into the executable (`-Wl,--export-dynamic -Wl,--whole-archive` on Linux, `-force_load` on macOS), and keep global symbols exported so shared-library consumers can resolve `_mulle_atinit`.
  - For deterministic unit tests, build with `MULLE_TEST` and use `mulle_atinit_test_run_callbacks()`/`mulle_atinit_reset()`, declaring them `extern` (as in `test/10-static/prios.c`).
- **Common Pitfalls:**
  - `mulle_atinit_add_callback`, `mulle_atinit_test_run_callbacks`, and `mulle_atinit_reset` are **not** in the public header — declare them manually if you use them (tests do).
  - Do not rely on registration order across different translation units to imply execution order; priorities (not insertion order) determine ordering unless priorities are equal.
  - A callback registered after the constructor ran is executed **immediately** inside the call, not deferred — do not register long-running or unsafe code late.
  - Do not `free` `userinfo` inside a callback unless you know it is still owned by the caller; `userinfo` is a borrowed pointer (like a C `void *` context argument).
  - Do not make mulle-atinit part of a shared library; `MULLE_INCLUDE_DYNAMIC` is forbidden via `#error`.
  - On systems without `dlsym` (e.g. musl static linking), define `__MULLE_STATICALLY_LINKED__` when building consumers.
  - Keep callbacks short: they execute in the executable's own constructor, before `main()`.

## 6. Integration Examples

Examples follow the library style: 3-space indent, Allman braces, aligned declarations, `return( expr);`.

### Example 1: Simple registration (priority 0, insertion order preserved)

```c
#include <mulle-atinit/mulle-atinit.h>
#include <stdio.h>

static void   a( void *s)
{
   printf( "%s: \"%s\"\n", __FUNCTION__, (char *) s);
}

static void   b( void *s)
{
   printf( "%s: \"%s\"\n", __FUNCTION__, (char *) s);
}

int  main( void)
{
   mulle_atinit( a, "first", 0, NULL);
   mulle_atinit( b, "mid", 0, NULL);

   /* callbacks run in the library constructor, before main() resumes;
      equal priorities execute in registration order: a then b */
   return( 0);
}
```

### Example 2: Prioritized callbacks (test pattern from `test/10-static/prios.c`)

```c
#define _GNU_SOURCE

#include <mulle-atinit/mulle-atinit.h>
#include <stdio.h>

/* test-only/internal symbols are not in the public header;
   declare them manually like the shipped tests do */
void   mulle_atinit_reset( void);
void   mulle_atinit_add_callback( void (*f)( void *),
                                  void *userinfo,
                                  int priority,
                                  char *comment);
void   mulle_atinit_test_run_callbacks( void);

static void   print( void *s)
{
   printf( "%s\n", (char *) s);
}

int  main( void)
{
   mulle_atinit_reset();

   /* higher priority runs earlier; equal priorities keep insertion order */
   mulle_atinit_add_callback( print, "1", 100, NULL);
   mulle_atinit_add_callback( print, "5",   0, NULL);
   mulle_atinit_add_callback( print, "2", 100, NULL);
   mulle_atinit_add_callback( print, "4",  50, NULL);
   mulle_atinit_add_callback( print, "3", 100, NULL);

   /* expected output: 1 2 3 4 5 */
   mulle_atinit_test_run_callbacks();

   return( 0);
}
```

### Example 3: Cross-tu constructor usage (pattern from `test/20-dynamic`)

Each shared library registers its own callback in its constructor; ordering across libraries is resolved by priority in the executable's constructor.

```c
#include <mulle-atinit/mulle-atinit.h>
#include <stdio.h>

MULLE_C_GLOBAL
void   x( void *s)
{
   printf( "%s: \"%s\"\n", __FUNCTION__, (char *) s);
   fflush( stdout);
}

MULLE_C_CONSTRUCTOR( load)
static void   load( void)
{
   mulle_atinit( x, "first", 0, NULL);   /* runs before priority -1 and -2 */
}
```

## 7. Dependencies

- `mulle-concurrent/mulle-thread` — mutex and thread-once for thread-safe registration and one-time init.
- `mulle-core/mulle-dlfcn` — `mulle_dlsym_exe` for optional dynamic symbol lookup on Windows (`_WIN32` + `MULLE_INCLUDE_DYNAMIC`).

## 8. Shortcut

- This file was last modified in commit `b774e10` (2026-08-04, "maintenance: add copyright license headers to source files"), with earlier content from `a0b2cf6`, `3bc9304` (2026-04-10). Since that commit only two maintenance commits followed (`9b188cb`, `6a7b1c8`): build/tooling updates and a version bump of `MULLE__ATINIT_VERSION` from `0.3.1` to `0.3.2` in `src/mulle-atinit.h`. No public API signatures changed; this file was refreshed to fix the late-registration semantics description and make Example 2 compilable (manual declarations as in `test/10-static/prios.c`).
- Source of truth for the public API: `src/mulle-atinit.h`. Working examples: `test/10-static/`, `test/20-dynamic/`, `test/30-mergesort/`.
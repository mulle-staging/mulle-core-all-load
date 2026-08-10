# mulle-atinit Library Documentation for AI
<!-- Keywords: init, callbacks, priority, constructor, mergesort, thread, C -->

## 1. Introduction & Purpose

- mulle-atinit provides a simple, deterministic mechanism to register and run "init" callbacks with explicit priorities. It ensures callbacks added across translation units are invoked in a stable, prioritized order at program/library startup (constructor phase).
- Solves ordering and portability issues for ELF constructors and static initializers, and supplies a deterministic fallback for platforms/links where constructor ordering is unreliable.
- Key features: priority-based registration, stable sort at run-time, thread-safe registration, test hooks.
- Depends on mulle-thread (mutex/once) and mulle-dlfcn for optional dynamic lookup on Windows.

## 2. Key Concepts & Design Philosophy

- Registration vs. Execution: callers register callbacks (function + userinfo + priority + comment); execution occurs later in a single run that sorts callbacks by priority and invokes them.
- Stability: uses a stable mergesort so equal priorities preserve insertion order.
- Thread-safety: a mutex protects the callback list; initialization uses a thread-once mechanism. Callbacks added while callbacks are running are appended and executed in the same run (no reprioritization).
- Minimal runtime: lightweight data structure (array of prioritized_callback) with realloc growth; sorting deferred to execution time to keep registration fast.

## 3. Core API & Data Structures

### 3.1. [mulle-atinit.h]

#### `typedef void mulle_atinit_function_t( void (*f)( void *), void *userinfo, int priority, char *comment );`
- Purpose: function type for the registration function (exposed for dynamic lookup on Windows).

#### `uint32_t mulle_atinit_get_version( void );`
- Returns a packed version number. Inline helpers exist for major/minor/patch.

#### `void _mulle_atinit( void (*f)( void *), void *userinfo, int priority, char *comment );`
- Purpose: Core registration function that appends the callback to the internal list in a thread-safe way.
- Behavior: If executed after callbacks already ran, zero-priority callbacks may be executed immediately to preserve semantics for late registration.

#### `static inline void mulle_atinit( void (*f)( void *), void *userinfo, int priority, char *comment );`
- Purpose: Inline wrapper that forwards to _mulle_atinit. On Windows with dynamic builds it resolves symbol via dlsym-like lookup.
- Usage: Preferred public API for registering callbacks.

#### Test-only hooks (available when built with MULLE_TEST):
- `void mulle_atinit_test_run_callbacks( void );` — run callbacks manually (used by tests).
- `void mulle_atinit_reset( void );` — reset internal state for deterministic tests.

### 3.2. [internal data structures in mulle-atinit.c]

#### `struct prioritized_callback`
- Fields: int priority; void (*f)(void *); void *userinfo; char *comment;
- Purpose: internal representation of a single registered callback.

#### Internal module-scope `vars` (static)
- Contains a mutex, current count `n`, allocated `size`, and pointer to array `calls`.
- Lifecycle: initialized via thread-once; `mulle_atinit_load` (constructor) triggers a run which sorts and executes callbacks.

#### Sorting & Execution functions
- `_prioritized_callback_mergesort( ...)` — stable mergesort; O(n log n) on execution.
- `mulle_atinit_add_callback(...)` — low-level append with amortized O(1) realloc growth.
- `mulle_atinit_run_callbacks()` — sorts the list and invokes callbacks in priority order.
- `mulle_atinit_load()` — constructor that ensures _mulle_atinit is called once and then runs callbacks at startup.

## 4. Performance Characteristics

- Registration: amortized O(1) for appends; occasional O(n) when realloc grows.
- Execution: sorting cost O(n log n) (stable mergesort) plus O(n) to invoke callbacks.
- Memory: array-backed storage that grows (initial capacity ~32). Trade-off: fast registration vs. deferred sort overhead.
- Thread-safety: registration and execution use a mutex; initialization uses thread-once. Not lock-free.

## 5. AI Usage Recommendations & Patterns

- Best Practices:
  - Use `mulle_atinit( func, userinfo, priority, "comment")` to register callbacks.
  - Prefer non-zero priorities to control ordering; equal priorities keep insertion order.
  - Do not access internal fields of callbacks; use API only.
  - For unit tests, build with `MULLE_TEST` to use `mulle_atinit_test_run_callbacks()` and `mulle_atinit_reset()`.
- Common Pitfalls:
  - Do not expect registration order alone to determine execution order if priorities differ.
  - Avoid long-running or blocking operations inside callbacks; they run during startup.
  - Dynamic/shared library builds are intentionally restricted; mulle-atinit is intended to be statically linked (MULLE_INCLUDE_DYNAMIC is not supported).
- Idiomatic Usage: register simple init routines that set up library-wide state, register destructors elsewhere if needed.

## 6. Integration Examples

### Example 1: Simple registration (preserve insertion order)

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

int
main( void)
{
   mulle_atinit( a, "first", 0, NULL);
   mulle_atinit( b, "second", 0, NULL);
   return( 0);
}
```

- Build note: This registers callbacks to be invoked by the library constructor; linking statically ensures the constructor runs.

### Example 2: Prioritized callbacks

```c
#include <mulle-atinit/mulle-atinit.h>
#include <stdio.h>

static void   print( void *s)
{
   printf( "%s\n", (char *) s);
}

int
main( void)
{
   /* higher priority runs earlier (larger numeric value executes first) */
   mulle_atinit_add_callback( print, "1", 100, NULL);
   mulle_atinit_add_callback( print, "5",   0, NULL);
   mulle_atinit_add_callback( print, "2", 100, NULL);

   /* For tests, run manually (only with MULLE_TEST build): */
   /* mulle_atinit_test_run_callbacks(); */

   return( 0);
}
```

- The provided tests (test/10-static) show how priorities and stability behave.

## 7. Dependencies

- mulle-concurrent/mulle-thread (mutex, thread-once)
- mulle-core/mulle-dlfcn (dynamic symbol lookup on some platforms)

## 8. Shortcut

- There was no prior TOC.md in asset/dox/ (this file is added to that path). The source of truth for the public API is `src/mulle-atinit.h`; tests live in `test/` and demonstrate usage patterns.





# `dlsym` test and exported symbols

The `dlsym.c` test defines a function `find_me` in the test executable and then
looks it up at runtime:

``` c
adr = dlsym( MULLE_RTLD_DEFAULT, "find_me");
```

For this to succeed, `find_me` must be present in the executable's **dynamic
symbol table**. This is where Linux (ELF) and macOS (Mach-O) differ, and it was
the cause of a macOS-only test failure (`"find_me" not found`).

## The platform difference

`dlsym( RTLD_DEFAULT, ...)` can only resolve symbols that the linker placed in
the dynamic symbol table. A symbol that is merely defined in the executable is
not automatically visible there.

- **Linux / ELF:** the linker is told to export the executable's own symbols
  with `-Wl,--export-dynamic`. `find_me` ends up in the dynamic table and the
  lookup succeeds.
- **macOS / Mach-O:** by default executable symbols are *not* placed in the
  dynamic table. Without an explicit request `find_me` stays private
  (`nm -m` shows it as `non-external (was a private external)`), so
  `dlsym( RTLD_DEFAULT, "find_me")` returns `NULL`.

The Mach-O linker spells the "export everything" flag differently:

| Linker        | Flag                      |
| ------------- | ------------------------- |
| ELF (Linux)   | `-Wl,--export-dynamic`    |
| Mach-O (macOS)| `-Wl,-export_dynamic`     |
| PE (Windows)  | `-Wl,--export-all-symbols`|

## The subtle trap: `-exported_symbol` overrides `-export_dynamic`

The test harness also exports specific startup symbols (`_mulle_atinit`,
`_mulle_atexit`, and — when the test allocator is linked — `_mulle_stacktrace`
and friends) using a per-symbol allowlist. On Mach-O that is spelled:

``` sh
-Wl,-exported_symbol -Wl,_<symbol>
```

Crucially, on Mach-O an explicit `-exported_symbol` allowlist **overrides**
`-export_dynamic`: once any `-exported_symbol` is present, the linker exports
*only* the listed symbols. Passing both together therefore does **not** export
`find_me`:

``` sh
# BROKEN on macOS: allowlist wins, only _mulle_atinit/_mulle_atexit exported,
#                  find_me is NOT exported -> dlsym returns NULL
clang ... -Wl,-exported_symbol -Wl,__mulle_atinit \
          -Wl,-exported_symbol -Wl,__mulle_atexit \
          -Wl,-export_dynamic
```

``` sh
# WORKS on macOS: -export_dynamic alone exports everything, including
#                 find_me AND the startup symbols
clang ... -Wl,-export_dynamic
```

Verified with `dyld_info -exports`:

- allowlist + `-export_dynamic` -> only `__mulle_atinit`, `__mulle_atexit`
- `-export_dynamic` alone       -> `_find_me`, `__mulle_atinit`,
  `_mulle_atinit_*`, ... (everything)

## Resolution in the toolchain

The fix lives in the test toolchain, not in this test:

1. **mulle-platform** gained a general `--export-dynamic` compile option that
   each compiler plugin renders in the correct per-linker spelling
   (`-Wl,-export_dynamic` on Darwin, `-Wl,--export-dynamic` on ELF, no-op on
   Windows/MSVC).
2. **mulle-test** requests `--export-dynamic` for the test executable on
   non-Windows platforms (Windows already uses `--export-all-symbols`), and its
   link-flag parser preserves an incoming `--export-dynamic` instead of
   dropping it.
3. On **Darwin**, when a general `--export-dynamic` is requested, the per-symbol
   `-exported_symbol` allowlist is suppressed, because it would otherwise
   override `-export_dynamic` and hide arbitrary symbols such as `find_me`.
   Nothing is lost: `-export_dynamic` exports the startup symbols too.

## Takeaway

If you write a test (or any executable) that relies on
`dlsym( RTLD_DEFAULT, "some_symbol")` finding a symbol defined in the executable
itself, remember:

- Linux exports executable symbols with `--export-dynamic`.
- macOS needs `-export_dynamic`, and must **not** simultaneously use an
  `-exported_symbol` allowlist, or only the allowlisted symbols will be visible.

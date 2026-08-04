# mulle-core-all-load Library Documentation for AI
<!-- Keywords: static-linking, all-load -->

## 1. Introduction & Purpose

mulle-core-all-load is a companion library to mulle-core that provides all-load functionality for static linking. It ensures all symbols from mulle-core and its constituent libraries are available when statically linked, preventing linker dead-code elimination of important initialization code and global constructors.

## 2. Key Concepts

- **All-Load Linking**: Forces linker to include all object files, not just referenced symbols
- **Static Initialization**: Ensures global constructors and initializers run correctly
- **Amalgamation Support**: Works with mulle-core amalgamated library

## 3. Constituent Libraries

This library includes all-load support for:
- mulle-atexit: Exit handler registration
- mulle-atinit: Initialization handler registration  
- mulle-dlfcn: Dynamic loading functionality
- mulle-stacktrace: Stack trace generation

See individual library documentation for detailed API information.

## 4. Usage

Link with `-lmulle-core-all-load` instead of `-lmulle-core` when static linking to ensure all symbols are included.

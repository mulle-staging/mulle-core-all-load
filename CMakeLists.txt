#
# You rarely need to edit this file. To add or remove files in your
# project edit the "src" folders contents and run `mulle-sde reflect`
#
#
# A convenient way to add a new source file is:
#    mulle-sde add src/foo.c
#
cmake_minimum_required( VERSION 3.19...99.99) # for REAL_PATH

project( mulle-core-all-load VERSION 0.8.0 LANGUAGES C)

# Common compiler flags for all platforms and compilers
set(COMMON_C_FLAGS "")

### AI generated

# Add suppression flags based on compiler
if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
  # GCC and Clang suppression flags
  add_compile_options(
    -Wno-unknown-pragmas
  )
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  # MSVC suppression flags
  add_compile_options(
    /wd4068  # Unknown pragma warning
  )
  # MSVC secure function warnings suppression
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# Your existing CMake configuration continues here...


###


set( MULLE_PROJECT_TYPE "Library")

### mulle-sde environment

# add cmake module paths to search path
list( INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/share")
list( INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/reflect")
list( INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include( Environment)

include( Files)

# Don't have this as default
# include( IDESupport OPTIONAL)

unset( SUB_PROJECT_NAMES)
file( GLOB SUB_PROJECT_PATHS src/mulle-*)

foreach( SUB_PROJECT_PATH ${SUB_PROJECT_PATHS})
   if( IS_DIRECTORY "${SUB_PROJECT_PATH}")
      get_filename_component( SUB_PROJECT_NAME "${SUB_PROJECT_PATH}" NAME)
      list( APPEND SUB_PROJECT_NAMES "${SUB_PROJECT_NAME}")
      message( STATUS "SUB_PROJECT_NAME = ${SUB_PROJECT_NAME}")
   endif()
endforeach()
set( SUB_PROJECT_NAMES ${SUB_PROJECT_NAMES} CACHE INTERNAL "sub-project names")


### Library

if( HEADER_PHASE)

   #
   # need to install like the embedded subprojects would
   #
   unset(REMAINING_HEADER)
   foreach( TMP_HEADER ${PROJECT_INSTALLABLE_HEADERS})
      get_filename_component( TMP_PROJECT_NAME "${TMP_HEADER}" DIRECTORY)
      get_filename_component( TMP_PROJECT_NAME "${TMP_PROJECT_NAME}" NAME)
      list( FIND SUB_PROJECT_NAMES "${TMP_PROJECT_NAME}" TMP_INDEX)
      # can't stay as a symlink, otherwise symlinks get installed which
      # fail, as they are relative
      file( REAL_PATH "${TMP_HEADER}" TMP_HEADER)
      if( NOT( "${TMP_INDEX}" EQUAL -1))
         install( FILES ${TMP_HEADER} DESTINATION "include/${TMP_PROJECT_NAME}")
      else()
         list( APPEND REMAINING_HEADER "${TMP_HEADER}")
      endif()
   endforeach()

   install( FILES ${REMAINING_HEADER} DESTINATION "include/${PROJECT_NAME}")
   install( FILES ${INSTALL_CMAKE_INCLUDES} DESTINATION "include/${PROJECT_NAME}/cmake")

   # manually maintained!
   install( FILES ${DLFCN_WIN32_PUBLIC_HEADERS}  DESTINATION "include")

   # short cut out
   if( NOT COMPILE_PHASE AND NOT LINK_PHASE)
      return()
   endif()
endif()

if( LINK_PHASE)
   include( Dependencies OPTIONAL)
endif()

include( Library)


# use LIBRARY_COMPILE_TARGET for compiler options
target_compile_definitions( "${LIBRARY_COMPILE_TARGET}" PUBLIC)
target_compile_options( "${LIBRARY_COMPILE_TARGET}" PUBLIC)
if( NOT MSVC)
   target_compile_options( "${LIBRARY_COMPILE_TARGET}" PUBLIC -Wunused)
endif()

# use LIBRARY_LINK_TARGET for linker options
if( LINK_PHASE)
   target_link_options( "${LIBRARY_LINK_TARGET}" PUBLIC)

   #
   # The idea is this is for non mulle-sde consumers to get the dependency
   # to mulle-core right. In shared lib configuration this is not necessary
   # Headers may still be a problem though
   # MEMO: should we do this in Library.cmake unconditionally ?
   if( NOT BUILD_SHARED_LIBS)
      target_link_libraries( "${LIBRARY_LINK_TARGET}" PUBLIC
               ${ALL_LOAD_DEPENDENCY_LIBRARIES}
               ${DEPENDENCY_LIBRARIES}
               ${DEPENDENCY_FRAMEWORKS}
               ${OPTIONAL_DEPENDENCY_LIBRARIES}
               ${OPTIONAL_DEPENDENCY_FRAMEWORKS}
               ${OS_SPECIFIC_LIBRARIES}
               ${OS_SPECIFIC_FRAMEWORKS})
   endif()

   # MEMO: no worky
   # target_include_directories( "${LIBRARY_LINK_TARGET}" INTERFACE
   #  ${CMAKE_CURRENT_BINARY_DIR}/include
   # )

   # Auto-apply force-link flags for external consumers using add_subdirectory()
   # This is needed because this is a force-linkable initialization library that
   # contains code that must be linked even if not directly referenced.
   # Without this, consumers using git submodule + add_subdirectory() would need
   # to manually add platform-specific force-link flags.
   if( NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
      if( APPLE)
         target_link_options( "${LIBRARY_LINK_TARGET}" INTERFACE 
            "-force_load" "$<TARGET_FILE:${LIBRARY_LINK_TARGET}>"
         )
      elseif( MSVC)
         target_link_options( "${LIBRARY_LINK_TARGET}" INTERFACE 
            "-WHOLEARCHIVE:$<TARGET_FILE:${LIBRARY_LINK_TARGET}>"
         )
      else()
         target_link_options( "${LIBRARY_LINK_TARGET}" INTERFACE 
            "-Wl,--whole-archive" "$<TARGET_FILE:${LIBRARY_LINK_TARGET}>" "-Wl,--no-whole-archive"
         )
      endif()
   endif()
endif()

include( InstallLibrary)

# Install TOC.md from constituent libraries
if( LINK_PHASE)
   foreach( SUB_PROJECT_NAME ${SUB_PROJECT_NAMES})
      set( SUB_ASSETS_TOC "${CMAKE_CURRENT_SOURCE_DIR}/src/${SUB_PROJECT_NAME}/TOC.md")
      if( EXISTS "${SUB_ASSETS_TOC}")
         message( STATUS "Installing TOC.md from ${SUB_PROJECT_NAME}")
         install( FILES "${SUB_ASSETS_TOC}"
                  PERMISSIONS OWNER_WRITE OWNER_READ
                              GROUP_READ
                              WORLD_READ
                  DESTINATION "share/${SUB_PROJECT_NAME}/dox")
      endif()
   endforeach()
endif()

if( LINK_PHASE)
   include( InstallCMakeInclude)
   include( InstallCMakePackage)
endif()

include( FinalOutput OPTIONAL)


######
#
# must be after CPACK_PACKAGE_VERSION
include( CMakePackage.cmake) # need extension
include( CPack)

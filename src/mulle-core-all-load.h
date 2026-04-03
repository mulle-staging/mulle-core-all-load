#ifndef mulle__core__all__load_h__
#define mulle__core__all__load_h__

#include "include.h"

#include <stdint.h>

// All amalgamated libraries are included via the main include.h
// Individual headers are available through the amalgamated sources

/*
 *  (c) <|YEAR|> nat <|ORGANIZATION|>
 *
 *  version:  major, minor, patch
 */
#define MULLE__CORE__ALL__LOAD_VERSION  ((0UL << 20) | (7 << 8) | 56)


static inline unsigned int   mulle_core_all_load_get_version_major( void)
{
   return( MULLE__CORE__ALL__LOAD_VERSION >> 20);
}


static inline unsigned int   mulle_core_all_load_get_version_minor( void)
{
   return( (MULLE__CORE__ALL__LOAD_VERSION >> 8) & 0xFFF);
}


static inline unsigned int   mulle_core_all_load_get_version_patch( void)
{
   return( MULLE__CORE__ALL__LOAD_VERSION & 0xFF);
}


// mulle-c11 feature: MULLE__CORE__ALL__LOAD_GLOBAL
uint32_t   mulle_core_all_load_get_version( void);


#include <mulle-atinit/mulle-atinit.h>
#include <mulle-atexit/mulle-atexit.h>
#include <mulle-stacktrace/mulle-stacktrace.h>

/*
 * The versioncheck header can be generated with
 * mulle-project-dependency-versions, but it is optional.
 */
#ifdef __has_include
# if __has_include( "_mulle-core-all-load-versioncheck.h")
#  include "_mulle-core-all-load-versioncheck.h"
# endif
#endif

#endif

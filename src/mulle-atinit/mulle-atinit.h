//
//  mulle-atinit.h
//  mulle-atinit
//
//  Created by Nat!
//  Copyright (c) 2017 Nat! - Mulle kybernetiK.
//  Copyright (c) 2017 Codeon GmbH.
//  All rights reserved.
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//  Neither the name of Mulle kybernetiK nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//

#ifndef mulle_atinit_h__
#define mulle_atinit_h__

#include "include.h"

#include <stdlib.h>
#include <stdint.h>


#if defined( _WIN32) && defined( MULLE_C_TRACE_DLL_EXPORTS)
# if defined( MULLE_INCLUDE_DYNAMIC)
#  pragma message("MULLE_INCLUDE_DYNAMIC is defined")
# endif
# if defined( MULLE__ATINIT_GLOBAL)
#  pragma message("MULLE__ATINIT_GLOBAL is defined as \"" MULLE_C_STRINGIFY_MACRO( MULLE__ATINIT_GLOBAL) "\"")
# else
#  pragma message("MULLE__ATINIT_GLOBAL is undefined")
# endif
#endif


#define MULLE__ATINIT_VERSION  ((0UL << 20) | (3 << 8) | 0)


static inline unsigned int   mulle_atinit_get_version_major( void)
{
   return( MULLE__ATINIT_VERSION >> 20);
}


static inline unsigned int   mulle_atinit_get_version_minor( void)
{
   return( (MULLE__ATINIT_VERSION >> 8) & 0xFFF);
}


static inline unsigned int   mulle_atinit_get_version_patch( void)
{
   return( MULLE__ATINIT_VERSION & 0xFF);
}

MULLE__ATINIT_GLOBAL
uint32_t   mulle_atinit_get_version( void);



typedef void   mulle_atinit_function_t( void (*f)( void *),
                                        void *userinfo,
                                        int priority,
                                        char *comment);

MULLE__ATINIT_GLOBAL
void   _mulle_atinit( void (*f)( void *),
                      void *userinfo,
                      int priority,
                      char *comment);


static inline void   mulle_atinit( void (*f)( void *),
                                   void *userinfo,
                                   int priority,
                                   char *comment)
{
#if defined( _WIN32) && defined( MULLE_INCLUDE_DYNAMIC)
   // as this is run like once per translation unit, it's no use
   // caching it (and also where ?)
   mulle_atinit_function_t   *p_mulle_atinit;

   p_mulle_atinit = (mulle_atinit_function_t *) mulle_dlsym_exe( "_mulle_atinit");
   if( ! p_mulle_atinit)
   {
      fprintf( stderr, "_mulle_atinit is not available yet, bummer\n");
      return;
   }
   (*p_mulle_atinit)( f, userinfo, priority, comment);
#else
   _mulle_atinit( f, userinfo, priority, comment);
#endif
}


#ifdef __has_include
# if __has_include( "_mulle-atinit-versioncheck.h")
#  include "_mulle-atinit-versioncheck.h"
# endif
#endif

#endif


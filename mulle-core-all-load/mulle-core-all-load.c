#include "include-private.h"

#include "mulle-core-all-load.h"


#ifndef MULLE__CORE__ALL__LOAD_BUILD
# error "MULLE__CORE__ALL__LOAD_BUILD must be defined by the compiler"
#endif



uint32_t   mulle_core_all_load_get_version( void)
{
   return( MULLE__CORE__ALL__LOAD_VERSION);
}

#include "include.h"

int main(void)
{
   uint32_t version;
   
   version = mulle_core_all_load_get_version();

   mulle_printf("mulle-core-all-load version seems ok\n", version >= 2048);
   mulle_printf("Force-link test completed\n");
   
   return 0;
}

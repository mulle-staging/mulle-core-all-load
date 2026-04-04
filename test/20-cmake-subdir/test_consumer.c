#include "include.h"

int main(void)
{
   uint32_t version;
   
   version = mulle_core_all_load_get_version();

   mulle_printf("mulle-core-all-load version: %u\n", version);
   mulle_printf("Force-link test completed\n");
   
   return 0;
}

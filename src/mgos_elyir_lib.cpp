// extern "C" {
#include "elyir.h"
// }
#include "mongoose/mongoose.h"

bool mgos_elyir_lib_init(void)
{
  elyir_begin();
  return true;
}
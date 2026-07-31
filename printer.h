#ifndef PRINTER_H
#define PRINTER_H

#include "safelisp.h"

#ifdef __cplusplus
extern "C" {
  #endif

  void print(FILE* output, void* o, int base);
  resizable_string_type* stringify(resizable_string_type* buf, void* o, int base);
  string_type* to_string_type(void* o, int base);

  #ifdef __cplusplus
}
#endif

#endif // PRINTER_H

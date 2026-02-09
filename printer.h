#ifndef PRINTER_H
#define PRINTER_H

#include "safelisp.h"

#ifdef __cplusplus
extern "C" {
  #endif

  void print(FILE* output, void* o, int base);
  
  #ifdef __cplusplus
}
#endif

#endif // PRINTER_H

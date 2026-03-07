#ifndef EVAL_H
#define EVAL_H

#include "safelisp.h"

#ifdef __cplusplus
extern "C" {
  #endif

  void* eval(void* list, void* env);
  void* eval_list(void* list, void* env);
  
  #ifdef __cplusplus
}
#endif

#endif // EVAL_H

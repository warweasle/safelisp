#ifndef EVAL_H
#define EVAL_H

#include "safelisp.h"

#ifdef __cplusplus
extern "C" {
  #endif

  void* eval(void* list, void* env);
  void* eval_list(void* list, void* env);

  // Call a TYPE_LAMBDA/TYPE_MACRO/TYPE_NATIVE value with an already-built
  // list of evaluated argument values (no re-evaluation, no macro
  // expansion). Exposed for rb-tree.c's custom-comparator support -- it
  // needs to call an interpreted comparator with two already-evaluated
  // keys, and env is whatever the calling Lisp code's env happens to be
  // (the comparator's own LAMBDA closure carries its defining scope,
  // per apply_callable's usual closure handling; env here only needs to
  // be a structurally valid environment cons, not any specific scope).
  void* apply_with_values(void* callee, void* args, void* env);
  
  #ifdef __cplusplus
}
#endif

#endif // EVAL_H

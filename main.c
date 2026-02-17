#include <stdio.h>

#include "safelisp.h"

/* #include <SDL3/SDL.h> */
/* #include <cairo.h> */
/* #include <pango/pango.h> */
/* #include <pango/pangocairo.h> */

int main(int argc, char* argv[]) {

  void* env = init_safelisp(stdin, stdout);
  
  // Call the parser
  void* atom = tread(env);
  atom = eval(atom, env);

  print(stdout, atom, 10);
  fputc('\n', stdout);

  //print(stdout, env, 10);
  //fputc('\n', stdout);
  
  return 0;
}

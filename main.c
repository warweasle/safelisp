#include <stdio.h>
#include "safelisp.h"

int main(int argc, char* argv[]) {

  void* env = init_taco(stdin, stdout);
  
  // Call the parser
  void* atom = tread(env);
  atom = eval(atom, env);

  print(stdout, atom, 10);
  fputc('\n', stdout);

  print(stdout, env, 10);
  fputc('\n', stdout);
  
  return 0;
}

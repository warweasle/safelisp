#include <stdio.h>
#include "safelisp.h"

int main(int argc, char* argv[]) {

  void* env = init_safelisp(stdin, stdout);
  
  // Call the parser
  void* atom = tread(env);

  // Eval
  atom = eval(atom, env);

  // Print
  print(stdout, atom, 10);
  fputc('\n', stdout);

  print(stdout, env, 10);
  fputc('\n', stdout);
  
  return 0;
}

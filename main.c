#include <stdio.h>
#include "taco.h"

#include "taco_parser.tab.h" // Include Bison-generated headers, which might vary by your setup
#include "taco_parser.yy.h"


int main(int argc, char* argv[]) {

  void* env = init_taco(stdin, stdout);
  
  // Call the parser
  void* atom = tread(env);
  atom = eval(atom, env);

  print(stdout, atom, 10);
  fputc('\n', stdout);

  return 0;
}

#include <stdio.h>
#include "taco.h"

#include "taco_parser.tab.h" // Include Bison-generated headers, which might vary by your setup
#include "taco_parser.yy.h"


int main(int argc, char* argv[]) {

  init_taco();
    
  // Set up any required input to the scanner here
  // YY_BUFFER_STATE bufferState = yy_scan_string("your input string", scanner);

  // Call the parser
  void* atom = tread(NULL);
  atom = eval(atom, NULL);
  fprintf(stdout, "\n");
  print(stdout, atom, 10);
  fputc('\n', stdout);
  return 0;
}

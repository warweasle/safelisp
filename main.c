#include <stdio.h>
#include "taco.h"

#include "taco.tab.h" // Include Bison-generated headers, which might vary by your setup
#include "taco.yy.h"


int main(int argc, char* argv[]) {

    // Initialize a scanner instance for Flex
  yyscan_t scanner;
  yylex_init(&scanner);
  
  // Set the input file for the lexer
  yyset_in(stdin, scanner);
  
  // Set up any required input to the scanner here
  // YY_BUFFER_STATE bufferState = yy_scan_string("your input string", scanner);

  // Call the parser
  void* atom = NULL;

  int parseResult = yyparse(scanner, &atom);

  printf("parseResult = %i\n", parseResult);
  
  // Clean up
  yylex_destroy(scanner);

  return 0;
}

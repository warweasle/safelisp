#include <stdio.h>
#include "taco.h"

#include "taco_parser.tab.h" // Include Bison-generated headers, which might vary by your setup
#include "taco_parser.yy.h"


int main(int argc, char* argv[]) {

  init_taco();
  
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
  
  if (parseResult == 0) {
    printf("Parsing successful %p.\n", atom);
    print(stdout, atom, 10);
    printf("\n");
    printf("\nNow evaling...\n");
    
    void* ret = eval(atom, NULL);
    printf("eval done...\n");
    print(stdout, ret, 10);
    printf("\n");
    
  } else {
    printf("Parsing failed. (%p)\n", atom);
  }

  
  // Clean up
  yylex_destroy(scanner);

  return 0;
}

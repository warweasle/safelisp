#include <stdio.h>
#include "safelisp.h"

// Loads and evaluates every top-level form in a prelude file, using the
// same tread()/eval() machinery as the main program -- this needs zero new
// C parsing/loading infrastructure. Swaps *INPUT*'s pointer to the prelude
// file for the duration, then restores it, so the real program still reads
// from stdin afterward exactly as before.
static void* load_prelude(const char* path, void* env) {

  FILE* prelude = fopen(path, "r");
  if(!prelude) {
    // Missing prelude isn't fatal -- FUN/MAC/FLET/MLET just won't exist.
    return NULL;
  }

  void* inputBinding = cassoc("*INPUT*", cdr(env));
  if(!inputBinding || !cdr(inputBinding)) {
    fclose(prelude);
    return ERROR("INPUT-BINDING-ERROR", "Could not find *INPUT* var!");
  }

  void* savedInput = cdr(inputBinding);
  cdr(inputBinding) = create_pointer_type(prelude, TYPE_POINTER);

  // tread() never signals end-of-file as an error -- the lexer's <<EOF>>
  // rule produces an ordinary symbol literally named "EOF" once no more
  // forms remain, so that sentinel (rather than feof()) is what actually
  // marks the end here.
  void* ret = NULL;
  while(1) {
    void* form = tread(env);

    if(is_type(form, TYPE_SYMBOL) && strcmp(to_string(form)->str, "EOF") == 0) {
      break;
    }

    ret = eval(form, env);
    if(is_error(ret)) break;
  }

  cdr(inputBinding) = savedInput;
  fclose(prelude);
  return ret;
}

int main(int argc, char* argv[]) {

  void* env = init_safelisp(stdin, stdout);

  void* preludeResult = load_prelude("prelude.safe", env);
  if(is_error(preludeResult)) {
    print(stderr, preludeResult, 10);
    fputc('\n', stderr);
  }

  // Call the parser
  void* atom = tread(env);

  // Eval
  atom = eval(atom, env);

  // Print
  print(stdout, atom, 10);
  fputc('\n', stdout);

  return 0;
}

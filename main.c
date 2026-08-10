#include <stdio.h>
#include <SDL3/SDL.h>
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

  // qix's entry point -- opens the editor's main window. safelisp's own
  // env/prelude are still initialized here since qix will script itself
  // with safelisp, but this is no longer the stdin-driven interpreter
  // loop main.c used to be (see git history on oceanwasp for that).
  void* env = init_safelisp(stdin, stdout);

  void* preludeResult = load_prelude("prelude.safe", env);
  if(is_error(preludeResult)) {
    print(stderr, preludeResult, 10);
    fputc('\n', stderr);
  }

  if(!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("qix", 1024, 768, 0);
  if(!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
  if(!renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  int running = 1;
  while(running) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      if(e.type == SDL_EVENT_QUIT) running = 0;
    }

    SDL_SetRenderDrawColor(renderer, 40, 80, 160, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

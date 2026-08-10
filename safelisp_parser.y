%define api.pure full
%locations
%param { yyscan_t scanner }

%parse-param { void** out_data } 


%code top {
#include "safelisp_parser.tab.h"
#include "safelisp_parser.yy.h"
#include "safelisp.h"

 }

%code requires {
  typedef void* yyscan_t;
   }
%code {

  void yyerror(YYLTYPE* yyllocp, yyscan_t unused, void** out_data, const char* msg);

  // A #X# seen before its own #X= resolved is left in the tree (see
  // safelisp_parser.l) as a plain symbol named "#X#", sitting wherever it
  // was used -- a placeholder. Once #X='s value is fully built, walk it
  // and replace every occurrence of that SAME symbol object (checked by
  // name, since a fresh symbol object is built per occurrence) with the
  // real value. Recurses through every cons-shaped type sharing
  // cons_cell's layout; anything else (atoms) has no sub-structure to
  // search.
  static void backpatch_references(void* node, const char* placeholder_name, void* real_value) {
    if(!node) return;

    switch(get_type(node)) {
    case TYPE_CONS:
    case TYPE_QUOTE:
    case TYPE_BACKTICK:
    case TYPE_COMMA:
    case TYPE_SPLICE:
    case TYPE_ERROR:
    case TYPE_VALUES:
    case TYPE_LAMBDA:
    case TYPE_MACRO:
      {
	void* c = to_cons(node)->car;
	if(is_type(c, TYPE_SYMBOL) && strcmp(to_string(c)->str, placeholder_name) == 0) {
	  to_cons(node)->car = real_value;
	}
	else {
	  backpatch_references(c, placeholder_name, real_value);
	}

	void* d = to_cons(node)->cdr;
	if(is_type(d, TYPE_SYMBOL) && strcmp(to_string(d)->str, placeholder_name) == 0) {
	  to_cons(node)->cdr = real_value;
	}
	else {
	  backpatch_references(d, placeholder_name, real_value);
	}
      }
      break;
    default:
      break;
    }
  }

 }

%union {
  void* p;
  int i;
}

%token LPAREN RPAREN QUOTE BACKTICK COMMA SPLICE DOT
%token <p> ATOM
%token <p> REFERENCE
%type <p> start
%type <p> sexpr 
%type <p> list
%type <p> members

%%

start: sexpr {$$ = $1;
   *out_data = $1;
   YYACCEPT;
 };

sexpr: ATOM      {$$ = $1;}
| list           {$$ = $1;}
| QUOTE sexpr    {
  $$ = create_quotetype(TYPE_QUOTE, $2);
 }
| BACKTICK sexpr {
  $$ = create_quotetype(TYPE_BACKTICK, $2);
 }
| SPLICE sexpr   {
  $$ = create_quotetype(TYPE_SPLICE, $2);
  }
| COMMA sexpr    {
     $$ = create_quotetype(TYPE_COMMA, $2);
   }
| REFERENCE sexpr {

  // $1's symbol text is "#N3" (safelisp_parser.l's #X= rule overwrites
  // the trailing '=' with '3' to build it) -- but every #X# lexer rule,
  // and the tree entry a placeholder puts itself under, uses "#N#".
  // Reconstruct that name so the tree key matches, and so the search
  // below is looking for what's actually there.
  size_t label_len = strlen(to_string($1)->str);
  char* label_name = (char*) GC_malloc(label_len + 1);
  strcpy(label_name, to_string($1)->str);
  label_name[label_len - 1] = '#';

  backpatch_references($2, label_name, $2);

  // mapset, not mapadd -- an unresolved #X# used before this #X= may
  // have already added a placeholder entry under this same key; mapset
  // updates it in place instead of inserting a duplicate.
  mapset(yyget_extra(scanner), create_symbol(label_name), $2, NULL);

  $$ = $2;
   }
;

list: LPAREN members RPAREN {$$ = $2;}
| LPAREN RPAREN         {
     $$ = cons(NULL, NULL);
   }
| LPAREN members DOT sexpr RPAREN {
  last($2)->cdr = $4; 
  $$ = $2;}
; 

members: sexpr          {
     $$ = cons($1, NULL);
     }
| sexpr members         {
     $$ = cons($1, $2);
   }

;
%%

void yyerror(YYLTYPE* yyllocp, yyscan_t unused, void** out_data, const char* msg) {
  
  out_data = (void**) ERROR("PARSE-ERROR", msg);
  fprintf(stderr, "[%d:%d]: %s\n",
		  yyllocp->first_line, yyllocp->first_column, msg);
 }

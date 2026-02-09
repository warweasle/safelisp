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

 }

%union {
  void* p;
}

%token LPAREN RPAREN QUOTE BACKTICK COMMA SPLICE DOT
%token <p> ATOM 
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
  
  out_data = (void**) ERROR(msg);
  fprintf(stderr, "[%d:%d]: %s\n",
		  yyllocp->first_line, yyllocp->first_column, msg);
 }

%option noinput nounput noyywrap 8bit nodefault
%option yylineno 
%option reentrant bison-bridge bison-locations

ws      [ \t\r\n\v\f]
digit   [0-9]
nondigit    [^\n\r\t\v\f ()`,/"{}']
id          {nondigit}+
word    [a-zA-Z0-9_]
float   [+-]?{digit}+\.{digit}+([eE][+-]?{digit}+)?

number_types ("u8"|"u16"|"u32"|"u64"|"u128"|"i8"|"i16"|"i32"|"i64"|"i128"|"f"|"d"|"ld")
float_number_types ("f"|"d"|"ld")

%x STRING
%x COMMENT

%{
#include "taco_parser.tab.h"
#include "taco.h"
%}

%%

{ws}+       {}

"("         { return LPAREN; }
")"         { return RPAREN; }
"'"         { return QUOTE; }
"`"         { return BACKTICK; }
",@"        { return SPLICE; }
","         { return COMMA; } 
"\."        { return DOT; }

"//"[^\n]*      { /* Ignore single-line comment */ }
"/*"            { BEGIN(COMMENT); }
<COMMENT>"*/"   { BEGIN(INITIAL); }
<COMMENT>.      { /* Consume comment contents */ }
<COMMENT><<EOF>>         { /* Handle EOF during comment for error reporting, possibly flag error */ }


"\""                    { BEGIN(STRING); 
yylval->p = create_resizable_string_type(1024, TYPE_RESIZABLE_STRING);
}

<STRING>"\""            { BEGIN(INITIAL);
yylval->p = create_string_type_from_resizable_string(yylval->p);
return ATOM;
}
<STRING>.|\n               { //printf("Just got a string char!!!: %c in string: %s\n", yytext[0], yytext);
                          yylval->p = putch_resizable_array(yylval->p, yytext[0]);
                          }
<STRING><<EOF>>         { /* Handle EOF during string for error reporting, possibly flag error */ }


[+-]?{digit}+    {

   int_type* newint = create_int_type(0);
   yylval->p = newint;
   mpz_set_str(newint->num, yytext, 10);
   return ATOM;

}


{float}     {

 float_type* newfloat = create_float_type();
 yylval->p = newfloat;
 mpf_set_str(newfloat->num, yytext, 10);
 return ATOM;

}

[tT][rR][uU][eE] {
    yylval->p = create_true_type();
    return ATOM;
}

[nN][uU][lL][lL] {
    yylval->p = NULL;
    return ATOM;
}

== {
    yylval->p = create_native_int_type(N_EQL);
    return ATOM;
}

= {
    yylval->p = create_native_int_type(N_SET);
    return ATOM;
}

[cC][oO][nN][sS] {

    yylval->p = create_native_int_type(N_CONS);
    return ATOM;
}

[cC][aR][rR] {

    yylval->p = create_native_int_type(N_CAR);
    return ATOM;
}

[cC][dD][rR] {

    yylval->p = create_native_int_type(N_CDR);
    return ATOM;
}

[Ll][Ii][Ss][Tt]  {

    yylval->p = create_native_int_type(N_LIST);
    return ATOM;
}
[?]          {

    yylval->p = create_native_int_type(N_IF);
    return ATOM;
}
[cC][oO][nN][dD]          {

    yylval->p = create_native_int_type(N_COND);
    return ATOM;
}

[Tt][Yy][Pp][Ee]  {

    yylval->p = create_native_int_type(N_TYPE);
    return ATOM;
}
[aA][pP][pP][eE][nN][dD]  {

    yylval->p = create_native_int_type(N_APPEND);
    return ATOM;
}
[&][&]  {

    yylval->p = create_native_int_type(N_AND);
    return ATOM;
}
[|][|]  {

    yylval->p = create_native_int_type(N_OR);
    return ATOM;
}
[pP][rR][iI][nN][tT]          {

    yylval->p = create_native_int_type(N_PRINT);
    return ATOM;
}
[tT][oO][-][sT][tT][rR][iI][nN][gG]          {

    yylval->p = create_native_int_type(N_TO_STRING);
    return ATOM;
}


[a-zA-Z_+*/<>=!?$%&~^][a-zA-Z0-9_+*/<>=!?$%&~^.-]*         { 

// Convert to uppercase
int i;
for(i=0; i<yyleng; i++) {
    
    yytext[i] = toupper(yytext[i]);
}

yylval->p = create_symbol_and_copy(yyleng, yytext);
return ATOM;
}



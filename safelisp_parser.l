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
#include "safelisp_parser.tab.h"
#include "safelisp.h"
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
<COMMENT>\n     {}
<COMMENT><<EOF>>         { /* Handle EOF during comment for error reporting, possibly flag error */ }


#\\[Ss][Pp][Aa][Cc][Ee]    { yylval->p = create_char_type(' ');  return ATOM; }
#\\[Nn][Ee][Ww][Ll][Ii][Nn][Ee]  { yylval->p = create_char_type('\n'); return ATOM; }
#\\[Tt][Aa][Bb]      { yylval->p = create_char_type('\t'); return ATOM; }
#\\[Rr][Ee][Tt][Uu][Rr][Nn]   { yylval->p = create_char_type('\r'); return ATOM; }
#\\[Nn][Uu][Ll][Ll]     { yylval->p = create_char_type('\0'); return ATOM; }
#\\.        { yylval->p = create_char_type(yytext[2]); return ATOM; }

"\""                    { BEGIN(STRING); 
yylval->p = create_resizable_string_type(1024, TYPE_RESIZABLE_STRING);
}

<STRING>"\""            { BEGIN(INITIAL);
yylval->p = create_string_type_from_resizable_string(yylval->p);
return ATOM;
}

<STRING>\\\"  {yylval->p = putch_resizable_array(yylval->p, '"');}
<STRING>\\\\  {yylval->p = putch_resizable_array(yylval->p, '\\');}
<STRING>\\n   {yylval->p = putch_resizable_array(yylval->p, '\n');}
<STRING>\\t   {yylval->p = putch_resizable_array(yylval->p, '\t');}
<STRING>\\r   {yylval->p = putch_resizable_array(yylval->p, '\r');}
<STRING>\\.   {BEGIN(INITIAL); yylval->p = ERROR("Unknown escape sequence in string!"); return ATOM;}

<STRING>.|\n               { //printf("Just got a string char!!!: %c in string: %s\n", yytext[0], yytext);
                          yylval->p = putch_resizable_array(yylval->p, yytext[0]);
                          }

<STRING><<EOF>>  {yylval->p = ERROR("Unterminated string!!! (EOF)");
return ATOM;}


<<EOF>> {yylval->p = create_symbol("EOF");
return ATOM;}

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

[=][=] {
    yylval->p = create_native_int_type(N_EQL);
    return ATOM;
}

[!][=] {
    yylval->p = create_native_int_type(N_NEQL);
    return ATOM;
}

[<] {
    yylval->p = create_native_int_type(N_LT);
    return ATOM;
}

[>] {
    yylval->p = create_native_int_type(N_GT);
    return ATOM;
}

[<][=] {
    yylval->p = create_native_int_type(N_LTE);
    return ATOM;
}

[>][=] {
    yylval->p = create_native_int_type(N_GTE);
    return ATOM;
}

[sS][eE][tT] {
    yylval->p = create_native_int_type(N_SET);
    return ATOM;
}

[cC][oO][nN][sS] {

    yylval->p = create_native_int_type(N_CONS);
    return ATOM;
}

[cC][aA][rR] {

    yylval->p = create_native_int_type(N_CAR);
    return ATOM;
}

[cC][dD][rR] {

    yylval->p = create_native_int_type(N_CDR);
    return ATOM;
}

[cC][aAdD]+[rR] {

   char *s = yytext;
   int n = yyleng;

   // We want uppercase for printing later...
   for(int i=1; i<n-1; i++) {
      s[i] = toupper(s[i]);
   }

   yylval->p = make_cnr(create_string_type_and_copy(n-2, s+1, TYPE_STRING));
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

[!][?]          {

    yylval->p = create_native_int_type(N_NIF);
    return ATOM;
}

[?][.][.][.]          {

    yylval->p = create_native_int_type(N_COND);
    return ATOM;
}

[?][?]          {

    yylval->p = create_native_int_type(N_WHEN);
    return ATOM;
}

[!][?][?]          {

    yylval->p = create_native_int_type(N_UNLESS);
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

[!]  {

    yylval->p = create_native_int_type(N_NOT);
    return ATOM;
}

[pP][rR][iI][nN][tT]          {

    yylval->p = create_native_int_type(N_PRINT);
    return ATOM;
}

[tT][oO][sS][tT][rR][iI][nN][gG]          {

    yylval->p = create_native_int_type(N_TO_STRING);
    return ATOM;
}

[eE][vV][aA][lL]          {

    yylval->p = create_native_int_type(N_EVAL);
    return ATOM;
}

[<][>] {
    yylval->p = create_native_int_type(N_LOOP);
    return ATOM;
}

[<][?][>]  {
   yylval->p = create_native_int_type(N_WHILE);
    return ATOM;
}

[bB][rR][eE][aA][kK] {
    yylval->p = create_native_int_type(N_BREAK);
    return ATOM;
}

[wW][iI][tT][hH][rR][eE][sS][tT][aA][rR][tT] {
    yylval->p = create_native_int_type(N_WITH_RESTART);
    return ATOM;
}

[iI][nN][vV][oO][kK][eE][rR][eE][sS][tT][aA][rR][tT] {
    yylval->p = create_native_int_type(N_INVOKE_RESTART);
    return ATOM;
}

[tT][yY][pP][eE] {
    yylval->p = create_native_int_type(N_TYPE);
    return ATOM;
}

[rR][eE][aA][dD] {
    yylval->p = create_native_int_type(N_READ);
    return ATOM;
}

[mM][aA][pP][mM][aA][kK][eE] {
    yylval->p = create_native_int_type(N_MAPMAKE);
    return ATOM;
}

[mM][aA][pP][aA][dD][dD] {
    yylval->p = create_native_int_type(N_MAPADD);
    return ATOM;
}

[mM][aA][pP][gG][eE][tT] {
    yylval->p = create_native_int_type(N_MAPGET);
    return ATOM;
}

[mM][aA][pP][sS][eE][tT] {
    yylval->p = create_native_int_type(N_MAPSET);
    return ATOM;
}

[mM][aA][pP][dD][eE][lL] {
    yylval->p = create_native_int_type(N_MAPDEL);
    return ATOM;
}

[mM][aA][pP] {
    yylval->p = create_native_int_type(N_MAP);
    return ATOM;
}

[rR][eE][dD][uU][cC][eE] {
    yylval->p = create_native_int_type(N_REDUCE);
    return ATOM;
}

[fF][iI][lL][tT][eE][rR] {
    yylval->p = create_native_int_type(N_FILTER);
    return ATOM;
}

[aA][pP][pP][lL][yY] {
    yylval->p = create_native_int_type(N_APPLY);
    return ATOM;
}


[cC][aA][tT] {
    yylval->p = create_native_int_type(N_CAT);
    return ATOM;
}

[lL][eE][tT] {
    yylval->p = create_native_int_type(N_LET);
    return ATOM;
}

[lL][aA][mM][bB][dD][aA] {
    yylval->p = create_native_int_type(N_LAMBDA);
    return ATOM;
}

[mM][aA][cC] {
    yylval->p = create_native_int_type(N_MACRO);
    return ATOM;
}

[+] {
    yylval->p = create_native_int_type(N_ADD);
    return ATOM;
}

[-] {
    yylval->p = create_native_int_type(N_SUB);
    return ATOM;
}

[*] {
    yylval->p = create_native_int_type(N_MULT);
    return ATOM;
}

[/] {
    yylval->p = create_native_int_type(N_DIV);
    return ATOM;
}

[mM][oO][dD] {
    yylval->p = create_native_int_type(N_MOD);
    return ATOM;
}

[qQ][uU][oO][tT][iI][eE][nN][tT] {
    yylval->p = create_native_int_type(N_QUOTIENT);
    return ATOM;
}

[rR][eE][mM][aA][iI][nN][dD][eE][rR] {
    yylval->p = create_native_int_type(N_REMAINDER);
    return ATOM;
}

[fF][lL][oO][oO][rR] {
    yylval->p = create_native_int_type(N_FLOOR);
    return ATOM;
}

[cC][eE][iI][lL][iI][nN][gG] {
    yylval->p = create_native_int_type(N_CEILING);
    return ATOM;
}

[rR][oO][uU][nN][dD] {
    yylval->p = create_native_int_type(N_ROUND);
    return ATOM;
}

[tT][rR][uU][nN][cC][aA][tT][eE] {
    yylval->p = create_native_int_type(N_TRUNCATE);
    return ATOM;
}

[aA][bB][sS] {
    yylval->p = create_native_int_type(N_ABS);
    return ATOM;
}

[sS][qQ][rR][tT] {
    yylval->p = create_native_int_type(N_SQRT);
    return ATOM;
}

[eE][xX][pP][tT] {
    yylval->p = create_native_int_type(N_EXPT);
    return ATOM;
}

[mM][iI][nN] {
    yylval->p = create_native_int_type(N_MIN);
    return ATOM;
}

[mM][aA][xX] {
    yylval->p = create_native_int_type(N_MAX);
    return ATOM;
}

[gG][cC][dD] {
    yylval->p = create_native_int_type(N_GCD);
    return ATOM;
}

[lL][cC][mM] {
    yylval->p = create_native_int_type(N_LCM);
    return ATOM;
}

[eE][xX][aA][cC][tT][?] {
    yylval->p = create_native_int_type(N_EXACTP);
    return ATOM;
}

[iI][nN][eE][xX][aA][cC][tT][?] {
    yylval->p = create_native_int_type(N_INEXACTP);
    return ATOM;
}

[tT][yY][pP][eE][?] {
    yylval->p = create_native_int_type(N_TYPEIS);
    return ATOM;
}

[nN][uU][lL][lL][?] {
    yylval->p = create_native_int_type(N_NULLP);
    return ATOM;
}

[cC][oO][nN][sS][?] {
    yylval->p = create_native_int_type(N_CONSP);
    return ATOM;
}

[pP][rR][oO][cC][eE][dD][uU][rR][eE][?] {
    yylval->p = create_native_int_type(N_PROCEDUREP);
    return ATOM;
}

[lL][eE][nN] {
    yylval->p = create_native_int_type(N_LEN);
    return ATOM;
}

[sS][uU][bB][sS][tT][rR] {
    yylval->p = create_native_int_type(N_SUBSTR);
    return ATOM;
}

[sS][tT][rR][rR][eE][fF] {
    yylval->p = create_native_int_type(N_STRREF);
    return ATOM;
}

[sS][tT][rR][uU][pP][pP][eE][rR] {
    yylval->p = create_native_int_type(N_STRUPPER);
    return ATOM;
}

[sS][tT][rR][lL][oO][wW][eE][rR] {
    yylval->p = create_native_int_type(N_STRLOWER);
    return ATOM;
}

[sS][tT][rR][eE][qQ] {
    yylval->p = create_native_int_type(N_STREQ);
    return ATOM;
}

[vV][aA][lL][uU][eE][sS] {
    yylval->p = create_native_int_type(N_VALUES);
    return ATOM;
}

[nN][tT][hH][vV][aA][lL][uU][eE] {
    yylval->p = create_native_int_type(N_NTHVALUE);
    return ATOM;
}

[mM][uU][lL][tT][iI][pP][lL][eE][vV][aA][lL][uU][eE][lL][iI][sS][tT] {
    yylval->p = create_native_int_type(N_MULTIPLEVALUELIST);
    return ATOM;
}

[aA][vV][aA][iI][lL][aA][bB][lL][eE][rR][eE][sS][tT][aA][rR][tT][sS] {
    yylval->p = create_native_int_type(N_AVAILABLERESTARTS);
    return ATOM;
}

\.\.\. {
    yylval->p = create_native_int_type(N_PROGN);
    return ATOM;
}

[1-9][0-9]*\.\.\. {

    int n = atoi(yytext);

    if(n == 0) {
      yylval->p = ERROR("N... Must be a positive integer!!!");
    }
    else if(n == 1) {
        yylval->p = create_native_int_type(N_PROG1);
    }
    else {
      /* Create/store an N... operation containing n. */
      yylval->p = (cc) create_nprog_type(n);
    }
    return ATOM;
}

[0-9a-zA-Z_+*/<>=!?$%&~^][a-zA-Z0-9_+*/<>=!?$%&~^.-]*         { 

// Convert to uppercase
int i;
for(i=0; i<yyleng; i++) {
    
    yytext[i] = toupper(yytext[i]);
}



yylval->p = create_symbol_and_copy(yyleng, yytext);
return ATOM;
}

<INITIAL>.|\n {
    char buf[1024];
    snprintf(buf, sizeof(buf), "LEX: unexpected char '%c' (0x%02X) at line %d\n",
             yytext[0], (unsigned char)yytext[0], yylineno);

    yylval->p = ERROR(buf);
    return ATOM;
}

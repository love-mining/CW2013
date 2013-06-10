%{
/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "ccccccc.tab.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

%}

%option noyywrap
%option yylineno

%state ANSI_C_COMMENT

%%

<INITIAL>
{

else    return KEYWORD_ELSE;
if      return KEYWORD_IF;
int     return KEYWORD_INT;
return  return KEYWORD_RETURN;
void    return KEYWORD_VOID;
while   return KEYWORD_WHILE;

[a-zA-Z]+   yylval.str = strdup(yytext); return MISC_ID;
 /* [_a-zA-Z][a-zA-Z]*  yylval.str = strdup(yytext); return MISC_ID; */

[0-9]+  yylval.num = strtoul(yytext, NULL, 10); return MISC_NUM;
 /* [\+\-]?[0-9]+   yylval.num = strtol(yytext, NULL, 10); return MISC_NUM; */

\+  return SYMBOL_ADD;
\-  return SYMBOL_SUB;
\*  return SYMBOL_MUL;
\/  return SYMBOL_DIV;
\<  return SYMBOL_LT;
\<\=    return SYMBOL_LEQ;
\>  return SYMBOL_GT;
\>\=    return SYMBOL_GEQ;
\=\=    return SYMBOL_EEQL;
\!\=    return SYMBOL_NEQ;
\=  return SYMBOL_EQL;
\;  return SYMBOL_SEMICOLON;
\,  return SYMBOL_COMMA;
\(  return SYMBOL_PARENTHESIS_L;
\)  return SYMBOL_PARENTHESIS_R;
\[  return SYMBOL_SQUARE_L;
\]  return SYMBOL_SQUARE_R;
\{  return SYMBOL_BRACKET_L;
\}  return SYMBOL_BRACKET_R;


\/\*    BEGIN(ANSI_C_COMMENT);
[ \n\t]+    /* eat whitespaces */ 
[^a-zA-Z0-9 \n\t\+\-\*\/\<\=\>\!\;\,\(\)\[\]\{\}]+ yyerror("unknown token.");

}


<ANSI_C_COMMENT>
{

\*\/    BEGIN(INITIAL);
\*  ; /* ignore it */
[^\*]+  ;   /* ignore it */


}

%%
    

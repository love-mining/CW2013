%{
/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "E6_30.h"
#include "E6_30.tab.h"

%}

%option noyywrap
%option yylineno

%%

int return TYPE_INT;
bool    return TYPE_BOOL;
array   return TYPE_ARRAY;
of  return KW_OF;
if  return KW_IF;
then    return KW_THEN;
or  return KW_OR;
true    return KW_TRUE;
false   return KW_FALSE;

\[  return MISC_L_SQUARE; 
\]  return MISC_R_SQUARE;
\;  return MISC_SEMICOLON;
\:  return MISC_COLON;
\:\=    return MISC_COLON_EQUAL;
\+  return MISC_PLUS;

[0-9]+  yylval.str = strdup(yytext); return MISC_NUMBER;
[_a-zA-Z][_a-zA-Z0-9]*  yylval.str = strdup(yytext); return MISC_IDENTIFIER;

[ \n\t]+    /* eat the whitespaces */
[^_a-zA-Z0-9 \n\t\[\]\;\:\+\=]+   yyerror("unknown token.");

%%
    

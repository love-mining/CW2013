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
#include "ccccccc.h"

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
return  return KEYWORD_RETURN;
while   return KEYWORD_WHILE;
int     yylval.num = TYPE_INT; return KEYWORD_INT;
void    yylval.num = TYPE_VOID; return KEYWORD_VOID;

[a-zA-Z]+   yylval.str = strdup(yytext); return MISC_ID;
 /* [_a-zA-Z][_a-zA-Z]*  yylval.str = strdup(yytext); return MISC_ID; */

[0-9]+  yylval.num = strtoul(yytext, NULL, 10); return MISC_NUM;
 /* [\+\-]?[0-9]+   yylval.num = strtol(yytext, NULL, 10); return MISC_NUM; */

\+  return yylval.num = SYMBOL_ADD;
\-  return yylval.num = SYMBOL_SUB;
\*  return yylval.num = SYMBOL_MUL;
\/  return yylval.num = SYMBOL_DIV;
\<  return yylval.num = SYMBOL_LT;
\<\=    return yylval.num = SYMBOL_LEQ;
\>  return yylval.num = SYMBOL_GT;
\>\=    return yylval.num = SYMBOL_GEQ;
\=\=    return yylval.num = SYMBOL_EEQL;
\!\=    return yylval.num = SYMBOL_NEQ;
\=  return yylval.num = SYMBOL_EQL;
\;  return yylval.num = SYMBOL_SEMICOLON;
\,  return yylval.num = SYMBOL_COMMA;
\(  return yylval.num = SYMBOL_PARENTHESIS_L;
\)  return yylval.num = SYMBOL_PARENTHESIS_R;
\[  return yylval.num = SYMBOL_SQUARE_L;
\]  return yylval.num = SYMBOL_SQUARE_R;
\{  return yylval.num = SYMBOL_BRACKET_L;
\}  return yylval.num = SYMBOL_BRACKET_R;


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
    

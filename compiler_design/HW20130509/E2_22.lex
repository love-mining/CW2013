%{
/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <err.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static inline void str_to_upper(char *str)
{
    for (; *str; str++)
        if (isalpha(*str))
            *str = toupper(*str);
    return;
}

%}

%option noyywrap

%state ANSI_C_COMMENT
%state C99_COMMENT

%%

<INITIAL>
{
\/\*    ECHO; BEGIN(ANSI_C_COMMENT); 
\/\/    ECHO; BEGIN(C99_COMMENT); 
.|\n    ECHO; 
}

<ANSI_C_COMMENT>
{
\*\/    ECHO; BEGIN(INITIAL); 
\*      ECHO;
[^\*]+  str_to_upper(yytext); ECHO; 
} 

<C99_COMMENT>
{
\\\n    ECHO;
\n      ECHO; BEGIN(INITIAL); 
\\      ECHO;
[^\\\n]+    str_to_upper(yytext); ECHO; 
} 

%%

int main(int argc, char **argv)
{
    /* bad parameter format or requires help message */
    if (argc > 3 || (argc > 1 && !strcmp(argv[1], "-h")))
    {
        printf(
            "Usage:\n"
            "%s \t# input=stdin output=stdout\n"
            "%s input_file \t# output=stdout\n"
            "%s input_file output_file\n",
            argv[0], argv[0], argv[0]
            );
        return 0;
    }
    if (argc > 2) /* output_file specified */
    {
        yyout = fopen(argv[2], "w");
        if (!yyout)
            err(-1, "cannot open file %s for output", argv[2]);
    }
    if (argc > 1) /* input file specified */
    {
        yyin = fopen(argv[1], "r");
        if (!yyin)
            err(-1, "cannot open file %s for input", argv[1]);
    }

    yylex();

    /* there shall be no problem invoking fclose with stdin/stdout */
    fclose(yyin);
    fclose(yyout);
    return 0;
}


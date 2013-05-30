/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include "E6_30.tab.h"
#include "E6_30.yy.h"

static char *input_filename = NULL;

int yyerror(char *msg)
{
    errx(1, "line %d of %s: '%s': Error: %s", yyget_lineno(), input_filename, yytext, msg);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2 || (argc > 1 && !strcmp(argv[1], "-h"))) 
    {
        printf("Usage: %s inputfile\n", argv[0]);
        return 0;
    }
    input_filename = argv[1];
    yyin = fopen(input_filename, "r");
    if (!yyin)
         err(-1, "cannot open file %s for input", input_filename);
    
    /* start the parsing process */
    yyparse();

    /* since control flow reaches here, the input programme is acceppted */
    fprintf(stderr, "File passed grammar checking: %s\n", input_filename);

    return 0;
}


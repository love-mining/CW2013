%{
/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>

typedef struct _count_t
{
    unsigned int chr;   /* for characters */
    unsigned int wrd;   /* for words */
    unsigned int ln;    /* for lines */
}   count_t;

static count_t _cnt = {};

%}

%option noyywrap

%%

[0-9A-Za-z]+    _cnt.chr += yyleng; _cnt.wrd += 1;
[^0-9A-Za-z\n]+ _cnt.chr += yyleng;
\n              _cnt.chr += 1; _cnt.ln += 1;

%%

int main(int argc, char **argv)
{
    /* requires help message */
    if (argc > 1 && !strcmp(argv[1], "-h"))
    {
        printf(
            "Usage:\n"
            "%s \t# input=stdin\n"
            "%s input_file ...\n",
            argv[0], argv[0]
            );
        return 0;
    }

    if (argc == 1) /* using stdin as input file */
    {
        _cnt = (count_t){};

        /* yyin is initially set to stdin */
        yylex();
    
        /* to output exactly in the format of `wc` of GNU Coreutils */
        printf("%7u %7u %7u\n", _cnt.ln, _cnt.wrd, _cnt.chr);
    }
    if (argc > 1) /* input file specified */
    {   
        count_t cnt[argc];
        cnt[0] = (count_t){};

        int i;
        for (i = 1; i < argc; i++)
        {
            _cnt = (count_t){};
            yyin = fopen(argv[i], "r");
            if (!yyin)
                err(-1, "cannot open file %s for input", argv[i]);
            yylex();
            fclose(yyin);
            cnt[i] = _cnt;
            cnt[0].chr += _cnt.chr;
            cnt[0].wrd += _cnt.wrd;
            cnt[0].ln += _cnt.ln;
        }

        /* to get the maximum output width */
        int width = 0;
        char *tmpstr;
        asprintf(&tmpstr, "%u%n", cnt[0].chr, &width); /* chr is the lagest */
        free(tmpstr);

        /* to output exactly in the format of `wc` of GNU Coreutils */
        for (i = 1; i < argc; i++)
            printf("%*5$u %*5$u %*5$u %s\n", 
                cnt[i].ln, cnt[i].wrd, cnt[i].chr, argv[i], width);
        printf("%*4$u %*4$u %*4$u total\n", 
            cnt[0].ln, cnt[0].wrd, cnt[0].chr, width);
    }
    return 0;
}


/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ccccccc.yy.h"
#include "ccccccc.tab.h"
#include "ccccccc.h"

static char *input_filename = NULL;

int parsing_error(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    char *fmt;
    asprintf(&fmt, "line %d of %s: Error: %s", yyget_lineno(), input_filename, msg);
    verrx(1, fmt, ap);
    return 0;
}

int yyerror(const char *msg)
{
    errx(1, "line %d of %s: '%s': Error: %s", yyget_lineno(), input_filename, yytext, msg);
    return 0;
}

static inline u32_t elfhash(const char *s)
{
    u32_t h = 0;
    while (*s)
    {
        h = (h << 4) + *s++;
        
        u32_t g = h >> 24;
        if (g)
            h ^= g;
        h &= 0x00FFFFFF;
    }
    return h;
}

htable_t *htable_new(u32_t size)
{
    htable_t *ret = malloc(sizeof(htable_t));
    ret->size = size;
    ret->pentry = calloc(size, sizeof(hentry_t*));
    return ret;
}

hentry_t *htable_insert(htable_t *htable, const char *key, hvalue_t *value)
{
    hentry_t *entry = htable_find(htable, key);
    if (entry)
        return NULL;
        
    u32_t hkey = elfhash(key) % htable->size;
    entry = malloc(sizeof(hentry_t));
    entry->key = strdup(key);
    entry->value = value;
    entry->next = htable->pentry[hkey];
    htable->pentry[hkey] = entry;
    return entry;
}

hentry_t *htable_find(htable_t *htable, const char *key)
{
    u32_t hkey = elfhash(key) % htable->size;
    hentry_t *entry = htable->pentry[hkey];
    while (entry && strcmp(entry->key, key))
        entry = entry->next;
    return entry;
}

void htable_delete(htable_t *htable)
{
    /* TODO: free the table and all entries cascadingly */
#if 0
    int i;
    for (i = 0; i < htable->size; i++)
        hentry_free(htable->pentry[i]);
#endif
    free(htable->pentry);
    free(htable);
    return;
}

static local_env_t _global_env = {NULL, NULL, 0};
local_env_t *global_env = &_global_env;
local_env_t *current_env = &_global_env;

static inline init(int argc, char **argv)
{
    if (argc != 2 || (argc > 1 && !strcmp(argv[1], "-h"))) 
        err(1, "Usage: %s inputfile\n", argv[0]);
    input_filename = argv[1];
    yyin = fopen(input_filename, "r");
    if (!yyin)
        err(1, "cannot open file %s for input", input_filename);

    global_env->symbol_table = htable_new(CONFIG_HTABLE_SIZE);

    return;
}

int main(int argc, char **argv)
{
    init(argc, argv);
   
    /* start the parsing process */
    yyparse();

    /* since control flow reaches here, the input programme is acceppted */
    fprintf(stderr, "File passed grammar checking: %s\n", input_filename);

    return 0;
}



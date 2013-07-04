/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <err.h>
#include <assert.h>
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

static local_env_t _global_env;
local_env_t *global_env = &_global_env;
local_env_t *current_env = &_global_env;

static unsigned codebufsz;
static unsigned codesz;
static int codeoffset;
static char **codebuf;
static int codehead;

int get_code_cursor()
{
    return codesz;
}

int set_code_cursor(int cursor)
{
    assert(cursor < codebufsz);
    int ret = codesz;
    codesz = cursor;
    return ret;
}

static int gen_code_buffered(const char *code, int comment)
{
    int ret = codeoffset;
    if (comment)
        codebuf[codesz] = strdup(code);
    else 
    {
        if (codehead == codesz) /* normal code */
        {
            asprintf(&codebuf[codesz], "%d: %s", codeoffset, code);
            codeoffset += 1;
        }
        else /* fillback code */
        {
            char *tmp;
            asprintf(
                &tmp, 
                "%.*s: %s", 
                strchr(codebuf[codesz], ':') - codebuf[codesz], 
                codebuf[codesz], 
                code
            );
            free(codebuf[codesz]);
            codebuf[codesz] = tmp;
        }

    }
    assert(codebuf[codesz]);
    codesz += 1;
    if (codesz > codehead)
        codehead = codesz;
    if (codesz >= codebufsz)
    {
        codebufsz *= 2;
        codebuf = realloc(codebuf, sizeof(char*) * codebufsz);
        assert(codebuf);
    }
    return ret;
}

int gen_code_buffered_RO(const char *opcode, int r, int s, int t)
{
    char *code;
    asprintf(&code, "%.5s %d, %d, %d", opcode, r, s, t);
    assert(code);
    int ret = gen_code_buffered(code, 0);
    free(code);
    return ret;
}

int gen_code_buffered_RM(const char *opcode, int r, int d, int s)
{
    char *code;
    asprintf(&code, "%.5s %d, %d(%d)", opcode, r, d, s);
    assert(code);
    int ret = gen_code_buffered(code, 0);
    free(code);
    return ret;
}

int gen_comment_buffered(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *code;
    vasprintf(&code, fmt, ap);
    int ret = gen_code_buffered(code, 1);
    free(code);
    return ret;
}

static inline init(int argc, char **argv)
{
    if (argc != 2 || (argc > 1 && !strcmp(argv[1], "-h"))) 
        err(1, "Usage: %s inputfile\n", argv[0]);
    input_filename = argv[1];
    yyin = fopen(input_filename, "r");
    if (!yyin)
        err(1, "cannot open file %s for input", input_filename);

    global_env->parent = NULL;
    global_env->symbol_table = htable_new(CONFIG_HTABLE_SIZE);
    global_env->base = 0;
    global_env->varsz = 0;

    codesz = 0;
    codeoffset = CONFIG_CODE_START_OFFSET;
    codebufsz = CONFIG_CODE_BUF_SIZE;
    codebuf = malloc(sizeof(char*) * codebufsz);
    codehead = 0;

    /* symbol table insertion */

    symbol_t *symbol;
    func_t *func;

    symbol = malloc(sizeof(symbol_t));
    symbol->name = "input";
    symbol->type = SYMBOL_FUNC;
    func = &symbol->func;
    func->type = TYPE_INT;
    func->offset = CONFIG_INPUT_OFFSET;
    func->paramsz = 0;
    func->param = NULL;
    assert(htable_insert(global_env->symbol_table, "input", symbol));

    symbol = malloc(sizeof(symbol_t));
    symbol->name = "output";
    symbol->type = SYMBOL_FUNC;
    func = &symbol->func;
    func->type = TYPE_VOID;
    func->offset = CONFIG_OUTPUT_OFFSET;
    func->paramsz = 1;
    func->param = malloc(sizeof(param_t));
    func->param->name = "_";
    func->param->type = TYPE_INT;
    func->param->arraysz = -1;
    func->param->next = NULL;
    assert(htable_insert(global_env->symbol_table, "output", symbol));

    return;
}

static void dump_code(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    // for now..
    vprintf(fmt, ap);
    puts("");
    return;
}

int main(int argc, char **argv)
{
    init(argc, argv);
   
    /* start the parsing process */
    yyparse();

    /* since control flow reaches here, the input programme is acceppted */
    fprintf(stderr, "File passed grammar checking: %s\n", input_filename);

    hentry_t *hmain = htable_find(global_env->symbol_table, "main");
    assert(hmain);
    symbol_t *smain = hmain->value;
    assert(smain);
    assert(smain->type == SYMBOL_FUNC);
    assert(smain->func.type == TYPE_VOID);
    assert(smain->func.paramsz == 0);

    const int end_of_code = 18;
    const int main_return = 7;
    dump_code("* initialization");
    dump_code("0: LDC  0, 0(0)");
    dump_code("1: LDC  %d, %d(0) * stack pointer", REG_STACK_PTR, 1 + global_env->varsz);
    dump_code("2: LDC  %d, %d(0)  * frame pointer", REG_FRAME_PTR, global_env->varsz);
    dump_code("3: LDC  1, %d(0) * return address", main_return);
    dump_code("4: ST   1, 0(%d)  * return address", REG_FRAME_PTR);
    dump_code("5: LDC  1, %d(0) * main entrance", smain->func.offset);
    dump_code("6: JEQ  0, 0(1)  * jump to main");
    dump_code("7: HALT 0, 0, 0");
    dump_code("* input");
    dump_code("8: IN   0, 0, 0");
    dump_code("9: LDC  1, 0, 0");
    dump_code("10: LD   2, -1, %d    * return address", REG_STACK_PTR);
    dump_code("11: JEQ  1, 0, 2      * return");
    dump_code("12: HALT 0, 0, 0 * for alignment & debugging usage");
    dump_code("* output");
    dump_code("13: LD   1, -2, %d   * param", REG_STACK_PTR);
    dump_code("14: OUT  1, 0, 0");
    dump_code("15: LDC  1, 0, 0");
    dump_code("16: LD   2, -1, %d    * return address", REG_STACK_PTR);
    dump_code("17: JEQ  1, 0, 2      * return");
    dump_code("18: HALT 0, 0, 0 * for alignment & debugging usage");

    /* TODO: check CONFIG_INPUT_OFFSET and CONFIG_OUTPUT_OFFSET */

    assert(end_of_code < CONFIG_CODE_START_OFFSET);
    int i;
    for (i = end_of_code; i < CONFIG_CODE_START_OFFSET; i++)
        dump_code("%d: HALT 0, 0, 0", i);
    for (i = 0; i < codesz; i++)
        dump_code(codebuf[i]);

    return 0;
}



/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#ifndef E6_30_H
#define E6_30_H

#define ERRPTR ((void*)-1)
#ifndef NULL
#define NULL ((void*)0)
#endif

#define CONFIG_HTABLE_SIZE  1007
#define CONFIG_CODE_BUF_SIZE    1024
#define CONFIG_CODE_START_OFFSET    32
#define CONFIG_INPUT_OFFSET 8
#define CONFIG_OUTPUT_OFFSET    13

enum
{
    STATUS_FAILED,
    STATUS_SUCCEEDED,
};

enum
{
    REG_TMP0 = 0,
    REG_TMP1 = 1,
    REG_TMP2 = 2,
    REG_TMP3 = 3,
    REG_RES0 = 4,
    REG_STACK_PTR = 5,
    REG_FRAME_PTR = 6,
    REG_PCOUNTER = 7,
};

typedef unsigned int u32_t;
typedef unsigned long u64_t;
typedef int i32_t;
typedef long long i64_t;
typedef u32_t ptr_t;

typedef void hvalue_t;

typedef struct _hentry_t
{
    const char *key;
    hvalue_t *value;
    struct _hentry_t *next;
}   hentry_t;

typedef struct _htable_t
{
    u32_t size;
    hentry_t **pentry;
}   htable_t;

typedef struct _local_env_t
{
    struct _local_env_t *parent;
    htable_t *symbol_table;
    int base;
    int varsz;
}   local_env_t;

enum 
{
    TYPE_VOID,
    TYPE_INT,
    TYPE_NONE = -1,
};

enum
{
    SYMBOL_VAR,
    SYMBOL_FUNC,
};

typedef struct _var_t
{
    int type;
    int arraysz;
    int offset;
}   var_t;

typedef struct _param_t
{
    char *name;
    int type;
    int arraysz;
    struct _param_t *next;
}   param_t;

typedef struct _func_t
{
    int type;
    int offset;
    int paramsz;
    struct _param_t *param;
}   func_t;

typedef struct _symbol_t
{
    char *name;
    int type;
    union
    {
        struct _var_t var;
        struct _func_t func;
    };
}   symbol_t;

extern int parsing_error(const char *msg, ...);
extern htable_t *htable_new(u32_t size);
extern void htable_delete(htable_t *htable);
extern hentry_t *htable_insert(htable_t *htable, const char *key, hvalue_t *value);
extern hentry_t *htable_find(htable_t *htable, const char *key);
extern hentry_t *htable_find_local(local_env_t *env, const char *key);

extern local_env_t *global_env;
extern local_env_t *current_env;

extern int gen_comment_buffered(const char *fmt, ...);
extern int gen_code_buffered_RO(const char *opcode, int r, int s, int t);
extern int gen_code_buffered_RM(const char *opcode, int r, int d, int s);

#endif /* CCCCCCC_H */

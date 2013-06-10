/*
 * This is part of the coursework for Compiler Design, 2013.05-06
 * By Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#ifndef E6_30_H
#define E6_30_H

#define ERRPTR ((void*)-1)
enum
{
    STATUS_FAILED,
    STATUS_SUCCEEDED,
};

typedef unsigned int u32_t;
typedef unsigned long u64_t;
typedef int i32_t;
typedef long long i64_t;

typedef void *hvalue_t;

typedef struct _hentry_t
{
    const char *key;
    hvalue_t value;
    struct _hentry_t *next;
}   hentry_t;

typedef struct _htable_t
{
    u32_t size;
    hentry_t **pentry;
}   htable_t;

extern htable_t *htable_new(u32_t size);
extern void htable_delete(htable_t *htable);
extern hentry_t *htable_insert(htable_t *htable, const char *key, hvalue_t value);
extern hentry_t *htable_find(htable_t *htable, const char *key);

#endif /* E6_30_H */

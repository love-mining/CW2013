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

typedef struct _type_t
{
    int ttype;
    int tsize;
    struct _type_t *tchild;
}   type_t;

typedef struct _node_t
{
    const char *name;
    struct _type_t type;
    struct _node_t *next;
}   node_t;

extern node_t *insert_node(const char *name, type_t *type);
extern node_t *find_node(const char *name);

#endif /* E6_30_H */

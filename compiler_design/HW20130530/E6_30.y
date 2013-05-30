%{
/* 
 * This is part of the coursework for Compiler Design, 2013.05_06 
 * by Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */
 
//#include "mass.yy.h"
#ifndef NULL
#define NULL ((void*)0)
#endif

#include <stdlib.h>
#include "E6_30.h"

/* well we just ignore the potential memory leak.. */
static inline type_t *new_type(int ttype, int tsize, type_t *tchild)
{
    type_t *t = malloc(sizeof(type_t));
    t->ttype = ttype;
    t->tsize = tsize;
    t->tchild = tchild;
    return t;
}

static inline int same_type(const type_t *type1, const type_t *type2)
{
    
    int ret = (type1->ttype == type2->ttype)
        && (type1->tsize == type2->tsize)
        && ((type1->tchild == NULL && type2->tchild == NULL) 
            || (type1->tchild 
                && type2->tchild 
                && same_type(type1->tchild, type2->tchild)));
    return ret;
}

%}

%union /* typedef of yylval */
{
    char *str;
    struct _type_t *type;
    struct _node_t *node;
}

%token <str>
    TYPE_INT TYPE_BOOL TYPE_ARRAY 
    KW_OF KW_IF KW_THEN KW_OR KW_TRUE KW_FALSE
    MISC_IDENTIFIER MISC_NUMBER
    MISC_L_SQUARE MISC_R_SQUARE MISC_SEMICOLON MISC_COLON MISC_COLON_EQUAL
    MISC_PLUS

%type <str>
    program 
    var_decls 
    stmts 
    var_decl
    stmt

%type <type>
    type_exp
    exp

%start all

%%

all: 
    program;

program:
    var_decls MISC_SEMICOLON stmts;

var_decls:
    var_decls MISC_SEMICOLON var_decl
    | var_decl
    ;

var_decl:   
    MISC_IDENTIFIER MISC_COLON type_exp
    {
        insert_node($1, $3);
    }
    ;

type_exp:
    TYPE_INT    { $$ = new_type(TYPE_INT, 0, NULL); }
    | TYPE_BOOL { $$ = new_type(TYPE_BOOL, 0, NULL); }
    | TYPE_ARRAY MISC_L_SQUARE MISC_NUMBER MISC_R_SQUARE KW_OF type_exp
        /* assuming MISC_NUMBER can always be converted into a int */
        { $$ = new_type(TYPE_ARRAY, strtol($3, NULL, 10), $6); }
    ;

stmts:
    stmts MISC_SEMICOLON stmt
    | stmt
    ;

stmt:
    KW_IF exp KW_THEN stmt  
    {
        if ($2->ttype != TYPE_BOOL)
            yyerror("type error");
    }
    | MISC_IDENTIFIER MISC_COLON_EQUAL exp
    {
        node_t *node = find_node($1);
        if (!node)
            yyerror("undeclared identifier");
        if (!same_type(&node->type, $3))
            yyerror("type error");
    }
    ;

exp:
    exp MISC_PLUS exp
    {
        if (($1->ttype != TYPE_INT) || ($3->ttype != TYPE_INT))
            yyerror("type error");
        $$ = new_type(TYPE_INT, 0, NULL);
    }
    | exp KW_OR exp
    {
        if (($1->ttype != TYPE_BOOL) || ($3->ttype != TYPE_BOOL))
            yyerror("type error");
        $$ = new_type(TYPE_BOOL, 0, NULL);
    }
    | exp MISC_L_SQUARE exp MISC_R_SQUARE 
    {
        if (($1->ttype != TYPE_ARRAY) || ($3->ttype != TYPE_INT))
            yyerror("type error");
        $$ = $1->tchild;
    }
    | MISC_NUMBER   { $$ = new_type(TYPE_INT, 0, NULL); }
    | MISC_IDENTIFIER
    {
        node_t *node = find_node($1);
        if (!node)
            yyerror("undeclared identifier");
        $$ = &node->type;
    }
    | KW_TRUE   { $$ = new_type(TYPE_BOOL, 0, NULL); }
    | KW_FALSE  { $$ = new_type(TYPE_BOOL, 0, NULL); }
    ;

%%


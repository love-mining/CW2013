%{
/* 
 * This is part of the coursework for Compiler Design, 2013.05_06 
 * by Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */
 
//#include "mass.yy.h"

%}

%union /* typedef of yylval */
{
    int int_t;
    char *str_t;
}

%token <str_t>
    KW_INT KW_BOOL KW_ARRAY KW_OF
    KW_IF KW_THEN 
    MISC_IDENTIFIER MISC_NUMBER
    MISC_L_SQUARE MISC_R_SQUARE MISC_SEMICOLON MISC_COLON MISC_COLON_EQUAL

%type <str_t>
    program 
    var_decls 
    stmts 
    var_decl
    type_exp
    stmt
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
    MISC_IDENTIFIER MISC_COLON type_exp;

type_exp:
    KW_INT
    | KW_BOOL
    | KW_ARRAY MISC_L_SQUARE MISC_NUMBER MISC_R_SQUARE KW_OF type_exp
    ;

stmts:
    stmts MISC_SEMICOLON stmt
    | stmt
    ;

stmt:
    KW_IF exp KW_THEN stmt
    | MISC_IDENTIFIER MISC_COLON_EQUAL exp
    ;

/*
 * This rule is not specified from the coursework and added by me, since the
 * program would obviously be incomplete without this.
 */
exp:
    MISC_NUMBER;

%%


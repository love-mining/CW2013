%{
/* 
 * This is part of the coursework for Compiler Design, 2013.05-06 
 * by Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <string.h>
#include <stdlib.h>

//#include "ccccccc.yy.h"
#include "ccccccc.h"

%}

%union /* typedef of yylval */
{
    char *str;
    long num;
    struct _func_t *func;
    struct _param_t *param;
}

%token <str>
    KEYWORD_ELSE KEYWORD_IF KEYWORD_RETURN KEYWORD_WHILE 
    MISC_ID

    SYMBOL_ADD SYMBOL_SUB SYMBOL_MUL SYMBOL_DIV SYMBOL_LT SYMBOL_LEQ SYMBOL_GT
    SYMBOL_GEQ SYMBOL_EEQL SYMBOL_NEQ SYMBOL_EQL 
    SYMBOL_SEMICOLON SYMBOL_COMMA SYMBOL_PARENTHESIS_L SYMBOL_PARENTHESIS_R
    SYMBOL_SQUARE_L SYMBOL_SQUARE_R SYMBOL_BRACKET_L SYMBOL_BRACKET_R

%token <num>
    MISC_NUM KEYWORD_INT KEYWORD_VOID

%type <str>
    program
    declaration_list
    declaration
    var_declaration
    fun_declaration
    fun_prototype
    compound_stmt
    local_declarations
    statement_list
    statement
    expression_stmt
    selection_stmt
    iteration_stmt
    return_stmt
    expression
    var
    simple_expression
    relop
    additive_expression
    addop
    term
    mulop
    factor
    call
    args
    arg_list
    empty

%type <num>
    type_specifier

%type <func>
    params
    param_list

%type <param>
    param

%right STMT_IF KEYWORD_ELSE

%start all

%%

all:
    program ;

program:
    declaration_list ;

declaration_list:
    declaration_list declaration 
    | declaration
    ;

declaration:
    var_declaration
    | fun_declaration
    ;

var_declaration:
    type_specifier MISC_ID SYMBOL_SEMICOLON
    {
        symbol_t *symbol = malloc(sizeof(symbol_t));
        symbol->name = strdup($2);
        symbol->type = SYMBOL_VAR;
        var_t *var = &symbol->var;
        var->type = $1;
        var->arraysz = -1;
        var->offset = current_env->varsz;
        current_env->varsz += 1;
        if (!htable_insert(current_env->symbol_table, $2, symbol))
            parsing_error("identifier '%s' already exists.", $2);
    }
    | type_specifier MISC_ID SYMBOL_SQUARE_L MISC_NUM SYMBOL_SQUARE_R
        SYMBOL_SEMICOLON
    {
        if ($1 == TYPE_VOID)
            parsing_error("'void' type cannot make up arrays.");
        symbol_t *symbol = malloc(sizeof(symbol_t));
        symbol->name = strdup($2);
        symbol->type = SYMBOL_VAR;
        var_t *var = &symbol->var;
        var->type = $1;
        var->arraysz = $4;
        var->offset = current_env->varsz;
        current_env->varsz += $4;
        if (!htable_insert(current_env->symbol_table, $2, symbol))
            parsing_error("identifier '%s' already exists.", $2);
    }
    ;

type_specifier:
    KEYWORD_INT
    | KEYWORD_VOID
    ;

fun_declaration:
    fun_prototype compound_stmt
    {
        puts("ffffffffffffunc");
    }
    ;

fun_prototype:
    type_specifier MISC_ID SYMBOL_PARENTHESIS_L params SYMBOL_PARENTHESIS_R 
    {
        symbol_t *symbol = malloc(sizeof(symbol_t));
        symbol->name = strdup($2);
        symbol->type = SYMBOL_FUNC;
        func_t *func = &symbol->func;
        func->type = $1;
        func->paramsz = $4->paramsz;
        func->param = $4->param;
        if (!htable_insert(current_env->symbol_table, $2, symbol))
            parsing_error("identifier '%s' already exists.", $2);
        free($4);
    }
    ;

params:
    param_list 
    | KEYWORD_VOID
    {
        func_t *func = malloc(sizeof(func_t));
        func->type = TYPE_VOID;
        func->paramsz = 0;
        func->param = NULL;
        $1 = func;
    }
    ;

param_list:
    param_list SYMBOL_COMMA param 
    {
        $1->paramsz += 1;
        $3->next = $1->param;
        $1->param = $3;
        $$ = $1;
    }
    | param
    {
        func_t *func = malloc(sizeof(func));
        func->type = TYPE_NONE;
        func->paramsz = 1;
        func->param = $1;
        $$ = func;
    }
    ;

param:
    type_specifier MISC_ID
    {
        param_t *param = malloc(sizeof(param_t));
        param->name = strdup($2);
        param->type = $1;
        param->arraysz = -1;
        param->next = NULL;
        $$ = param;
    }
    | type_specifier MISC_ID SYMBOL_SQUARE_L SYMBOL_SQUARE_R
    {
        param_t *param = malloc(sizeof(param_t));
        param->name = strdup($2);
        param->type = $1;
        param->arraysz = 0;
        param->next = NULL;
        $$ = param;
    }
    ;

compound_stmt:
    SYMBOL_BRACKET_L local_declarations statement_list SYMBOL_BRACKET_R
    {
        puts("ssssssssstmt");
    }
    ;

local_declarations:
    local_declarations var_declaration
    | empty
    ;

statement_list:
    statement_list statement
    | empty
    ;

statement:
    expression_stmt
    | compound_stmt
    | selection_stmt
    | iteration_stmt
    | return_stmt
    ;

expression_stmt:
    expression SYMBOL_SEMICOLON
    | SYMBOL_SEMICOLON
    ;

selection_stmt:
    KEYWORD_IF SYMBOL_PARENTHESIS_L expression SYMBOL_PARENTHESIS_R statement
        %prec STMT_IF
    | KEYWORD_IF SYMBOL_PARENTHESIS_L expression SYMBOL_PARENTHESIS_R statement
        KEYWORD_ELSE statement
    ;

iteration_stmt:
    KEYWORD_WHILE SYMBOL_PARENTHESIS_L expression SYMBOL_PARENTHESIS_R statement
    ;

return_stmt:
    KEYWORD_RETURN SYMBOL_SEMICOLON
    | KEYWORD_RETURN expression SYMBOL_SEMICOLON
    ;

expression:
    var SYMBOL_EQL expression
    | simple_expression
    ;

var:
    MISC_ID
    | MISC_ID SYMBOL_SQUARE_L expression SYMBOL_SQUARE_R
    ;

simple_expression:
    additive_expression relop additive_expression
    | additive_expression
    ;

relop:
    SYMBOL_LEQ | SYMBOL_LT | SYMBOL_GT | SYMBOL_GEQ | SYMBOL_EEQL | SYMBOL_NEQ ;

additive_expression:
    additive_expression addop term
    | term
    ;

addop:
    SYMBOL_ADD | SYMBOL_SUB ;

term:
    term mulop factor
    | factor
    ;

mulop:
    SYMBOL_MUL | SYMBOL_DIV ;

factor:
    SYMBOL_PARENTHESIS_L expression SYMBOL_PARENTHESIS_R
    | var
    | call
    | MISC_NUM
    ;

call:
    MISC_ID SYMBOL_PARENTHESIS_L args SYMBOL_PARENTHESIS_R 
    ;

args:
    arg_list
    | empty
    ;

arg_list:
    arg_list SYMBOL_COMMA expression
    | expression
    ;

empty:
    {}
    ;

%%


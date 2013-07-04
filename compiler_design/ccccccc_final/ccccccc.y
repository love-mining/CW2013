%{
/* 
 * This is part of the coursework for Compiler Design, 2013.05-06 
 * by Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

//#include "ccccccc.yy.h"
#include "ccccccc.h"

typedef struct _int2_t
{
    int x;
    int y;
}   int2_t;
%}

%union /* typedef of yylval */
{
    char *str;
    long num;
    struct _func_t *func;
    struct _param_t *param;
    struct _var_t *var;
    struct _int2_t *int2;
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
    compound_stmt_stage1
    compound_stmt_stage2
    compound_stmt
    local_declarations
    statement_list
    statement
    expression_stmt
    selection_stmt
    iteration_stmt
    return_stmt
    expression
    simple_expression
    additive_expression
    term
    factor
    call
    args
    arg_list
    empty

%type <num>
    type_specifier
    relop
    addop
    mulop
    selection_stmt_stage1
    selection_stmt_stage2
    iteration_stmt_stage1

%type <int2>
    iteration_stmt_stage2

%type <func>
    params
    param_list

%type <param>
    param

%type <var>
    var

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
        var->offset = current_env->varsz + current_env->base;
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
        var->offset = current_env->varsz + current_env->base;
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
        assert(current_env->parent);
        current_env = current_env->parent;
 
        /* generate codes for function exits */
        gen_comment_buffered("* exiting function");
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 0, REG_FRAME_PTR);
        gen_code_buffered_RM("LD", REG_FRAME_PTR, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDC", REG_TMP1, 0, 0);
        gen_code_buffered_RM("LD", REG_TMP2, -1, REG_STACK_PTR);
        gen_code_buffered_RM("JEQ", REG_TMP1, 0, REG_TMP2);
    }
    ;

fun_prototype:
    type_specifier MISC_ID SYMBOL_PARENTHESIS_L params SYMBOL_PARENTHESIS_R 
    {
        /* generate codes for funcs */
        int func_offset = gen_comment_buffered("* func: %s", $2);
        gen_code_buffered_RM("ST", REG_FRAME_PTR, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_FRAME_PTR, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);

        /* symbol table insertion */
        symbol_t *symbol = malloc(sizeof(symbol_t));
        symbol->name = strdup($2);
        symbol->type = SYMBOL_FUNC;
        func_t *func = &symbol->func;
        func->type = $1;
        func->offset = func_offset;
        func->paramsz = $4->paramsz;
        func->param = $4->param;
        if (!htable_insert(current_env->symbol_table, $2, symbol))
            parsing_error("identifier '%s' already exists.", $2);
        free($4);

        /* replace the global env using a local one */
        local_env_t *new_env = malloc(sizeof(local_env_t));
        new_env->parent = current_env;
        new_env->symbol_table = htable_new(CONFIG_HTABLE_SIZE);
        new_env->base = 0;
        new_env->varsz = 1; /* we've already have pused FRAME_PTR */
        current_env = new_env;

        /* insert params to local env */
        param_t *param;
        for (param = func->param; param; param = param->next)
        {
            symbol_t *symbol = malloc(sizeof(symbol_t));
            symbol->type = SYMBOL_VAR;
            symbol->name = strdup(param->name);
            var_t *var = &symbol->var;
            var->type = param->type;
            var->arraysz = param->arraysz;
            var->offset = current_env->varsz + current_env->base; 
            if (var->arraysz <= 0) /* int or array pointer */
                current_env->varsz += 1;
            else
                current_env->varsz += var->arraysz;
            if (!htable_insert(current_env->symbol_table, param->name, symbol))
                parsing_error("identifier '%s' already exists.", param->name);
        }
        
        gen_comment_buffered("* copying parameters");
        int i;
        for (i = current_env->varsz; i > 0; i--)
        {
            gen_code_buffered_RM("LD", REG_TMP0, - i, REG_FRAME_PTR);
            gen_code_buffered_RM("ST", REG_TMP0, 0, REG_STACK_PTR);
            gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);
        }
        gen_comment_buffered("* end of copying parameters");
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
        $$ = func;
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
        gen_comment_buffered("* param: %s", $1->name);
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

compound_stmt_stage1:
    SYMBOL_BRACKET_L 
    {
        /* replace the global env using a local one */
        local_env_t *new_env = malloc(sizeof(local_env_t));
        new_env->parent = current_env;
        new_env->symbol_table = htable_new(CONFIG_HTABLE_SIZE);
        new_env->base = current_env->base + current_env->varsz;
        new_env->varsz = 0;
        current_env = new_env;
    }
    ;

compound_stmt_stage2:
    compound_stmt_stage1 local_declarations 
    {
        gen_comment_buffered("* enter compound stmt");
        gen_code_buffered_RM("LDA", REG_STACK_PTR, current_env->varsz, REG_STACK_PTR);
    }
    ;
compound_stmt:
    compound_stmt_stage2 statement_list SYMBOL_BRACKET_R
    {
        assert(current_env->parent);
        gen_comment_buffered("* leave compound stmt");
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -current_env->varsz, REG_STACK_PTR);
        current_env = current_env->parent;
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
    {
        gen_comment_buffered("* expression_stmt: to balance stack pointer");
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
    }
    | SYMBOL_SEMICOLON
    ;

selection_stmt_stage1:
    KEYWORD_IF SYMBOL_PARENTHESIS_L expression SYMBOL_PARENTHESIS_R
    {
        gen_comment_buffered("* begin of if");
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP0, 0, REG_STACK_PTR);
        /* to be refilled later */
        $$ = get_code_cursor();
        gen_code_buffered_RM("LDA", REG_TMP1, 0, REG_TMP1);
        gen_code_buffered_RM("JEQ", REG_TMP0, -1, -1); 
    }
    ;

selection_stmt_stage2:
    selection_stmt_stage1 statement
    {
        /* for later fill back */
        $$ = get_code_cursor();
        gen_code_buffered_RM("LDA", REG_TMP0, 0, REG_TMP0);
        gen_code_buffered_RM("LDA", REG_TMP0, 0, REG_TMP0);
        gen_code_buffered_RM("LDA", REG_TMP0, 0, REG_TMP0);

        /* fill back */
        int offset = gen_comment_buffered("* end of if");
        int cursor = set_code_cursor($1);
        gen_code_buffered_RM("LDC", REG_TMP1, offset, 0);
        gen_code_buffered_RM("JEQ", REG_TMP0, 0, REG_TMP1); 
        set_code_cursor(cursor);

        gen_comment_buffered("* begin of else");
    }
    ;

selection_stmt:
    selection_stmt_stage2 %prec STMT_IF
    {
        gen_comment_buffered("* end of else");
    }
    | selection_stmt_stage2 KEYWORD_ELSE statement
    {
        /* fill back */
        int offset = gen_comment_buffered("* end of else");
        int cursor = set_code_cursor($1);
        gen_code_buffered_RM("LDC", REG_TMP0, 0, 0);
        gen_code_buffered_RM("LDC", REG_TMP1, offset, 0);
        gen_code_buffered_RM("JEQ", REG_TMP0, 0, REG_TMP1); 
        set_code_cursor(cursor);
    }
    ;

iteration_stmt_stage1:
    KEYWORD_WHILE SYMBOL_PARENTHESIS_L 
    {
        $$ = gen_comment_buffered("* begin of while");
    }
    ;
iteration_stmt_stage2:
    iteration_stmt_stage1 expression SYMBOL_PARENTHESIS_R 
    {
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP0, 0, REG_STACK_PTR);
        /* to be refilled later */
        int tmp = get_code_cursor();
        gen_code_buffered_RM("LDA", REG_TMP1, 0, REG_TMP1);
        gen_code_buffered_RM("JEQ", REG_TMP0, -1, -1); 
        $$ = malloc(sizeof(int2_t));
        $$->x = $1;
        $$->y = tmp;
    }
    ;
iteration_stmt:
    iteration_stmt_stage2 statement
    {
        gen_code_buffered_RM("LDC", REG_TMP0, 0, 0);
        gen_code_buffered_RM("LDC", REG_TMP1, $1->x, 0);
        gen_code_buffered_RM("JEQ", REG_TMP0, 0, REG_TMP1); 
        /* fill back */
        int offset = gen_comment_buffered("* end of while");
        int cursor = set_code_cursor($1->y);
        gen_code_buffered_RM("LDC", REG_TMP1, offset, 0);
        gen_code_buffered_RM("JEQ", REG_TMP0, 0, REG_TMP1); 
        set_code_cursor(cursor);
    }
    ;

return_stmt:
    KEYWORD_RETURN SYMBOL_SEMICOLON
    {
        /* generate codes for function exits */
        gen_comment_buffered("* exiting function");
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 0, REG_FRAME_PTR);
        gen_code_buffered_RM("LD", REG_FRAME_PTR, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDC", REG_TMP1, 0, 0);
        gen_code_buffered_RM("LD", REG_TMP2, -1, REG_STACK_PTR);
        gen_code_buffered_RM("JEQ", REG_TMP1, 0, REG_TMP2);
    }
    | KEYWORD_RETURN expression SYMBOL_SEMICOLON
    {
        /* generate codes for function exits */
        gen_comment_buffered("* exiting function");
        gen_code_buffered_RM("LD", REG_TMP0, -1, REG_FRAME_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 0, REG_FRAME_PTR);
        gen_code_buffered_RM("LD", REG_FRAME_PTR, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDC", REG_TMP1, 0, 0);
        gen_code_buffered_RM("LD", REG_TMP2, -1, REG_STACK_PTR);
        gen_code_buffered_RM("JEQ", REG_TMP1, 0, REG_TMP2);
    }
    ;

expression:
    var SYMBOL_EQL expression
    {
        gen_comment_buffered("* assignment");
        gen_code_buffered_RM("LD", REG_TMP0, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP1, -2, REG_STACK_PTR);
        gen_code_buffered_RM("ST", REG_TMP0, 0, REG_TMP1);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
    }
    | simple_expression
    ;

var:
    MISC_ID
    {
        hentry_t *hvar = NULL;
        local_env_t *tmp_env = current_env;
        while (tmp_env)
        {
            hvar = htable_find(tmp_env->symbol_table, $1);
            if (hvar)
                break;
            tmp_env = tmp_env->parent;
        }
        if (!hvar)
            parsing_error("undefined identifier: %s", $1);
        symbol_t *svar = hvar->value;
        assert(svar);
        if (svar->type != SYMBOL_VAR)
            parsing_error("target is not a variable: %s", $1);
        var_t *var = malloc(sizeof(var_t));
        var->offset = svar->var.offset;
        if (current_env->parent)
            gen_code_buffered_RM("LDA", REG_TMP1, var->offset, REG_FRAME_PTR);
        else
            gen_code_buffered_RM("LDC", REG_TMP1, var->offset, 0);
        gen_code_buffered_RM("ST", REG_TMP1, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);
        $$ = var;
    }
    | MISC_ID SYMBOL_SQUARE_L expression SYMBOL_SQUARE_R
    {
        hentry_t *hvar = NULL;
        local_env_t *tmp_env = current_env;
        while (tmp_env)
        {
            hvar = htable_find(tmp_env->symbol_table, $1);
            if (hvar)
                break;
            tmp_env = tmp_env->parent;
        }
        if (!hvar)
            parsing_error("undefined identifier: %s", $1);
        symbol_t *svar = hvar->value;
        assert(svar);
        if (svar->type != SYMBOL_VAR)
            parsing_error("target is not a variable: %s", $1);
        var_t *var = malloc(sizeof(var_t));
        if (current_env->parent)
            gen_code_buffered_RM("LDA", REG_TMP1, var->offset, REG_FRAME_PTR);
        else
            gen_code_buffered_RM("LDC", REG_TMP1, var->offset, 0);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP0, 0, REG_STACK_PTR);
        gen_code_buffered_RO("ADD", REG_TMP1, REG_TMP1, REG_TMP0);
        gen_code_buffered_RM("ST", REG_TMP1, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);
        $$ = var;
    }
    ;

simple_expression:
    additive_expression relop additive_expression
    {
        static const char *i2j[] = {
            [SYMBOL_LEQ] = "JLE",
            [SYMBOL_LT] = "JLT",
            [SYMBOL_GT] = "JGT",
            [SYMBOL_GEQ] = "JGE",
            [SYMBOL_EEQL] = "JEQ",
            [SYMBOL_NEQ] = "JNE",
        };
        gen_comment_buffered("* relop ");
        gen_code_buffered_RM("LD", REG_TMP0, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP1, -2, REG_STACK_PTR);
        gen_code_buffered_RO("SUB", REG_TMP1, REG_TMP1, REG_TMP0);
        gen_code_buffered_RM("LDC", REG_TMP0, 1, 0);
        gen_code_buffered_RM(i2j[$2], REG_TMP1, 1, REG_PCOUNTER);
        gen_code_buffered_RM("LDC", REG_TMP0, 0, 0);
        gen_code_buffered_RM("ST", REG_TMP0, -2, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
    
    }
    | additive_expression
    ;

relop:
    SYMBOL_LEQ | SYMBOL_LT | SYMBOL_GT | SYMBOL_GEQ | SYMBOL_EEQL | SYMBOL_NEQ ;

additive_expression:
    additive_expression addop term
    {
        gen_comment_buffered("* addop");
        gen_code_buffered_RM("LD", REG_TMP0, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP1, -2, REG_STACK_PTR);
        gen_code_buffered_RO($2 == SYMBOL_ADD ? "ADD" : "SUB", REG_TMP1, REG_TMP1, REG_TMP0);
        gen_code_buffered_RM("ST", REG_TMP1, -2, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
    }
    | term
    ;

addop:
    SYMBOL_ADD | SYMBOL_SUB ;

term:
    term mulop factor
    {
        gen_comment_buffered("* mulop");
        gen_code_buffered_RM("LD", REG_TMP0, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP1, -2, REG_STACK_PTR);
        gen_code_buffered_RO($2 == SYMBOL_MUL ? "MUL" : "DIV", REG_TMP1, REG_TMP1, REG_TMP0);
        gen_code_buffered_RM("ST", REG_TMP1, -2, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
    }
    | factor
    ;

mulop:
    SYMBOL_MUL | SYMBOL_DIV ;

factor:
    SYMBOL_PARENTHESIS_L expression SYMBOL_PARENTHESIS_R
    | var
    {
        gen_comment_buffered("* load variable");
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP1, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LD", REG_TMP0, 0, REG_TMP1);
        gen_code_buffered_RM("ST", REG_TMP0, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);
    }
    | call
    {
        gen_code_buffered_RM("ST", REG_TMP0, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);
    }
    | MISC_NUM
    {
        gen_comment_buffered("* load instance value");
        gen_code_buffered_RM("LDC", REG_TMP0, $1, 0);
        gen_code_buffered_RM("ST", REG_TMP0, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);
    }
    ;

call:
    MISC_ID SYMBOL_PARENTHESIS_L args SYMBOL_PARENTHESIS_R 
    {
        /* maybe some checking first... */
        hentry_t *hfunc = htable_find(global_env->symbol_table, $1);
        if (!hfunc)
            parsing_error("undefined identifier: %s", $1);
        symbol_t *sfunc = hfunc->value;
        assert(sfunc);
        if (sfunc->type != SYMBOL_FUNC)
            parsing_error("target not callable: %s", $1);

        /* TODO: check parameter type */
        //assert(sfunc->func.type == TYPE_VOID);
        //assert(sfunc->func.paramsz == 0);

        gen_comment_buffered("* prepare for func call: %s", $1);
        gen_code_buffered_RM("LDC", REG_TMP0, 0, 0);
        gen_code_buffered_RM("LDA", REG_TMP1, 3, REG_PCOUNTER);
        gen_code_buffered_RM("ST", REG_TMP1, 0, REG_STACK_PTR);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, 1, REG_STACK_PTR);
        gen_code_buffered_RM("JEQ", REG_TMP0, sfunc->func.offset, REG_TMP0);

        /* clean the stack after invokation */
        gen_comment_buffered("* cleaning stack after func call: %s", $1);
        gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
        param_t *param;
        for (param = sfunc->func.param; param; param = param->next)
        {
            assert(param->arraysz != 0);
            if (param->arraysz == -1)
                gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
            else
                gen_code_buffered_RM("LDA", REG_STACK_PTR, -1, REG_STACK_PTR);
        }
    }
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


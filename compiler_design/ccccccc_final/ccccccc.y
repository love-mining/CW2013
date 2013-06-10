%{
/* 
 * This is part of the coursework for Compiler Design, 2013.05-06 
 * by Pengyu CHEN (cpy.prefers.you[at]gmail.com)
 * COPYLEFT, ALL WRONGS RESERVED.
 */
 
//#include "ccccccc.yy.h"
#ifndef NULL
#define NULL ((void*)0)
#endif

%}

%union /* typedef of yylval */
{
    char *str;
    long num;
}

%token <str>
    KEYWORD_ELSE KEYWORD_IF KEYWORD_INT KEYWORD_RETURN KEYWORD_VOID KEYWORD_WHILE
    MISC_ID

    SYMBOL_ADD SYMBOL_SUB SYMBOL_MUL SYMBOL_DIV SYMBOL_LT SYMBOL_LEQ SYMBOL_GT
    SYMBOL_GEQ SYMBOL_EEQL SYMBOL_NEQ SYMBOL_EQL 
    SYMBOL_SEMICOLON SYMBOL_COMMA SYMBOL_PARENTHESIS_L SYMBOL_PARENTHESIS_R
    SYMBOL_SQUARE_L SYMBOL_SQUARE_R SYMBOL_BRACKET_L SYMBOL_BRACKET_R

%token <num>
    MISC_NUM

%type <str>
    program
    declaration_list
    declaration
    var_declaration
    type_specifier
    fun_declaration
    params
    param_list
    param
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
    
    }
    | type_specifier MISC_ID SYMBOL_SQUARE_L MISC_NUM SYMBOL_SQUARE_R
        SYMBOL_SEMICOLON
    {
    
    }
    ;

type_specifier:
    KEYWORD_INT
    | KEYWORD_VOID
    ;

fun_declaration:
    type_specifier MISC_ID SYMBOL_PARENTHESIS_L params SYMBOL_PARENTHESIS_R 
        compound_stmt
    ;

params:
    param_list 
    | KEYWORD_VOID
    ;

param_list:
    param_list SYMBOL_COMMA param 
    | param
    ;

param:
    type_specifier MISC_ID
    | type_specifier MISC_ID SYMBOL_SQUARE_L SYMBOL_SQUARE_R
    ;

compound_stmt:
    SYMBOL_BRACKET_L local_declarations statement_list SYMBOL_BRACKET_R
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


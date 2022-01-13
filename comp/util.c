
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csua.h"
#include "visitor.h"

#define STATEMENT_LIST_STACK_BUFSIZE 10

static int statement_list_stack_size = 0;

static CS_Compiler *current_compiler = NULL;

void cs_set_current_compiler(CS_Compiler *compiler) {
    current_compiler = compiler;
}

CS_Compiler* cs_get_current_compiler() {
    return current_compiler;
}

DeclarationList* cs_chain_declaration(DeclarationList* decl_list, Declaration* decl) {
    DeclarationList* p;    
    DeclarationList* list = cs_create_declaration_list(decl);
    if (decl_list == NULL) return list;
    for (p = decl_list; p->next; p = p->next);
    p->next = list;
    return decl_list;
}

StatementList* cs_chain_statement_list(StatementList* stmt_list, Statement* stmt) {
    StatementList* p = NULL;
    StatementList* nstmt_list = cs_create_statement_list(stmt);
    if (stmt_list == NULL) {
        return nstmt_list;
    }   
    for (p = stmt_list; p->next; p = p->next);
    p->next = nstmt_list;
    
    return stmt_list;
}
/*
//statement_listのスタックの初期化
//ToDo 適宜この関数を実行
void cs_init_statement_list_stack(StatementList **statement_stack) {
    // 引数は&cs_get_current_compiler()->stmt_stack;
    statement_list_stack_size = STATEMENT_LIST_STACK_BUFSIZE;
    *statement_stack = (StatementList*)malloc(sizeof(StatementList)*statement_list_stack_size);
}

//スタックの先頭のstatement_listにstatementをつなげる。つなげたstatement_listの先頭のstatement_listを返す
void
*/


ElseIfStatementList* cs_chain_elsif_list(ElseIfStatementList* elsif_list, Expression *elsif_expr, Statement *elsif_block_stmt) {
    ElseIfStatementList* p = NULL;
    ElseIfStatementList* nelsif_list = cs_create_elsif_list(elsif_expr, elsif_block_stmt);
    if (elsif_list == NULL) {
        fprintf(stderr, "cs_chain_elsif_list: elsif_list == NULL\n");
        return nelsif_list;
    }
    for (p = elsif_list; p->next != NULL; p = p->next);
    p->next = nelsif_list;

    return elsif_list;
}


FunctionDeclarationList* cs_chain_function_declaration_list(FunctionDeclarationList* func_list, FunctionDeclaration* func) {
    FunctionDeclarationList* p = NULL;
    FunctionDeclarationList* nfunc_list = cs_create_function_declaration_list(func);
    if (func_list == NULL) {
        return nfunc_list;
    }
    for (p = func_list; p->next; p = p->next);
    p->next = nfunc_list;
    return func_list;
}

ParameterList* cs_chain_parameter_list(ParameterList* list, CS_BasicType type, char* name) {
    ParameterList* p = NULL;
    ParameterList* current = cs_create_parameter(type, name);
    for (p = list; p->next; p = p->next);
    p->next = current;
    return list;    
}

ArgumentList* cs_chain_argument_list(ArgumentList* list, Expression* expr) {
    ArgumentList* p;
    ArgumentList* current = cs_create_argument(expr);
    for (p = list; p->next; p = p->next);
    p->next = current;
    return list;
}



static Declaration* search_decls_from_list(DeclarationList* list, const char* name) {
    for(; list; list = list->next) {
        if (!strcmp(list->decl->name, name)) {
            return list->decl;
        }
    }
    return NULL;
}
// search from a block temporary
//Todo
Declaration* cs_search_decl_in_block() {
    return NULL;
}

Declaration* cs_search_decl_global(const char* name) {
    CS_Compiler* compiler = cs_get_current_compiler();
    return search_decls_from_list(compiler->decl_list, name);
}

static FunctionDeclaration* search_function_from_list(FunctionDeclarationList* list, const char* name) {
    for (;list; list = list->next) {
       if (!strcmp(list->func->name, name)) {
           return list->func;
       }
    }
    return NULL;
}

FunctionDeclaration* cs_search_function(const char* name) {
    CS_Compiler* compiler = cs_get_current_compiler();
    return search_function_from_list(compiler->func_list, name);
}

void print_ExpressionKind(int type) {
    switch (type) {
        case BOOLEAN_EXPRESSION: fprintf(stderr, "type = BOOLEAN_EXPRESSION\n"); break;
        case DOUBLE_EXPRESSION: fprintf(stderr, "type = DOUBLE_EXPRESSION\n"); break;
        case INT_EXPRESSION: fprintf(stderr, "type = INT_EXPRESSION\n"); break;
        case IDENTIFIER_EXPRESSION: fprintf(stderr, "type = IDENTIFIER_EXPRESSION\n"); break;
        case INCREMENT_EXPRESSION: fprintf(stderr, "type = INCREMENT_EXPRESSION\n"); break;
        case DECREMENT_EXPRESSION: fprintf(stderr, "type = DECREMENT_EXPRESSION\n"); break;
        case FUNCTION_CALL_EXPRESSION: fprintf(stderr, "type = FUNCTION_CALL_EXPRESSION\n"); break;
        case MINUS_EXPRESSION: fprintf(stderr, "type = MINUS_EXPRESSION\n"); break;
        case LOGICAL_NOT_EXPRESSION: fprintf(stderr, "type = LOGICAL_NOT_EXPRESSION\n"); break;
        case MUL_EXPRESSION: fprintf(stderr, "type = MUL_EXPRESSION\n"); break;
        case DIV_EXPRESSION: fprintf(stderr, "type = DIV_EXPRESSION\n"); break;
        case MOD_EXPRESSION: fprintf(stderr, "type = MOD_EXPRESSION\n"); break;
        case ADD_EXPRESSION: fprintf(stderr, "type = ADD_EXPRESSION\n"); break;
        case SUB_EXPRESSION: fprintf(stderr, "type = SUB_EXPRESSION\n"); break;
        case GT_EXPRESSION: fprintf(stderr, "type = GT_EXPRESSION\n"); break;
        case GE_EXPRESSION: fprintf(stderr, "type = GE_EXPRESSION\n"); break;
        case LT_EXPRESSION: fprintf(stderr, "type = LT_EXPRESSION\n"); break;
        case LE_EXPRESSION: fprintf(stderr, "type = LE_EXPRESSION\n"); break;
        case EQ_EXPRESSION: fprintf(stderr, "type = EQ_EXPRESSION\n"); break;
        case NE_EXPRESSION: fprintf(stderr, "type = NE_EXPRESSION\n"); break;
        case LOGICAL_AND_EXPRESSION: fprintf(stderr, "type = LOGICAL_AND_EXPRESSION\n"); break;
        case LOGICAL_OR_EXPRESSION: fprintf(stderr, "type = LOGICAL_OR_EXPRESSION\n"); break;
        case ASSIGN_EXPRESSION: fprintf(stderr, "type = ASSIGN_EXPRESSION\n"); break;
        case CAST_EXPRESSION: fprintf(stderr, "type = CAST_EXPRESSION\n"); break;
        case EXPRESSION_KIND_PLUS_ONE: fprintf(stderr, "type = EXPRESSION_KIND_PLUS_ONE\n"); break;
        default :
            fprintf(stderr, "type = ??\n");
    }
}

static int stack_max_size = 0;

uint32_t* create_unit32_t_stack() {
    stack_max_size = stack_max_size > 10 ? stack_max_size : 10;
    return (uint32_t*)MEM_malloc(sizeof(uint32_t)*stack_max_size);
}


uint32_t pop_label_replacement_point(CodegenVisitor *visitor) {
    if (visitor->stack_p <= 0) {
        return 0xffff;
    }
    //printf("stack_p = %d, return %d\n",visitor->stack_p-1, visitor->label_replacement_point_stack[visitor->stack_p-1]);
    return visitor->label_replacement_point_stack[--(visitor->stack_p)];
}

void push_label_replacement_point(CodegenVisitor *visitor, uint32_t label_replacement_point) {
    //printf("start push_label_replacement_point\n");
    //printf("%d, %d, %d\n", visitor->stack_p, stack_max_size, label_replacement_point);
    if (visitor->stack_p >= stack_max_size) {
        stack_max_size += 5;
        printf("push_label_replacement_point: MEM_realloc");
        visitor->label_replacement_point_stack = (uint32_t*)MEM_realloc(visitor->label_replacement_point_stack, sizeof(uint32_t) * stack_max_size);
    }

    visitor->label_replacement_point_stack[visitor->stack_p++] = label_replacement_point;
    /*
    for (int i=0; i<visitor->stack_p; ++i) {
        printf("%d ", visitor->label_replacement_point_stack[i]);
    }
    putchar('\n');
    printf("end push_label_replacement_point\n");
    */
}
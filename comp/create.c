

#include <stdio.h>
#include <string.h>
#include "csua.h"
#include "../memory/MEM.h"

static MEM_Storage storage = NULL;
static CS_Compiler* compiler = NULL;

static int *linenum = NULL;

/* for debug */
extern int current_line;

static void init_storage() {    
    if (storage == NULL) { 
#ifdef STORAGE                
        storage = MEM_open_storage(0);
        printf("init_storage\n");
        linenum = &current_line;
        
#else
        storage = cs_get_current_compiler()->storage;
        linenum = &cs_get_current_compiler()->current_line;
        
#endif
    }
}

void* cs_malloc(size_t size) {
    init_storage();
    return MEM_storage_malloc(storage, size);
}

static Expression* cs_create_expression(ExpressionKind ekind) {
    Expression* expr = (Expression*)cs_malloc(sizeof(Expression));    
    expr->kind = ekind;
    expr->type = NULL;
    expr->line_number = *linenum;
    return expr;
}


Expression* cs_create_int_expression(int v) {
    Expression* expr = cs_create_expression(INT_EXPRESSION);
    expr->u.int_value = v;
    return expr;
}

void delete_storage() {
#ifdef STORAGE            
    if (storage != NULL) {
        MEM_dispose(storage);
    }
#endif            
}

//使われない
ExpressionList* cs_chain_expression_list(ExpressionList* list, Expression* expr) {
    ExpressionList* p = list;
    ExpressionList* nlist= (ExpressionList*)MEM_storage_malloc(storage, sizeof(ExpressionList));    
    nlist->next = NULL;
    nlist->expression = expr;    
    if (p != NULL) {
        while (p->next) p = p->next;
        p->next = nlist;
        return list;
    } 
    return nlist;
    
}

Expression* cs_create_double_expression(double v) {
    Expression* expr = cs_create_expression(DOUBLE_EXPRESSION);
    expr->u.double_value = v;
    return expr;
}

Expression* cs_create_boolean_expression(CS_Boolean v) {
    Expression* expr = cs_create_expression(BOOLEAN_EXPRESSION);
    expr->u.boolean_value = v;
    return expr;
}

Expression* cs_create_identifier_expression(char* identifier) {
    Expression* expr = cs_create_expression(IDENTIFIER_EXPRESSION);
    expr->u.identifier.name = identifier;
    return expr;
}

Expression* cs_create_inc_dec_expression(Expression* id_expr, ExpressionKind inc_dec) {
    Expression* expr = cs_create_expression(inc_dec);
    expr->u.inc_dec = id_expr;
    return expr;
}

// args is argument not yet
Expression* cs_create_function_call_expression(Expression* function, ArgumentList* args) {
    Expression* expr = cs_create_expression(FUNCTION_CALL_EXPRESSION);
    expr->u.function_call_expression.function = function;
    expr->u.function_call_expression.argument = args;
    return expr;
}

Expression* cs_create_minus_expression(Expression* operand) {
    Expression* expr = cs_create_expression(MINUS_EXPRESSION);
    expr->u.minus_expression = operand;
    return expr;
}

Expression* cs_create_logical_not_expression(Expression* operand) {
    Expression* expr = cs_create_expression(LOGICAL_NOT_EXPRESSION);
    expr->u.logical_not_expression = operand;
    return expr;
}

Expression* cs_create_binary_expression(ExpressionKind kind, Expression* left, Expression* right) {
    Expression* expr = cs_create_expression(kind);
    expr->u.binary_expression.left = left;
    expr->u.binary_expression.right = right;
    return expr;   
}

Expression* cs_create_assignment_expression(Expression *left, AssignmentOperator aope, Expression* operand) {
    Expression* expr = cs_create_expression(ASSIGN_EXPRESSION);
    expr->u.assignment_expression.aope = aope;
    expr->u.assignment_expression.left = left;
    expr->u.assignment_expression.right = operand;
    return expr;            
}

Expression* cs_create_cast_expression(CS_CastType ctype, Expression* operand) {
    Expression* expr = cs_create_expression(CAST_EXPRESSION);
    expr->u.cast_expression.ctype = ctype;
    expr->u.cast_expression.expr = operand;
    return expr;
}
        
        
char* cs_create_identifier(const char* str) {
    char* new_char;
    new_char = (char*)cs_malloc(strlen(str) + 1);
    strcpy(new_char, str);
    return new_char;
}


/* For Statement */
static Statement* cs_create_statement(StatementType type) {
    //Expression* expr = (Expression*)cs_malloc(sizeof(Expression));   
    Statement* stmt = (Statement*)cs_malloc(sizeof(Statement));
    stmt->type = type;
    return stmt;    
}

Statement* cs_create_statement_block(StatementList* statement_list) {
    Statement* stmt = cs_create_statement(STATEMENT_BLOCK);
    stmt->u.statement_block = statement_list;
    return stmt;
}


Statement* cs_create_if_statement(Expression* if_expr, Statement* if_block_stmt, ElseIfStatementList* elsif_list, Statement* else_block_stmt) {
    IfStatement* if_stmt = (IfStatement*)cs_malloc(sizeof(IfStatement));

    if (elsif_list == NULL && else_block_stmt == NULL) {
        if_stmt->type = IF_ONLY;
    } else if (elsif_list == NULL) {
        if_stmt->type = IF_ELSE;
    } else if (else_block_stmt == NULL) {
        if_stmt->type = IF_ELSEIF;
    } else {
        if_stmt->type = IF_ELSEIF_ELSE;
    }

    if_stmt->if_expr = if_expr;
    if_stmt->if_block_stmt = if_block_stmt;
    if_stmt->elseif_stmt_list = elsif_list;
    if_stmt->else_block_stmt = else_block_stmt;
    
    Statement* stmt = cs_create_statement(IF_STATEMENT);
    stmt->u.ifstatement_s = if_stmt;
    return stmt;
}

ElseIfStatementList* cs_create_elsif_list(Expression *elsif_expr, Statement *elsif_block_stmt) {
    ElseIfStatement* elsif_stmt = (ElseIfStatement*)cs_malloc(sizeof(ElseIfStatement));
    elsif_stmt->expression_s = elsif_expr;
    elsif_stmt->stmt = elsif_block_stmt;
    ElseIfStatementList* elsif_list = (ElseIfStatementList*)cs_malloc(sizeof(ElseIfStatementList));
    elsif_list->elseIfStatement = elsif_stmt;
    elsif_list->next = NULL;
    return elsif_list;
}

//create while
Statement* cs_create_while_statement(Expression *while_expr, Statement* while_block_stmt){
	WhileStatement *while_stmt = (WhileStatement*)cs_malloc(sizeof(WhileStatement));
	while_stmt->while_expr = while_expr;
	while_stmt->while_block_stmt = while_block_stmt;
	Statement* stmt = cs_create_statement(WHILE_STATEMENT);
	stmt->u.whilestatement_s = while_stmt;
	return stmt;
}

Statement* cs_create_expression_statement(Expression* expr) {
    Statement* stmt = cs_create_statement(EXPRESSION_STATEMENT);
    stmt->u.expression_s = expr;
    return stmt;
}


TypeSpecifier* cs_create_type_specifier(CS_BasicType type) {
    TypeSpecifier* ts = (TypeSpecifier*)cs_malloc(sizeof(TypeSpecifier));
    ts->basic_type = type;

    return ts;
}

ParameterList* cs_create_parameter(CS_BasicType type, char* name) {
    ParameterList* param = (ParameterList*)cs_malloc(sizeof(ParameterList));
    param->type = cs_create_type_specifier(type);
    param->name = name;
    param->line_number = *linenum;
    param->next = NULL;
    return param;
}

static Declaration* cs_create_declaration(CS_BasicType type, char* name, Expression* initializer) {
    Declaration* decl = (Declaration*)cs_malloc(sizeof(Declaration));
    decl->type = cs_create_type_specifier(type);
    decl->name = name;
    decl->initializer = initializer;
    decl->index = -1;
    return decl;        
}

Statement* cs_create_declaration_statement(CS_BasicType type, char* name, Expression* initializer) {
    Statement* stmt = cs_create_statement(DECLARATION_STATEMENT);
    stmt->u.declaration_s = cs_create_declaration(type, name, initializer);   
    return stmt;    
}


StatementList* cs_create_statement_list(Statement* stmt) {
    StatementList* stmt_list = (StatementList*)cs_malloc(sizeof(StatementList));
    stmt_list->stmt = stmt;
    stmt_list->next = NULL;
    return stmt_list;
}

DeclarationList* cs_create_declaration_list(Declaration* decl) {
    DeclarationList* list = cs_malloc(sizeof(DeclarationList));
    list->next = NULL;
    list->decl = decl; 
    return list;
}


FunctionDeclaration* cs_create_function_declaration(CS_BasicType type, char *name, ParameterList* param) {
    FunctionDeclaration* decl = (FunctionDeclaration*)cs_malloc(sizeof(FunctionDeclaration));
    decl->type = cs_create_type_specifier(type);
    decl->name = name;
    decl->param = param;
    decl->index = -1;
    return decl;
}

FunctionDeclarationList* cs_create_function_declaration_list(FunctionDeclaration* func) {
    FunctionDeclarationList* list = cs_malloc(sizeof(FunctionDeclarationList));
    list->next = NULL;
    list->func = func;
    return list;
}

ArgumentList* cs_create_argument(Expression* expr) {
    ArgumentList* argument = cs_malloc(sizeof(ArgumentList));
    argument->expr = expr;
    argument->next = NULL;
    return argument;
}




#include <stdio.h>
#include <stdlib.h>

#include "visitor.h"

static void traverse_expr_children(Expression* expr, Visitor* visitor);
static void traverse_stmt_children(Statement*  stmt, Visitor* visitor);

void traverse_stmt_list(StatementList* stmt_list, Visitor* visitor) {
    while(stmt_list != NULL) {
        traverse_stmt(stmt_list->stmt, visitor);
        stmt_list = stmt_list->next;
    }
}

void traverse_expr(Expression* expr, Visitor* visitor) {
    if (expr) {
        if (visitor->enter_expr_list[expr->kind] == NULL) {
            fprintf(stderr, "enter->type(%d) is null\n", expr->kind);
            exit(1);
        }

        visitor->enter_expr_list[expr->kind](expr, visitor);
        traverse_expr_children(expr, visitor);
        visitor->leave_expr_list[expr->kind](expr, visitor);

    }
}

void traverse_stmt(Statement* stmt, Visitor* visitor) {
    if (stmt) {
        if (visitor->enter_stmt_list[stmt->type] == NULL) {
            fprintf(stderr, "enter->type(%d) is null\n", stmt->type);
            exit(1);
        }
        //codegenvisitorのifは
        //PUSH_LABEL IF全体の終端のラベルを張る位置を記憶
        visitor->enter_stmt_list[stmt->type](stmt, visitor);
        traverse_stmt_children(stmt, visitor);
        //codegenvisitorのifは
        //最後の命令をラベルpop命令に。IF全体の終端のlabelを張る
        visitor->leave_stmt_list[stmt->type](stmt, visitor);
    }
}

static void traverse_if_stmt(Statement* stmt, Visitor* visitor) {
    IfStatement *ifstmt = stmt->u.ifstatement_s;
    IfStatementType type = ifstmt->type;

    //fprintf(stderr, "enter if block\n");

    
    if (visitor->if_codegen_expr_list) {
        //codegenvisitor
        //PUSH_LABEL 次のブロックの先頭のラベルを張る位置を記憶
        visitor->if_codegen_expr_list[ENTER_IF_EXPR](ifstmt->if_expr, visitor);
        traverse_expr(ifstmt->if_expr, visitor);
        //条件JUMP命令
        visitor->if_codegen_expr_list[LEAVE_IF_EXPR](ifstmt->if_expr, visitor);
    } else {
        traverse_expr(ifstmt->if_expr, visitor);
    }

    if (visitor->if_codegen_stmt_list) {
        //codegenvisitor
        visitor->if_codegen_stmt_list[ENTER_INNER_IF](ifstmt->if_block_stmt, visitor);
        traverse_stmt_list(ifstmt->if_block_stmt->u.statement_block, visitor);
        //強制JUMMP命令
        //次のブロックの先頭のラベルを記憶位置に張る
        visitor->if_codegen_stmt_list[LEAVE_INNER_IF](ifstmt->if_block_stmt, visitor);
    } else {
        traverse_stmt_list(ifstmt->if_block_stmt->u.statement_block, visitor);
    }

    //fprintf(stderr, "leave if block\n");

    if (type == IF_ELSEIF || type == IF_ELSEIF_ELSE) {
        ElseIfStatementList *elsif_list = ifstmt->elseif_stmt_list;
        while (elsif_list) {
            //fprintf(stderr, "enter elsif\n");
            if (visitor->if_codegen_expr_list) {
                //codegenvisitor
                //次のブロックの先頭のラベルを張る位置を記憶
                visitor->if_codegen_expr_list[ENTER_ELSIF_EXPR](elsif_list->elseIfStatement->expression_s, visitor);
                traverse_expr(elsif_list->elseIfStatement->expression_s, visitor);
                //条件JUMP命令
                visitor->if_codegen_expr_list[LEAVE_ELSIF_EXPR](elsif_list->elseIfStatement->expression_s, visitor);
            } else {
                traverse_expr(elsif_list->elseIfStatement->expression_s, visitor);
            }

            if (visitor->if_codegen_stmt_list) {
                visitor->if_codegen_stmt_list[ENTER_INNER_ELSIF](elsif_list->elseIfStatement->stmt, visitor);
                traverse_stmt_list(elsif_list->elseIfStatement->stmt->u.statement_block, visitor);
                //強制JUMP命令
                //次のブロックの先頭のラベルを記憶位置に張る
                visitor->if_codegen_stmt_list[LEAVE_INNER_ELSIF](elsif_list->elseIfStatement->stmt, visitor);
            } else {
                traverse_stmt_list(elsif_list->elseIfStatement->stmt->u.statement_block, visitor);
            }

            elsif_list = elsif_list->next;
            //fprintf(stderr, "leave elsif\n");
        }
    }

    if (type == IF_ELSE || type == IF_ELSEIF_ELSE) {
        //fprintf(stderr, "enter else\n");

        if (visitor->if_codegen_stmt_list) {
            visitor->if_codegen_stmt_list[ENTER_INNER_ELSE](ifstmt->else_block_stmt, visitor);
            traverse_stmt_list(ifstmt->else_block_stmt->u.statement_block, visitor);
            //ラベルpop命令
            visitor->if_codegen_stmt_list[LEAVE_INNER_ELSE](ifstmt->else_block_stmt, visitor);
        } else {
            traverse_stmt_list(ifstmt->else_block_stmt->u.statement_block, visitor);
        }

        //fprintf(stderr, "leave else\n");
    }
    
}

static void traverse_stmt_children(Statement* stmt, Visitor* visitor) {
    switch(stmt->type) {
        case EXPRESSION_STATEMENT: {
            traverse_expr(stmt->u.expression_s, visitor);
          break;  
        }
        case DECLARATION_STATEMENT: {
            traverse_expr(stmt->u.declaration_s->initializer, visitor);
            break;
        }
        case IF_STATEMENT: {
            traverse_if_stmt(stmt, visitor);
            break;
        }
        case STATEMENT_BLOCK: {
            traverse_stmt_list(stmt->u.statement_block, visitor);
            break;
        }
        default: {
            fprintf(stderr, "No such stmt->type %d in traverse_stmt_children\n", stmt->type);
        }
    }
}

static void traverse_expr_children(Expression* expr, Visitor *visitor) {
    switch(expr->kind) {
        case BOOLEAN_EXPRESSION:
        case IDENTIFIER_EXPRESSION:
        case DOUBLE_EXPRESSION:
        case INT_EXPRESSION: {
            break;
        }

        case INCREMENT_EXPRESSION:
        case DECREMENT_EXPRESSION: {
            traverse_expr(expr->u.inc_dec, visitor);
            break;
        }
        case MINUS_EXPRESSION: {
            traverse_expr(expr->u.minus_expression, visitor);
            break;
        }
        case LOGICAL_NOT_EXPRESSION: {
            traverse_expr(expr->u.logical_not_expression, visitor);
            break;
        }
        case ASSIGN_EXPRESSION: {
            traverse_expr(expr->u.assignment_expression.right, visitor);
            if (visitor->notify_expr_list) {
                if (visitor->notify_expr_list[ASSIGN_EXPRESSION]) {
                    visitor->notify_expr_list[ASSIGN_EXPRESSION](expr, visitor);
                }
            }
            
            
            traverse_expr(expr->u.assignment_expression.left, visitor);
            break;
        }
        case CAST_EXPRESSION: {
            traverse_expr(expr->u.cast_expression.expr, visitor);
            break;
        }
        case FUNCTION_CALL_EXPRESSION: {
//            printf("function call!\n");
            ArgumentList* args = expr->u.function_call_expression.argument;
            if (args) {
                for (; args; args=args->next) {
                    traverse_expr(args->expr, visitor);
                }
            }
            traverse_expr(expr->u.function_call_expression.function, visitor);            
            break;
        }
        case LOGICAL_AND_EXPRESSION:
        case LOGICAL_OR_EXPRESSION:            
        case LT_EXPRESSION:
        case LE_EXPRESSION:
        case GT_EXPRESSION:
        case GE_EXPRESSION:
        case EQ_EXPRESSION:
        case NE_EXPRESSION:            
        case MOD_EXPRESSION:
        case DIV_EXPRESSION:
        case MUL_EXPRESSION:
        case SUB_EXPRESSION:
        case ADD_EXPRESSION: {
            if (expr->u.binary_expression.left) {
                traverse_expr(expr->u.binary_expression.left, visitor);
            }
            if (expr->u.binary_expression.right) {
                traverse_expr(expr->u.binary_expression.right, visitor);
            }
            break;
        }
        default:
            fprintf(stderr, "No such expr->kind %d in traverse_expr_children\n", expr->kind);
    }
}
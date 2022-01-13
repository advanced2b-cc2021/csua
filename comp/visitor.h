
#ifndef _VISITOR_H_
#define _VISITOR_H_

#include "csua.h"

//typedef void (*visit_expr)(Expression* expr);
//typedef void (*visit_stmt)(Statement*  stmt);

typedef void (*visit_expr)(Expression* expr, Visitor* visitor);
typedef void (*visit_stmt)(Statement*  stmt, Visitor* visitor);
//typedef void (*visit_inside_ifstmt)(StatementList* sta)




typedef struct MeanCheckLog_tag {
    char                    *log_str;
    struct MeanCheckLog_tag *next;
} MeanCheckLogger;

struct Visitor_tag {
    visit_expr* enter_expr_list;
    visit_expr* leave_expr_list;
    
    visit_expr* notify_expr_list;
    
    visit_stmt* enter_stmt_list;
    visit_stmt* leave_stmt_list;

    visit_stmt* if_codegen_stmt_list;
    visit_expr* if_codegen_expr_list;
};

struct MeanVisitor_tag {
    Visitor visitor;
    CS_Compiler *compiler;
    int i;
    int j;
    MeanCheckLogger *check_log;
};

typedef enum {
    VISIT_NORMAL,
    VISIT_NOMAL_ASSIGN,
} VisitIdentState;

typedef enum {
    VISIT_F_NO,
    VISIT_F_CALL,    
} VisitFunCallState;

typedef enum {
    ENTER_IF_EXPR,
    LEAVE_IF_EXPR,
    ENTER_ELSIF_EXPR,
    LEAVE_ELSIF_EXPR,
    IF_CODEGEN_EXPR_TYPE_PLUS_ONE,
} IF_CODEGEN_EXPR_TYPE;

typedef enum {
    ENTER_INNER_IF,
    LEAVE_INNER_IF,
    ENTER_INNER_ELSIF,
    LEAVE_INNER_ELSIF,
    ENTER_INNER_ELSE,
    LEAVE_INNER_ELSE,
    IF_CODEGEN_STMT_TYPE_PLUS_ONE,
} IF_CODEGEN_STMT_TYPE;

struct CodegenVisitor_tag {
    Visitor        visitor;
    CS_Compiler   *compiler;
    CS_Executable *exec;
    
    VisitIdentState     vi_state;
    VisitFunCallState       vf_state;
    uint16_t       assign_depth;
    
    uint32_t       *label_replacement_point_stack;
    int            stack_p;
    uint32_t       CODE_ALLOC_SIZE;
    uint32_t       current_code_size;   //確保した領域サイズ
    uint32_t       pos; //次コードを書き込む位置
    uint8_t        *code;
};

/* visitor.c */
void print_depth();
Visitor* create_treeview_visitor();
void delete_visitor(Visitor* visitor);
void traverse_expr(Expression* expr, Visitor* visitor);
void traverse_stmt(Statement*  stmt, Visitor* visitor);
void traverse_stmt_list(StatementList* stmt_list, Visitor* visitor);
char* get_ifstmt_type_name(IfStatementType type);

/* mean_visitor */
MeanVisitor* create_mean_visitor();
void show_mean_error(MeanVisitor* visitor);
char* get_type_name(CS_BasicType type);

/* codegen_visitor */
CodegenVisitor* create_codegen_visitor(CS_Compiler* compiler, CS_Executable *exec);

/* util.c */
uint32_t* create_unit32_t_stack();
uint32_t pop_label_replacement_point(CodegenVisitor *visitor);
void push_label_replacement_point(CodegenVisitor *visitor, uint32_t label_replacement_point);

#endif
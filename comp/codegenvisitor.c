#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "csua.h"
#include "visitor.h"
#include "../svm/svm.h"


static size_t get_opsize(OpcodeInfo *op) {
    size_t size = 0;
    for (int j = 0; j < strlen(op->parameter); ++j) {
        switch (op->parameter[j]) {
            case 'i': {
                size += 2;
                break;
            }
            case '4': {
                size += 4;
                break;
            }
            default: {
                fprintf(stderr, "unknown parameter [%c]in disassemble\n", op->parameter[j]);
                break;
            }
        }
    }
    return size;
}


static void gen_byte_code(CodegenVisitor* visitor, SVM_Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    
    OpcodeInfo oInfo = svm_opcode_info[op];
    printf("-->%s\n", oInfo.opname);
    printf("-->%s\n", oInfo.parameter);
    //printf("%d  %d  %d\n", (visitor->pos + 1 + 1 + (get_opsize(&oInfo))), visitor->current_code_size, (visitor->current_code_size + visitor->CODE_ALLOC_SIZE));
    // pos + 1byte + operator (1byte) + operand_size
    if ((visitor->pos + 1 + 1 + (get_opsize(&oInfo))) > visitor->current_code_size) {
        visitor->code = MEM_realloc(visitor->code, visitor->current_code_size += visitor->CODE_ALLOC_SIZE);        
    }
    
    visitor->code[visitor->pos++] = op & 0xff;
    
    for (int i = 0; i < strlen(oInfo.parameter); ++i) {
        switch(oInfo.parameter[i]) {
            case 'i': { // 2byte index
                int operand = va_arg(ap, int);
                visitor->code[visitor->pos++] = (operand >> 8) & 0xff;
                visitor->code[visitor->pos++] = operand        & 0xff;                
                break;
            }
            case '4': {
                uint32_t operand = va_arg(ap, uint32_t);
                visitor->code[visitor->pos++] = (operand >> 24) & 0xff;
                visitor->code[visitor->pos++] = (operand >> 16) & 0xff;
                visitor->code[visitor->pos++] = (operand >>  8) & 0xff;
                visitor->code[visitor->pos++] = (operand      ) & 0xff;
                break;
            }
            default: {
                fprintf(stderr, "undefined parameter\n");
                exit(1);
                break;
            }
        }
    }    
    va_end(ap);    
}

static void skip_4byte_code(CodegenVisitor* visitor) {
    visitor->pos += 4;
    if (visitor->pos >= visitor->current_code_size) {
        visitor->code = MEM_realloc(visitor->code,
                visitor->current_code_size + visitor->CODE_ALLOC_SIZE);
    }
}

static void replace_4byte_code(CodegenVisitor *visitor, uint32_t replace_point, uint32_t label) {
    //printf("%d %d %d\n", replace_point, label, visitor->pos);
    visitor->code[replace_point  ] = (label >> 24) & 0xff;
    visitor->code[replace_point+1] = (label >> 16) & 0xff;
    visitor->code[replace_point+2] = (label >>  8) & 0xff;
    visitor->code[replace_point+3] = (label      ) & 0xff;
}



static int add_constant(CS_Executable* exec, CS_ConstantPool* cpp) {
    exec->constant_pool = MEM_realloc(exec->constant_pool, 
            sizeof(CS_ConstantPool) * (exec->constant_pool_count+1));
    exec->constant_pool[exec->constant_pool_count] = *cpp;
    return exec->constant_pool_count++;
}


static void enter_castexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter castexpr : %d\n", expr->u.cast_expression.ctype);
}
static void leave_castexpr(Expression* expr, Visitor* visitor) { 
    fprintf(stderr, "leave castexpr\n");
    switch(expr->u.cast_expression.ctype) {
        case CS_INT_TO_DOUBLE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_CAST_INT_TO_DOUBLE);
            break;
        }
        case CS_DOUBLE_TO_INT: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_CAST_DOUBLE_TO_INT);            
            break;
        }
        default: {
            fprintf(stderr, "unknown cast type in codegenvisitor\n");
            exit(1);
        }
    }
    
//    exit(1);
}

static void enter_boolexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter boolexpr : %d\n", expr->u.boolean_value);
}
static void leave_boolexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave boolexpr\n");
    CS_Executable* exec = ((CodegenVisitor*)visitor)->exec;
    CS_ConstantPool cp;
    cp.type = CS_CONSTANT_INT;
    if (expr->u.boolean_value == CS_FALSE) {
        cp.u.c_int = 0;
    } else {
        cp.u.c_int = 1;
    }
    int idx = add_constant(exec, &cp);
    gen_byte_code((CodegenVisitor*)visitor, SVM_PUSH_INT, idx);
}


static void enter_intexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter intexpr : %d\n", expr->u.int_value);

}
static void leave_intexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave intexpr\n");
    CS_Executable* exec = ((CodegenVisitor*)visitor)->exec;
    CS_ConstantPool cp;
    cp.type = CS_CONSTANT_INT;
    cp.u.c_int = expr->u.int_value;
    int idx = add_constant(exec, &cp);
    gen_byte_code((CodegenVisitor*)visitor, SVM_PUSH_INT, idx);    
}

static void enter_doubleexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter doubleexpr : %f\n", expr->u.double_value);
}
static void leave_doubleexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave doubleexpr\n");
    CS_Executable* exec = ((CodegenVisitor*)visitor)->exec;
    CS_ConstantPool cp;
    cp.type = CS_CONSTANT_DOUBLE;
    cp.u.c_double = expr->u.double_value;
    int idx = add_constant(exec, &cp);
    gen_byte_code((CodegenVisitor*)visitor, SVM_PUSH_DOUBLE, idx); 
}

static void enter_identexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter identifierexpr : %s\n", expr->u.identifier.name);
}
static void leave_identexpr(Expression* expr, Visitor* visitor) {
    //fprintf(stderr, "leave identifierexpr\n");            
    CodegenVisitor* c_visitor = (CodegenVisitor*)visitor;
    switch (c_visitor->vi_state) {
        case VISIT_NORMAL: {
//            fprintf(stderr, "push value to stack\n");
            if (expr->u.identifier.is_function) {
//                printf("name=%s, index=%d\n", 
//                        expr->u.identifier.u.function->name,
//                        expr->u.identifier.u.function->index);
                gen_byte_code(c_visitor, SVM_PUSH_FUNCTION,
                        expr->u.identifier.u.function->index);
            } else {
                switch(expr->type->basic_type) {
                    case CS_BOOLEAN_TYPE:
                    case CS_INT_TYPE: {
                        gen_byte_code(c_visitor, SVM_PUSH_STATIC_INT,
                                expr->u.identifier.u.declaration->index);
                        break;
                    }
                    case CS_DOUBLE_TYPE: {
//                        fprintf(stderr, "double not implementerd visit_nomal in leave_identexpr codegenvisitor\n");
//                        exit(1);                        
                        gen_byte_code(c_visitor, SVM_PUSH_STATIC_DOUBLE,
                                expr->u.identifier.u.declaration->index);
                        break;
                        
                                                
                    }
                    default: {
                        fprintf(stderr, "%d: unknown type in visit_normal in leave_identexpr codegenvisitor\n", expr->line_number); 
                        exit(1);
                    }
                }
            }
            break;
        }
        case VISIT_NOMAL_ASSIGN: {

            // Variable is not a function, then store the value
            if (!expr->u.identifier.is_function) {
//                fprintf(stderr, "index = %d\n", expr->u.identifier.u.declaration->index);
//                fprintf(stderr, "type = %s\n", get_type_name(expr->type->basic_type));
                switch (expr->type->basic_type) {
                    case CS_BOOLEAN_TYPE:
                    case CS_INT_TYPE:    {
                        gen_byte_code(c_visitor, SVM_POP_STATIC_INT, 
                                expr->u.identifier.u.declaration->index);
                        break;
                    }
                    case CS_DOUBLE_TYPE: {
//                        fprintf(stderr, "double not implementerd assign in leave_identexpr codegenvisitor\n");
//                        exit(1);
                        gen_byte_code(c_visitor, SVM_POP_STATIC_DOUBLE,
                                expr->u.identifier.u.declaration->index);                      
                        break;
                    }
                    default: {
                        fprintf(stderr, "unknown type in leave_identexpr codegenvisitor\n");
                        exit(1);
                        gen_byte_code(c_visitor, SVM_POP_STATIC_DOUBLE,
                                expr->u.identifier.u.declaration->index);
                        break;
                    }
                }
            } else {
                fprintf(stderr, "%d: cannot assign value to function\n", expr->line_number);
                exit(1);
            }
            
//            if (c_visitor->assign_depth > 1) { // nested assign
            if ((c_visitor->assign_depth > 1) || (c_visitor->vf_state == VISIT_F_CALL)) { // nested assign or inside function call
                switch(expr->type->basic_type) {
                    case CS_BOOLEAN_TYPE:
                    case CS_INT_TYPE: {
                        gen_byte_code(c_visitor, SVM_PUSH_STATIC_INT,
                                expr->u.identifier.u.declaration->index);
                        break;
                    }
                    case CS_DOUBLE_TYPE: {
//                        fprintf(stderr, "double not implementerd assign_depth in leave_identexpr codegenvisitor\n");
//                        exit(1);
                        gen_byte_code(c_visitor, SVM_PUSH_STATIC_DOUBLE,
                                expr->u.identifier.u.declaration->index);  
                        break;
                    }
                    default: {
                        fprintf(stderr, "%d: unknown type in leave_identexpr codegenvisitor\n", expr->line_number); 
                        exit(1);
                    }
                }
            }                                                            
            
            break;
        }
        default: {
            fprintf(stderr, "no such v_state error\n");
            exit(1);
        }
    }
}


static void enter_addexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter addexpr : +\n");
}
static void leave_addexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave addexpr\n");
    switch(expr->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_ADD_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_ADD_DOUBLE);
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_addexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
        
    }
}

static void enter_subexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter subexpr : -\n");
}
static void leave_subexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave subexpr\n");
    switch(expr->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_SUB_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_SUB_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_subexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }    
}

static void enter_mulexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter mulexpr : *\n");
}
static void leave_mulexpr(Expression* expr, Visitor* visitor) {
    switch(expr->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_MUL_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_MUL_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_subexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }     
}

static void enter_divexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter divexpr : /\n");
}
static void leave_divexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave divexpr\n");
    switch(expr->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_DIV_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_DIV_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_subexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }
}

static void enter_modexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter modexpr : mod \n");
}
static void leave_modexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave modexpr\n");
    switch(expr->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_MOD_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_MOD_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_subexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }
}


static void enter_gtexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter gtexpr : > \n");
}
static void leave_gtexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave gtexpr\n");   
    switch(expr->u.binary_expression.left->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_GT_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_GT_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_gtexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }       
}

static void enter_geexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter geexpr : >= \n");
    
}
static void leave_geexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave geexpr\n");
     switch(expr->u.binary_expression.left->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_GE_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_GE_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_geexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }    
}

static void enter_ltexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter ltexpr : < \n");
}
static void leave_ltexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave ltexpr\n");
    switch(expr->u.binary_expression.left->type->basic_type) {
//    switch(expr->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_LT_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_LT_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_ltexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }    
}

static void enter_leexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter leexpr : <= \n");
}
static void leave_leexpr(Expression* expr, Visitor* visitor) {
    switch(expr->u.binary_expression.left->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_LE_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_LE_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_leexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }        
}

static void enter_eqexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter eqexpr : == \n");
}
static void leave_eqexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave eqexpr\n");
    switch(expr->u.binary_expression.left->type->basic_type) {
        case CS_BOOLEAN_TYPE:
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_EQ_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_EQ_DOUBLE);
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_eqexpr codegenvisitor\n", expr->line_number);             
            exit(1);
        }
    }       
}

static void enter_neexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter neexpr : != \n");
}
static void leave_neexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave neexpr\n");
    switch(expr->u.binary_expression.left->type->basic_type) {
        case CS_BOOLEAN_TYPE:
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_NE_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_NE_DOUBLE);
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_eqexpr codegenvisitor\n", expr->line_number);             
            exit(1);
        }
    }     
}

static void enter_landexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter landexpr : && \n");
}
static void leave_landexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave landexpr\n");
    gen_byte_code((CodegenVisitor*)visitor, SVM_LOGICAL_AND);
}

static void enter_lorexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter lorexpr : || \n");
}
static void leave_lorexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave lorexpr\n");
    gen_byte_code((CodegenVisitor*)visitor, SVM_LOGICAL_OR);    
}

static void enter_incexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter incexpr : ++ \n");
}
static void leave_incexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave incexpr\n");
    
    if (((CodegenVisitor*)visitor)->vi_state != VISIT_NORMAL) {
        fprintf(stderr, "expression is not assignable\n");
        exit(1);
    }
    
   gen_byte_code((CodegenVisitor*)visitor, SVM_INCREMENT);
   gen_byte_code((CodegenVisitor*)visitor, SVM_POP_STATIC_INT, 
           expr->u.inc_dec->u.identifier.u.declaration->index);
   gen_byte_code((CodegenVisitor*)visitor, SVM_PUSH_STATIC_INT, 
           expr->u.inc_dec->u.identifier.u.declaration->index);
   
   
}

static void enter_decexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter decexpr : -- \n");
}
static void leave_decexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave decexpr\n");
    if (((CodegenVisitor*)visitor)->vi_state != VISIT_NORMAL) {
        fprintf(stderr, "expression is not assignable\n");
        exit(1);
    }
    
    gen_byte_code((CodegenVisitor*)visitor, SVM_DECREMENT);
    gen_byte_code((CodegenVisitor*)visitor, SVM_POP_STATIC_INT, 
            expr->u.inc_dec->u.identifier.u.declaration->index);
    gen_byte_code((CodegenVisitor*)visitor, SVM_PUSH_STATIC_INT, 
            expr->u.inc_dec->u.identifier.u.declaration->index);
}

static void enter_minusexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter minusexpr : - \n");
}
static void leave_minusexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave minusexpr\n");
    switch(expr->type->basic_type) {
        case CS_INT_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_MINUS_INT);
            break;
        }
        case CS_DOUBLE_TYPE: {
            gen_byte_code((CodegenVisitor*)visitor, SVM_MINUS_DOUBLE);            
            break;
        }
        default: {
            fprintf(stderr, "%d: unknown type in leave_minusexpr codegenvisitor\n", expr->line_number); 
            exit(1);
        }
    }    
}

static void enter_lognotexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter lognotexpr : ! \n");
}
static void leave_lognotexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave lognotexpr\n");    
    gen_byte_code((CodegenVisitor*)visitor, SVM_LOGICAL_NOT); 
}

static void enter_assignexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter assignexpr : %d \n", expr->u.assignment_expression.aope);
    
//    printf("ope = %d\n", expr->u.assignment_expression.aope);
    if (expr->u.assignment_expression.aope != ASSIGN) {
        if (expr->u.assignment_expression.left->kind == IDENTIFIER_EXPRESSION &&
                expr->u.assignment_expression.left->u.identifier.is_function == CS_FALSE) {
            gen_byte_code((CodegenVisitor*)visitor, SVM_PUSH_STATIC_INT,
                    expr->u.assignment_expression.left->u.identifier.u.declaration->index);
            
//gen_byte_code((CodegenVisitor*)visitor, SVM_PUSH_STATIC_INT, 
//           expr->u.inc_dec->u.identifier.u.declaration->index);            
            
        }
    }
    
    
    ((CodegenVisitor*)visitor)->assign_depth++;
}
static void leave_assignexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave assignexpr\n");
    CodegenVisitor* c_visitor = (CodegenVisitor*)visitor;
    
    --c_visitor->assign_depth;
    if (c_visitor->vf_state == VISIT_F_CALL) {
        c_visitor->vi_state = VISIT_NORMAL;
        c_visitor->assign_depth = 0;      
    }    
}

static void notify_assignexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "NOTIFY assignexpr : %d \n", expr->u.assignment_expression.aope);

    switch(expr->u.assignment_expression.aope) {
        case ADD_ASSIGN: {
            switch(expr->u.assignment_expression.right->type->basic_type) {
                case CS_INT_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_ADD_INT);
                    break;
                }
                case CS_DOUBLE_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_ADD_DOUBLE);                    
                    break;
                }
                default: {
                    exit(1);
                }
            }                    
            break;
        }
        case SUB_ASSIGN: {
            switch(expr->u.assignment_expression.right->type->basic_type) {
                case CS_INT_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_SUB_INT);
                    break;
                }
                case CS_DOUBLE_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_SUB_DOUBLE);                    
                    break;
                }
                default: {
                    exit(1);
                }
            }
            break;
        }
        case MUL_ASSIGN: {
            switch(expr->u.assignment_expression.right->type->basic_type) {
                case CS_INT_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_MUL_INT);
                    break;
                }
                case CS_DOUBLE_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_MUL_DOUBLE);                    
                    break;
                }
                default: {
                    exit(1);
                }
            }
            break;
        }
        case DIV_ASSIGN: {
            switch(expr->u.assignment_expression.right->type->basic_type) {
                case CS_INT_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_DIV_INT);
                    break;
                }
                case CS_DOUBLE_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_DIV_DOUBLE);                    
                    break;
                }
                default: {
                    exit(1);
                }
            }
            break;
        }
        case MOD_ASSIGN: {
            switch(expr->u.assignment_expression.right->type->basic_type) {
                case CS_INT_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_MOD_INT);
                    break;
                }
                case CS_DOUBLE_TYPE: {
                    gen_byte_code((CodegenVisitor*)visitor, SVM_MOD_DOUBLE);                    
                    break;
                }
                default: {
                    exit(1);
                }
            }
            break;
        }
        case ASSIGN: {
            break;
        }
        case ASSIGN_PLUS_ONE:
        defualt: {
            fprintf(stderr, "unsuuuport4ed assign operator\n");
            exit(1);
        }
    }

    ((CodegenVisitor*)visitor)->vi_state = VISIT_NOMAL_ASSIGN;

    
    
    
}

static void enter_funccallexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "enter function call :\n");
    ((CodegenVisitor*)visitor)->vf_state = VISIT_F_CALL;
    
}
static void leave_funccallexpr(Expression* expr, Visitor* visitor) {
//    fprintf(stderr, "leave function call\n");
    ((CodegenVisitor*)visitor)->vf_state = VISIT_F_NO;    
    gen_byte_code((CodegenVisitor*)visitor, SVM_INVOKE);
}

/* For statement */
static void enter_exprstmt(Statement* stmt, Visitor* visitor) {
//    fprintf(stderr, "enter exprstmt :\n");

}
static void leave_exprstmt(Statement* stmt, Visitor* visitor) {
//    fprintf(stderr, "leave exprstmt\n");
    
    CodegenVisitor* c_visitor = (CodegenVisitor*)visitor;
    switch (c_visitor->vi_state) {
        case VISIT_NORMAL: {
            gen_byte_code(c_visitor, SVM_POP);
            break;
        }
        case VISIT_NOMAL_ASSIGN: {            
            c_visitor->vi_state = VISIT_NORMAL;
            c_visitor->assign_depth = 0;
            break;
        }
        default: {
            fprintf(stderr, "no such visit state in leave_exprstmt\n");
            break;
        }
    }
    
//    ((CodegenVisitor*)visitor)->v_state = VISIT_NORMAL;
    
    
}

static void enter_declstmt(Statement* stmt, Visitor* visitor) {
//    fprintf(stderr, "enter declstmt name=%s, type=%s:\n", 
//            stmt->u.declaration_s->name,
//            get_type_name(stmt->u.declaration_s->type->basic_type));
            
}

static void leave_declstmt(Statement* stmt, Visitor* visitor) {
//    fprintf(stderr, "leave declstmt\n");
//初期化子が指定されているならそれで初期化する
    if (stmt->u.declaration_s->initializer) {
         Declaration* decl = cs_search_decl_in_block(); // dummy
         if (!decl) {

             decl = cs_search_decl_global(stmt->u.declaration_s->name);


             switch(decl->type->basic_type) {
                 case CS_BOOLEAN_TYPE:
                 case CS_INT_TYPE: {
                     gen_byte_code((CodegenVisitor*)visitor, SVM_POP_STATIC_INT, 
                             decl->index);
                     break;
                 }
                 case CS_DOUBLE_TYPE: {
                     gen_byte_code((CodegenVisitor*)visitor, SVM_POP_STATIC_DOUBLE, 
                             decl->index);
                     break;
                 }
                 default: {
                     fprintf(stderr, "unknown type in leave_declstmt\n");
                     exit(1);
                 }
             }
             
             
         }
    }
    
    
}

static void enter_ifstmt(Statement* stmt, Visitor* visitor) {
    CodegenVisitor* codegenVisitor = (CodegenVisitor*)visitor;
    //rabelは0で仮置き
    gen_byte_code(codegenVisitor, SVM_PUSH_LABEL, (uint32_t)0);
    //labelを張る位置を覚えておく
    push_label_replacement_point(codegenVisitor, codegenVisitor->pos-4);

}

static void enter_stmtblock(Statement* stmt, Visitor* visitor) {

}

static void leave_ifstmt(Statement* stmt, Visitor* visitor) {
    CodegenVisitor* codegenVisitor = (CodegenVisitor*)visitor;
    //最後のSVM_JUMPかSV_POP_LABEL命令を消してSVM_POP_LABELに変更する
    codegenVisitor->pos--;
    gen_byte_code(codegenVisitor, SVM_POP_LABEL);

    //if文の終わりのlabelを記憶していた場所に張る
    uint32_t replace_point = pop_label_replacement_point(codegenVisitor);
    if (replace_point == 0xffff) {
        fprintf(stderr, "underflow label_replacement_point_stack\n");
    }
    replace_4byte_code(codegenVisitor, replace_point, codegenVisitor->pos);
}

static void leave_stmtblock(Statement* stmt, Visitor* visitor) {

}

static void enter_if_expr(Expression* expr, Visitor* visitor) {
    CodegenVisitor* codegenVisitor = (CodegenVisitor*)visitor;
    //rabelは0で仮置き
    //puts("start gen_byte_code");
    gen_byte_code(codegenVisitor, SVM_PUSH_LABEL, (uint32_t)0);
    //puts("end gen_byte_code");
    //labelを張る位置を覚えておく
    push_label_replacement_point(codegenVisitor, codegenVisitor->pos-4);
    //puts("end push_label_replacement_point");
}

static void leave_if_expr(Expression* expr, Visitor* visitor) {
    gen_byte_code((CodegenVisitor*)visitor, SVM_CJMP);
}

static void enter_inner_if(Statement *stmt, Visitor* Visitor) {
    
}

static void leave_inner_if(Statement* stmt, Visitor* visitor) {
    CodegenVisitor* codegenVisitor = (CodegenVisitor*)visitor;

    gen_byte_code(codegenVisitor, SVM_JUMP);

    //次の命令へのラベルを記憶していた場所に張る
    uint32_t replace_point = pop_label_replacement_point(codegenVisitor);
    if (replace_point == 0xffff) {
        fprintf(stderr, "underflow label_replacement_point_stack\n");
    }
    replace_4byte_code(codegenVisitor, replace_point, codegenVisitor->pos);
}

static void leave_inner_else(Statement* stmt, Visitor* visitor) {
    gen_byte_code((CodegenVisitor*)visitor, SVM_POP_LABEL);
}


CodegenVisitor* create_codegen_visitor(CS_Compiler* compiler, CS_Executable *exec) {
    visit_expr* enter_expr_list;
    visit_expr* leave_expr_list;
    visit_stmt* enter_stmt_list;
    visit_stmt* leave_stmt_list;
    
    visit_expr* notify_expr_list;
    
    //引数のExpressionとstatementは使わないので新しい構造体を定義したほうがいいかもしれないが使いまわす。
    visit_expr* if_codegen_expr_list;
    visit_stmt* if_codegen_stmt_list;
    
    if (compiler == NULL || exec == NULL) {
        fprintf(stderr, "Compiler or Executable is NULL\n");
        exit(1);
    }
    
    CodegenVisitor* visitor = (CodegenVisitor*)MEM_malloc(sizeof(CodegenVisitor));
    visitor->compiler = compiler;
    visitor->exec = exec;
    visitor->CODE_ALLOC_SIZE = 10; // temporary
    visitor->current_code_size = 0;
    visitor->pos = 0;
    visitor->code = NULL;
    visitor->vi_state = VISIT_NORMAL;
    visitor->vf_state = VISIT_F_NO;
    visitor->assign_depth = 0;
    visitor->label_replacement_point_stack = create_unit32_t_stack();
    visitor->stack_p = 0;

    enter_expr_list = (visit_expr*)MEM_malloc(sizeof(visit_expr) * EXPRESSION_KIND_PLUS_ONE);
    leave_expr_list = (visit_expr*)MEM_malloc(sizeof(visit_expr) * EXPRESSION_KIND_PLUS_ONE);
    notify_expr_list = (visit_expr*)MEM_malloc(sizeof(visit_expr) * EXPRESSION_KIND_PLUS_ONE);
    
    enter_stmt_list = (visit_stmt*)MEM_malloc(sizeof(visit_stmt) * STATEMENT_TYPE_COUNT_PLUS_ONE);
    leave_stmt_list = (visit_stmt*)MEM_malloc(sizeof(visit_stmt) * STATEMENT_TYPE_COUNT_PLUS_ONE);
    
    if_codegen_stmt_list = (visit_stmt*)MEM_malloc(sizeof(visit_stmt) * IF_CODEGEN_STMT_TYPE_PLUS_ONE);
    if_codegen_expr_list = (visit_expr*)MEM_malloc(sizeof(visit_expr) * IF_CODEGEN_EXPR_TYPE_PLUS_ONE);

    memset(enter_expr_list, 0, sizeof(visit_expr) * EXPRESSION_KIND_PLUS_ONE);
    memset(leave_expr_list, 0, sizeof(visit_expr) * EXPRESSION_KIND_PLUS_ONE);
    memset(notify_expr_list, 0, sizeof(visit_expr) * EXPRESSION_KIND_PLUS_ONE);    
    memset(enter_stmt_list, 0, sizeof(visit_expr) * STATEMENT_TYPE_COUNT_PLUS_ONE);
    memset(leave_stmt_list, 0, sizeof(visit_expr) * STATEMENT_TYPE_COUNT_PLUS_ONE);
    memset(if_codegen_stmt_list, 0, sizeof(visit_expr) * IF_CODEGEN_STMT_TYPE_PLUS_ONE);
    memset(if_codegen_expr_list, 0, sizeof(visit_expr) * IF_CODEGEN_EXPR_TYPE_PLUS_ONE);

    
    enter_expr_list[BOOLEAN_EXPRESSION]       = enter_boolexpr;
    enter_expr_list[INT_EXPRESSION]           = enter_intexpr;
    enter_expr_list[DOUBLE_EXPRESSION]        = enter_doubleexpr;
    enter_expr_list[IDENTIFIER_EXPRESSION]    = enter_identexpr;    
    enter_expr_list[ADD_EXPRESSION]           = enter_addexpr;
    enter_expr_list[SUB_EXPRESSION]           = enter_subexpr;
    enter_expr_list[MUL_EXPRESSION]           = enter_mulexpr;
    enter_expr_list[DIV_EXPRESSION]           = enter_divexpr;
    enter_expr_list[MOD_EXPRESSION]           = enter_modexpr;    
    enter_expr_list[GT_EXPRESSION]            = enter_gtexpr;
    enter_expr_list[GE_EXPRESSION]            = enter_geexpr;
    enter_expr_list[LT_EXPRESSION]            = enter_ltexpr;
    enter_expr_list[LE_EXPRESSION]            = enter_leexpr;
    enter_expr_list[EQ_EXPRESSION]            = enter_eqexpr;
    enter_expr_list[NE_EXPRESSION]            = enter_neexpr;
    enter_expr_list[LOGICAL_AND_EXPRESSION]   = enter_landexpr;
    enter_expr_list[LOGICAL_OR_EXPRESSION]    = enter_lorexpr;
    enter_expr_list[INCREMENT_EXPRESSION]     = enter_incexpr;
    enter_expr_list[DECREMENT_EXPRESSION]     = enter_decexpr;
    enter_expr_list[MINUS_EXPRESSION]         = enter_minusexpr;
    enter_expr_list[LOGICAL_NOT_EXPRESSION]   = enter_lognotexpr;
    enter_expr_list[ASSIGN_EXPRESSION]        = enter_assignexpr;
    enter_expr_list[FUNCTION_CALL_EXPRESSION] = enter_funccallexpr;
    enter_expr_list[CAST_EXPRESSION]          = enter_castexpr;
    
    enter_stmt_list[EXPRESSION_STATEMENT]     = enter_exprstmt;
    enter_stmt_list[DECLARATION_STATEMENT]    = enter_declstmt;
    enter_stmt_list[IF_STATEMENT]             = enter_ifstmt;
    enter_stmt_list[STATEMENT_BLOCK]          = enter_stmtblock;

    notify_expr_list[ASSIGN_EXPRESSION]       = notify_assignexpr;
    
    
    
    leave_expr_list[BOOLEAN_EXPRESSION]       = leave_boolexpr;
    leave_expr_list[INT_EXPRESSION]           = leave_intexpr;
    leave_expr_list[DOUBLE_EXPRESSION]        = leave_doubleexpr;
    leave_expr_list[IDENTIFIER_EXPRESSION]    = leave_identexpr;    
    leave_expr_list[ADD_EXPRESSION]           = leave_addexpr;
    leave_expr_list[SUB_EXPRESSION]           = leave_subexpr;
    leave_expr_list[MUL_EXPRESSION]           = leave_mulexpr;
    leave_expr_list[DIV_EXPRESSION]           = leave_divexpr;
    leave_expr_list[MOD_EXPRESSION]           = leave_modexpr;    
    leave_expr_list[GT_EXPRESSION]            = leave_gtexpr;
    leave_expr_list[GE_EXPRESSION]            = leave_geexpr;
    leave_expr_list[LT_EXPRESSION]            = leave_ltexpr;
    leave_expr_list[LE_EXPRESSION]            = leave_leexpr;
    leave_expr_list[EQ_EXPRESSION]            = leave_eqexpr;
    leave_expr_list[NE_EXPRESSION]            = leave_neexpr;
    leave_expr_list[LOGICAL_AND_EXPRESSION]   = leave_landexpr;
    leave_expr_list[LOGICAL_OR_EXPRESSION]    = leave_lorexpr;
    leave_expr_list[INCREMENT_EXPRESSION]     = leave_incexpr;
    leave_expr_list[DECREMENT_EXPRESSION]     = leave_decexpr;
    leave_expr_list[DECREMENT_EXPRESSION]     = leave_decexpr;
    leave_expr_list[MINUS_EXPRESSION]         = leave_minusexpr;
    leave_expr_list[LOGICAL_NOT_EXPRESSION]   = leave_lognotexpr;
    leave_expr_list[ASSIGN_EXPRESSION]        = leave_assignexpr;
    leave_expr_list[FUNCTION_CALL_EXPRESSION] = leave_funccallexpr;
    leave_expr_list[CAST_EXPRESSION]          = leave_castexpr;
    
    leave_stmt_list[EXPRESSION_STATEMENT]     = leave_exprstmt;
    leave_stmt_list[DECLARATION_STATEMENT]    = leave_declstmt;
    leave_stmt_list[IF_STATEMENT]             = leave_ifstmt;
    leave_stmt_list[STATEMENT_BLOCK]          = leave_stmtblock;


    if_codegen_expr_list[ENTER_IF_EXPR]       = enter_if_expr;
    if_codegen_expr_list[LEAVE_IF_EXPR]       = leave_if_expr;
    if_codegen_expr_list[ENTER_ELSIF_EXPR]    = enter_if_expr;//使いまわし
    if_codegen_expr_list[LEAVE_ELSIF_EXPR]    = leave_if_expr;//使いまわし


    if_codegen_stmt_list[ENTER_INNER_IF]      = enter_inner_if;
    if_codegen_stmt_list[LEAVE_INNER_IF]      = leave_inner_if;
    if_codegen_stmt_list[ENTER_INNER_ELSIF]   = enter_inner_if;//使いまわし
    if_codegen_stmt_list[LEAVE_INNER_ELSIF]   = leave_inner_if;//使いまわし
    if_codegen_stmt_list[ENTER_INNER_ELSE]    = enter_inner_if;//使いまわし
    if_codegen_stmt_list[LEAVE_INNER_ELSE]    = leave_inner_else;
    
    ((Visitor*)visitor)->enter_expr_list = enter_expr_list;
    ((Visitor*)visitor)->leave_expr_list = leave_expr_list;
    ((Visitor*)visitor)->enter_stmt_list = enter_stmt_list;
    ((Visitor*)visitor)->leave_stmt_list = leave_stmt_list;

    ((Visitor*)visitor)->notify_expr_list = notify_expr_list;
    
    ((Visitor*)visitor)->if_codegen_expr_list = if_codegen_expr_list;
    ((Visitor*)visitor)->if_codegen_stmt_list = if_codegen_stmt_list;
    
    
    return visitor;
}

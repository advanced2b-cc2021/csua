#include <stdio.h>
#include <stdlib.h>
#include "csua.h"
int statement_list_group = 0;
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

int main(int argc, char* argv[]) {

    extern int yyparse(void);
    extern FILE *yyin;
    
    if (argc == 1) {
        printf("Usage ./prst dir/filename.cs\n");
        return 1;
    }

//    yyin = fopen("tests/prog1.cs", "r");
    yyin = fopen(argv[1], "r");    
    if (yyin == NULL) {
        fprintf(stderr, "cannot open file\n");
        exit(1);
    }
    
    if (yyparse()) {
        fprintf(stderr, "Parse Error\n");
        exit(1);
    }
    

    fclose(yyin);
    
    fprintf(stderr, "prase OK\n");
    return 0;
}
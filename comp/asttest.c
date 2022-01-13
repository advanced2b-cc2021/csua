#include <stdio.h>
#include <stdlib.h>
#include "csua.h"
#include "visitor.h"
#include "../memory/MEM.h"


int main(int argc, char* argv[]) {
    
//    FILE *fin = fopen("tests/prog6.cs", "r");
    if (argc == 1) {
        printf("Usage ./prst dir/filename.cs\n");
        return 1;
    }

    FILE* fin = fopen(argv[1], "r");    
    
    
    CS_Compiler* compiler = CS_create_compiler();
    
    //CS_compile(compiler, fin);
    
    //
    extern FILE *yyin;
    extern int yyparse(void);
    yyin = fin;
    if (yyin == NULL) {
        fprintf(stderr, "cannot open file\n");
        exit(1);
    }
    if (yyparse()) {
        fprintf(stderr, "Parse Error");
        exit(1);
    }   
    //

    Visitor* visitor = create_treeview_visitor();
    
    StatementList* stmt_list = compiler->root_stmt_list;
    printf("--------------\n");
    stmt_list = compiler->root_stmt_list;
    traverse_stmt_list(stmt_list, visitor);
    printf("--- FunctionInfo ---\n");
    FunctionDeclarationList* func_list = compiler->func_list;
    for (int i = 0; func_list; func_list = func_list->next, ++i) {
        FunctionDeclaration* f = func_list->func;
        ParameterList* param = f->param;
        if (param) {
            for(; param; param= param->next) {
                printf("param = %s\n", param->name);
            }
        }
    }
    
    fclose(fin);
    delete_visitor(visitor);    
    CS_delete_compiler(compiler);
    MEM_dump_memory();
    return 0;
}
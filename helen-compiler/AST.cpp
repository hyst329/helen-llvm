//
// Created by main on 20.09.2015.
//

#include "AST.h"

IRBuilder<> AST::builder(getGlobalContext());

Value* VariableAST::codegen() {
    return 0;
}

//Value *ValueAST::codegen() {
//    return nullptr;
//}

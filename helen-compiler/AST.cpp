//
// Created by main on 20.09.2015.
//

#include "AST.h"

IRBuilder<> AST::builder(getGlobalContext());

Value *VariableAST::codegen() {
    return 0;
}

Value *ConstantIntAST::codegen() {
    return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value);
}

Value *ConstantRealAST::codegen() {
    return ConstantFP::get(getGlobalContext(), APFloat(value));
}
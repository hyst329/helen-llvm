//
// Created by main on 20.09.2015.
//

#include "AST.h"
#include "Error.h"

namespace Helen
{
IRBuilder<> AST::builder(getGlobalContext());
map<string, Value*> AST::variables;
map<string, Function*> AST::functions;

Value* ConstantIntAST::codegen()
{
    return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value);
}

Value* ConstantRealAST::codegen()
{
    return ConstantFP::get(getGlobalContext(), APFloat(value));
}

Value* VariableAST::codegen()
{
    try {
        return variables.at(name);
    } catch(out_of_range) {
        return Error::errorValue(ErrorType::UndeclaredVariable);
    }
}
}
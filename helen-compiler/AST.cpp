//
// Created by main on 20.09.2015.
//

#include "AST.h"
#include "Error.h"

namespace Helen
{
unique_ptr<Module> AST::module = 0;
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

Value* ConditionAST::codegen()
{
    if(condition->codegen()) // TODO: add "is nonzero" or similar
        return thenBranch->codegen();
    else
        return elseBranch->codegen();
}

Value* FunctionCallAST::codegen()
{
    Function* f = module->getFunction(functionName);
    if(!f)
        return Error::errorValue(ErrorType::UndeclaredFunction);
    std::vector<Value*> vargs;
    for(unsigned i = 0, e = arguments.size(); i != e; ++i) {
        //TODO: add type checking
        vargs.push_back(arguments[i]->codegen());
        if(!vargs.back())
            return nullptr;
    }
    return builder.CreateCall(f, vargs, "calltmp");
}

Value* SequenceAST::codegen()
{
    for(shared_ptr<Helen::AST>& a : instructions)
        a->codegen();
}

Value* NullAST::codegen()
{
    return 0;
}
}

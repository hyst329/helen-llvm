//
// Created by main on 20.09.2015.
//

#include "AST.h"
#include "Error.h"
#include "FunctionNameMangler.h"

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

Value* ConstantCharAST::codegen()
{
    return ConstantInt::get(Type::getInt8Ty(getGlobalContext()), value);
}

Value* ConstantStringAST::codegen()
{
    return ConstantDataArray::getString(getGlobalContext(), StringRef(value)); // TODO: String
}

Value* DeclarationAST::codegen()
{
}

Value* VariableAST::codegen()
{
    try {
        return variables.at(name);
    } catch(out_of_range) {
        return Error::errorValue(ErrorType::UndeclaredVariable, {name});
    }
}

Value* ConditionAST::codegen()
{
    Value* cond = condition->codegen();
    if(!cond)
        return 0;
    cond = builder.CreateICmpNE(cond, ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0), "ifcond");
    Function* f = builder.GetInsertBlock()->getParent();
    BasicBlock* thenBB = BasicBlock::Create(getGlobalContext(), "then", f);
    BasicBlock* elseBB = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock* mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    builder.CreateCondBr(cond, thenBB, elseBB);
    // Emit then value.
    builder.SetInsertPoint(thenBB);

    Value* thenValue = thenBranch->codegen();
    if(!thenValue)
        return 0;

    builder.CreateBr(mergeBB);
    thenBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(elseBB);
    builder.SetInsertPoint(elseBB);

    Value* elseValue = elseBranch->codegen();
    if(!elseValue)
        return 0;

    builder.CreateBr(mergeBB);
    elseBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(mergeBB);
    PHINode* PN = builder.CreatePHI(Type::getInt64Ty(getGlobalContext()), 2, "iftmp");

    PN->addIncoming(thenValue, thenBB);
    PN->addIncoming(elseValue, elseBB);
    return PN;
}

Value* FunctionCallAST::codegen()
{
    std::vector<Value*> vargs;
    std::vector<Type*> types;
    for(unsigned i = 0, e = arguments.size(); i != e; ++i) {
        // TODO: add type checking
        vargs.push_back(arguments[i]->codegen());
        if(!vargs.back())
            return nullptr;
    }
    for(Value* v: vargs) types.push_back(v->getType());
    functionName = FunctionNameMangler::mangleName(functionName, types);
    string hrName = FunctionNameMangler::humanReadableName(functionName);
    Function* f = module->getFunction(functionName);
    if(!f)
        return Error::errorValue(ErrorType::UndeclaredFunction, {hrName});
    ArrayRef<Type*> params = f->getFunctionType()->params();
    for(unsigned i = 0; i < vargs.size(); i++)
        if(vargs[i]->getType() != params[i])
            return Error::errorValue(ErrorType::WrongArgumentType, {std::to_string(i)});
    return builder.CreateCall(f, vargs, "calltmp");
}

Value* SequenceAST::codegen()
{
    for(shared_ptr<Helen::AST>& a : instructions) {
        Value* v = a->codegen();
        if(dynamic_cast<ReturnAST*>(a.get()))
            return v;
    }
    return 0;
}

Value* NullAST::codegen()
{
    return 0;
}

Function* FunctionPrototypeAST::codegen()
{
    FunctionType* ft = FunctionType::get(returnType, args, false);
    name = FunctionNameMangler::mangleName(name, args);
    Function* f = Function::Create(ft, Function::ExternalLinkage, name, module.get());
    functions[name] = f;

    unsigned i = 0;
    for(auto& arg : f->args())
        arg.setName(argNames[i++]);
    return f;
}

Function* FunctionAST::codegen()
{
    Function* f = module->getFunction(proto->getName());

    if(!f)
        f = proto->codegen();

    if(!f)
        return 0;

    if(!f->empty())
        return (Function*)Error::errorValue(ErrorType::FunctionRedefined, {proto->getName()});

    BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "entry", f);
    builder.SetInsertPoint(bb);
    for(auto& arg : f->args())
        variables[arg.getName()] = &arg;
    if(Value* ret = body->codegen()) {
        builder.CreateRet(ret);
        verifyFunction(*f);
        return f;
    }
    f->eraseFromParent();
    return nullptr;
}
Value* ReturnAST::codegen()
{
    return result->codegen();
}
}

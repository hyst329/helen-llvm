//
// Created by main on 20.09.2015.
//

#include "AST.h"
#include "Error.h"
#include "FunctionNameMangler.h"
#include "BuiltinFunctions.h"

namespace Helen
{
unique_ptr<Module> AST::module = 0;
unique_ptr<legacy::FunctionPassManager> AST::fpm = 0;
IRBuilder<> AST::builder(getGlobalContext());
map<string, AllocaInst*> AST::variables;
map<string, Function*> AST::functions;
stack<string> AST::callstack;

AllocaInst* AST::createEntryBlockAlloca(Function* f, Type* t, const std::string& VarName)
{
    IRBuilder<> tmp(&f->getEntryBlock(), f->getEntryBlock().begin());
    return tmp.CreateAlloca(t, 0, VarName.c_str());
}

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
    Function* f = builder.GetInsertBlock()->getParent();

    Value* initValue;
    if(initialiser) {
        initValue = initialiser->codegen();
        if(!initValue)
            return nullptr;
    } else {
        initValue = 0;
    }
    AllocaInst* alloca = createEntryBlockAlloca(f, type, name);
    if(initValue)
        builder.CreateStore(initValue, alloca);
    variables[name] = alloca;
}

Value* VariableAST::codegen()
{
    try {
        Value* val = variables.at(name);
        return builder.CreateLoad(val, name.c_str());
    } catch(out_of_range) {
        return Error::errorValue(ErrorType::UndeclaredVariable, { name });
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
    // Assignment is the special case
    if(functionName == BuiltinFunctions::operatorMarker + "=") {
        shared_ptr<AST> left = arguments[0], right = arguments[1];
        VariableAST* lefte = dynamic_cast<VariableAST*>(left.get());
        if(!lefte)
            return Error::errorValue(ErrorType::AssignmentError, {std::to_string((size_t)lefte)});
        Value* v = right->codegen();
        if(!v)
            return nullptr;
        try {
            Value* var = variables.at(lefte->getName());
            builder.CreateStore(v, var);
            return v;
        } catch(out_of_range) {
            return Error::errorValue(ErrorType::UndeclaredVariable, {lefte->getName()});
        }
    }
    std::vector<Value*> vargs;
    std::vector<Type*> types;
    for(unsigned i = 0, e = arguments.size(); i != e; ++i) {
        // TODO: add type checking
        vargs.push_back(arguments[i]->codegen());
        if(!vargs.back())
            return nullptr;
    }
    for(Value* v : vargs)
        types.push_back(v->getType());
    functionName = FunctionNameMangler::mangleName(functionName, types);
    string hrName = FunctionNameMangler::humanReadableName(functionName);
    Function* f = module->getFunction(functionName);
    if(!f)
        return Error::errorValue(ErrorType::UndeclaredFunction, { hrName, functionName });
    ArrayRef<Type*> params = f->getFunctionType()->params();
    for(unsigned i = 0; i < vargs.size(); i++)
        if(vargs[i]->getType() != params[i])
            return Error::errorValue(ErrorType::WrongArgumentType, { std::to_string(i) });
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
    for(auto& arg : f->args()) {
        arg.setName(argNames[i++]);
    }
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
        return (Function*)Error::errorValue(ErrorType::FunctionRedefined, { proto->getName() });
    callstack.push(proto->getName());
    BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "entry", f);
    builder.SetInsertPoint(bb);
    for(auto& arg : f->args()) {
        AllocaInst* alloca = createEntryBlockAlloca(f, arg.getType(), arg.getName());
        builder.CreateStore(&arg, alloca);
        variables[arg.getName()] = alloca;
    }
    if(Value* ret = body->codegen()) {
        builder.CreateRet(ret);
        verifyFunction(*f);
        fpm->run(*f);
        callstack.pop();
        string previous = callstack.empty() ? "main" : callstack.top();
        BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "resume", module->getFunction(previous));
        builder.SetInsertPoint(bb);
        return f;
    }
    /*callstack.pop();
    string previous = callstack.empty() ? "_main_v" : callstack.top();
    bb = BasicBlock::Create(getGlobalContext(), "resume", module->getFunction(previous));
    builder.SetInsertPoint(bb);*/
    f->eraseFromParent();
    return nullptr;
}
Value* ReturnAST::codegen()
{
    return result->codegen();
}
}

#include "BuiltinFunctions.h"
#include "AST.h"
#include "FunctionNameMangler.h"

namespace Helen
{

using namespace llvm;

const std::string BuiltinFunctions::operatorMarker = "__operator_";
const std::string BuiltinFunctions::unaryOperatorMarker = "__unary_operator_";

void BuiltinFunctions::createMainFunction()
{
    createAllBuiltins();
    vector<Type*> v;
    FunctionType* ft = FunctionType::get(Type::getInt32Ty(getGlobalContext()), v, false);
    string name = FunctionNameMangler::mangleName("main", v);
    Function* f = Function::Create(ft, Function::ExternalLinkage, name, AST::module.get());
    AST::functions[name] = f;
    BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "start", f);
    AST::builder.SetInsertPoint(bb);
}

void BuiltinFunctions::createAllBuiltins()
{
    createArith();
}

void BuiltinFunctions::createArith()
{
    FunctionType* ft;
    Function* f;
    BasicBlock* bb;
    string name;
    Value* left, *right, *res;
    Type* i = Type::getInt64Ty(getGlobalContext());
    Type* r = Type::getDoubleTy(getGlobalContext());
    //_operator_(int, int), _operator_(real, real)
    for (char c : { '+', '-', '*', '/' }) {
        for (Type* t : { i, r }) {
            ft = FunctionType::get(t, vector<Type*>{ t, t }, false);
            name = FunctionNameMangler::mangleName(operatorMarker + c, { t, t });
            f = Function::Create(ft, Function::ExternalLinkage, name, AST::module.get());
            bb = BasicBlock::Create(getGlobalContext(), "entry", f);
            AST::builder.SetInsertPoint(bb);
            auto it = f->arg_begin();
            left = it++;
            right = it;
            switch (c) {
            case '+':
                res = t == i ? AST::builder.CreateAdd(left, right, "tmp") : AST::builder.CreateFAdd(left, right, "tmp");
                break;
            case '-':
                res = t == i ? AST::builder.CreateSub(left, right, "tmp") : AST::builder.CreateFSub(left, right, "tmp");
                break;
            case '*':
                res = t == i ? AST::builder.CreateMul(left, right, "tmp") : AST::builder.CreateFMul(left, right, "tmp");
                break;
            case '/':
                res =
                    t == i ? AST::builder.CreateSDiv(left, right, "tmp") : AST::builder.CreateFDiv(left, right, "tmp");
                break;
            }
            AST::builder.CreateRet(res);
        }
    }
}
}

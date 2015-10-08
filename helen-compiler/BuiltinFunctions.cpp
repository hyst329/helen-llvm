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
    string name = "main";
    Function* f = Function::Create(ft, Function::ExternalLinkage, name, AST::module.get());
    AST::functions[name] = f;
    BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "start", f);
    AST::builder.SetInsertPoint(bb);
}

void BuiltinFunctions::createAllBuiltins()
{
    createArith();
    createIO();
    createIndex();
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
            f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
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

void BuiltinFunctions::createIO()
{
    std::vector<Type*> printf_types;
    printf_types.push_back(Type::getInt8PtrTy(getGlobalContext()));

    FunctionType* printf_type = FunctionType::get(Type::getInt32Ty(getGlobalContext()), printf_types, true);

    Function* printf = Function::Create(printf_type, Function::ExternalLinkage, "printf", AST::module.get());
    printf->setCallingConv(CallingConv::C);

    // __out
    Type* i = Type::getInt64Ty(getGlobalContext());
    Type* r = Type::getDoubleTy(getGlobalContext());
    Type* v = Type::getVoidTy(getGlobalContext());
    for (Type* t : { i, r }) {
        string s;
        if (t == i)
            s = "%lld\n";
        if (t == r)
            s = "%lf\n";
        string name = FunctionNameMangler::mangleName("__out", { t });
        FunctionType* ft = FunctionType::get(i, { t }, false);
        Function* f = Function::Create(ft, Function::ExternalLinkage, name, AST::module.get());
        BasicBlock* parent = AST::builder.GetInsertBlock();
        BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "entry", f);
        AST::builder.SetInsertPoint(bb);
        vector<Value*> args;
        Constant* fmt = ConstantDataArray::getString(getGlobalContext(), StringRef(s));
        Type* stype = ArrayType::get(IntegerType::get(getGlobalContext(), 8), s.size() + 1);
        GlobalVariable* var =
            new GlobalVariable(*AST::module.get(), stype, true, GlobalValue::PrivateLinkage, fmt, ".str");
        Constant* zero = Constant::getNullValue(llvm::IntegerType::getInt32Ty(getGlobalContext()));
        Constant* ind[] = { zero, zero };
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 7
        Constant* fmt_ref = ConstantExpr::getGetElementPtr(stype, var, ind);
#else
        Constant* fmt_ref = ConstantExpr::getGetElementPtr(var, ind);
#endif
        auto it = printf->arg_begin();
        Value* val = it++;
        args.push_back(fmt_ref);
        args.push_back(f->arg_begin());
        CallInst* call = AST::builder.CreateCall(printf, args);
        call->setTailCall(true);
        AST::builder.CreateRet(Constant::getNullValue(llvm::IntegerType::getInt64Ty(getGlobalContext())));
    }
}

void BuiltinFunctions::createIndex()
{
    // The index function is temporary handled in AST.cpp
}
}

#include "BuiltinFunctions.h"
#include "AST.h"
#include "FunctionNameMangler.h"

namespace Helen
{

using namespace llvm;

const std::string BuiltinFunctions::operatorMarker = "__operator_";
const std::string BuiltinFunctions::unaryOperatorMarker = "__unary_operator_";

void BuiltinFunctions::createMainFunction(bool isMainModule)
{
    createAllBuiltins();
    if(!isMainModule)
        return;
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
    createLnC();
    createMemory();
    createIO();
    createIndex();
}

void BuiltinFunctions::createArith()
{
    FunctionType* ft;
    Function* f;
    BasicBlock* bb;
    string name;
    Value *left, *right, *res;
    Type* i8 = Type::getInt8Ty(getGlobalContext());
    Type* i16 = Type::getInt16Ty(getGlobalContext());
    Type* i32 = Type::getInt32Ty(getGlobalContext());
    Type* i64 = Type::getInt64Ty(getGlobalContext());
    Type* r64 = Type::getDoubleTy(getGlobalContext());
    //_operator_(int, int), _operator_(real, real)
    for(char c : { '+', '-', '*', '/' }) {
        for(Type* t : { i8, i16, i32, i64, r64 }) {
            ft = FunctionType::get(t, vector<Type*>{ t, t }, false);
            name = FunctionNameMangler::mangleName(operatorMarker + c, { t, t });
            f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
            f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
            bb = BasicBlock::Create(getGlobalContext(), "entry", f);
            AST::builder.SetInsertPoint(bb);
            auto it = f->arg_begin();
            left = it++;
            right = it;
            switch(c) {
            case '+':
                res = t->isIntegerTy() ? AST::builder.CreateAdd(left, right, "tmp") :
                                         AST::builder.CreateFAdd(left, right, "tmp");
                break;
            case '-':
                res = t->isIntegerTy() ? AST::builder.CreateSub(left, right, "tmp") :
                                         AST::builder.CreateFSub(left, right, "tmp");
                break;
            case '*':
                res = t->isIntegerTy() ? AST::builder.CreateMul(left, right, "tmp") :
                                         AST::builder.CreateFMul(left, right, "tmp");
                break;
            case '/':
                res = t->isIntegerTy() ? AST::builder.CreateSDiv(left, right, "tmp") :
                                         AST::builder.CreateFDiv(left, right, "tmp");
                break;
            }
            AST::builder.CreateRet(res);
        }
    }
    // unary operators
    for(char c : { '+', '-' }) {
        for(Type* t : { i8, i16, i32, i64, r64 }) {
            ft = FunctionType::get(t, vector<Type*>{ t }, false);
            name = FunctionNameMangler::mangleName(unaryOperatorMarker + c, { t });
            f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
            f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
            bb = BasicBlock::Create(getGlobalContext(), "entry", f);
            AST::builder.SetInsertPoint(bb);
            auto it = f->arg_begin();
            left = it;
            switch(c) {
            case '+':
                res = left;
                break;
            case '-':
                res = t->isIntegerTy() ? AST::builder.CreateNeg(left, "tmp") : AST::builder.CreateFNeg(left, "tmp");
                break;
            }
            AST::builder.CreateRet(res);
        }
    }
}
void BuiltinFunctions::createLnC()
{
    FunctionType* ft;
    Function* f;
    BasicBlock* bb;
    string name;
    Value *left, *right, *res;
    Type* i8 = Type::getInt8Ty(getGlobalContext());
    Type* i16 = Type::getInt16Ty(getGlobalContext());
    Type* i32 = Type::getInt32Ty(getGlobalContext());
    Type* i64 = Type::getInt64Ty(getGlobalContext());
    Type* i1 = Type::getInt1Ty(getGlobalContext());
    Type* r64 = Type::getDoubleTy(getGlobalContext());
    //_operator_(int, int), _operator_(real, real)
    for(string s : { "<", ">", "<=", ">=", "==", "!=" }) {
        for(Type* t : { i8, i16, i32, i64, r64 }) {
            ft = FunctionType::get(i1, vector<Type*>{ t, t }, false);
            name = FunctionNameMangler::mangleName(operatorMarker + s, { t, t });
            f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
            f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
            bb = BasicBlock::Create(getGlobalContext(), "entry", f);
            AST::builder.SetInsertPoint(bb);
            auto it = f->arg_begin();
            left = it++;
            right = it;
            if(s == "<")
                res = t->isIntegerTy() ? AST::builder.CreateICmpSLT(left, right, "tmp") :
                                         AST::builder.CreateFCmpOLT(left, right, "tmp");
            if(s == "<=")
                res = t->isIntegerTy() ? AST::builder.CreateICmpSLE(left, right, "tmp") :
                                         AST::builder.CreateFCmpOLE(left, right, "tmp");
            if(s == ">")
                res = t->isIntegerTy() ? AST::builder.CreateICmpSGT(left, right, "tmp") :
                                         AST::builder.CreateFCmpOGT(left, right, "tmp");
            if(s == ">=")
                res = t->isIntegerTy() ? AST::builder.CreateICmpSGE(left, right, "tmp") :
                                         AST::builder.CreateFCmpOGE(left, right, "tmp");
            if(s == "==")
                res = t->isIntegerTy() ? AST::builder.CreateICmpEQ(left, right, "tmp") :
                                         AST::builder.CreateFCmpOEQ(left, right, "tmp");
            if(s == "!=")
                res = t->isIntegerTy() ? AST::builder.CreateICmpNE(left, right, "tmp") :
                                         AST::builder.CreateFCmpONE(left, right, "tmp");
            AST::builder.CreateRet(res);
        }
    }
    for(string s : { "~&", "~|", "~@" }) {
        for(Type* t : { i8, i16, i32, i64 }) {
            ft = FunctionType::get(i1, vector<Type*>{ t, t }, false);
            name = FunctionNameMangler::mangleName(operatorMarker + s, { t, t });
            f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
            f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
            bb = BasicBlock::Create(getGlobalContext(), "entry", f);
            AST::builder.SetInsertPoint(bb);
            auto it = f->arg_begin();
            left = it++;
            right = it;
            if(s == "~&")
                res = AST::builder.CreateAnd(left, right, "tmp");
            if(s == "~|")
                res = AST::builder.CreateOr(left, right, "tmp");
            if(s == "~@")
                res = AST::builder.CreateXor(left, right, "tmp");
            AST::builder.CreateRet(res);
        }
    }
    // unary bitwise NOT operator
    for(Type* t : { i8, i16, i32, i64 }) {
        ft = FunctionType::get(t, vector<Type*>{ t }, false);
        name = FunctionNameMangler::mangleName(unaryOperatorMarker + "~!", { t });
        f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
        f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
        bb = BasicBlock::Create(getGlobalContext(), "entry", f);
        AST::builder.SetInsertPoint(bb);
        auto it = f->arg_begin();
        left = it;
        res = AST::builder.CreateNot(left, "tmp");
        AST::builder.CreateRet(res);
    }
    for(string s : { "&", "|", "@" }) {
        ft = FunctionType::get(i1, vector<Type*>{ i1, i1 }, false);
        name = FunctionNameMangler::mangleName(operatorMarker + s, { i1, i1 });
        f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
        f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
        bb = BasicBlock::Create(getGlobalContext(), "entry", f);
        AST::builder.SetInsertPoint(bb);
        auto it = f->arg_begin();
        left = it++;
        right = it;
        if(s == "&")
            res = AST::builder.CreateAnd(left, right, "tmp");
        if(s == "|")
            res = AST::builder.CreateOr(left, right, "tmp");
        if(s == "@")
            res = AST::builder.CreateXor(left, right, "tmp");
        AST::builder.CreateRet(res);
    }
    // unary NOT operator
    ft = FunctionType::get(i1, vector<Type*>{ i1 }, false);
    name = FunctionNameMangler::mangleName(unaryOperatorMarker + '!', { i1 });
    f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
    f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
    bb = BasicBlock::Create(getGlobalContext(), "entry", f);
    AST::builder.SetInsertPoint(bb);
    auto it = f->arg_begin();
    left = it;
    res = AST::builder.CreateNot(left, "tmp");
    AST::builder.CreateRet(res);
}

void BuiltinFunctions::createIO()
{
    std::vector<Type*> printf_types;
    printf_types.push_back(Type::getInt8PtrTy(getGlobalContext()));

    FunctionType* printf_type = FunctionType::get(Type::getInt32Ty(getGlobalContext()), printf_types, true);

    Function* printf = Function::Create(printf_type, Function::ExternalLinkage, "printf", AST::module.get());
    printf->setCallingConv(CallingConv::C);

    // __out
    Type* i16 = Type::getInt64Ty(getGlobalContext());
    Type* i32 = Type::getInt64Ty(getGlobalContext());
    Type* i64 = Type::getInt64Ty(getGlobalContext());
    Type* r64 = Type::getDoubleTy(getGlobalContext());
    Type* i8 = Type::getInt8Ty(getGlobalContext());
    Type* i1 = Type::getInt1Ty(getGlobalContext());
    Type* s = PointerType::get(i8, 0);
    Type* v = Type::getVoidTy(getGlobalContext());
    for(Type* t : { i16, i32, i64, r64, i8, i1, s }) {
        string fmtstr;
        if(t == i16)
            fmtstr = "%hd\n";
        if(t == i32)
            fmtstr = "%d\n";
        if(t == i64)
            fmtstr = "%lld\n";
        if(t == r64)
            fmtstr = "%lf\n";
        if(t == i8)
            fmtstr = "%c\n";
        if(t == s)
            fmtstr = "%s\n";
        if(t == i1)
            fmtstr = "%d\n";
        string name = FunctionNameMangler::mangleName("__out", { t });
        FunctionType* ft = FunctionType::get(i64, { t }, false);
        Function* f = Function::Create(ft, Function::LinkOnceODRLinkage, name, AST::module.get());
        f->addAttribute(AttributeSet::FunctionIndex, Attribute::AlwaysInline);
        BasicBlock* parent = AST::builder.GetInsertBlock();
        BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "entry", f);
        AST::builder.SetInsertPoint(bb);
        vector<Value*> args;
        Constant* fmt = ConstantDataArray::getString(getGlobalContext(), StringRef(fmtstr));
        Type* stype = ArrayType::get(IntegerType::get(getGlobalContext(), 8), fmtstr.size() + 1);
        GlobalVariable* var =
            new GlobalVariable(*AST::module.get(), stype, true, GlobalValue::PrivateLinkage, fmt, ".pfstr");
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

void BuiltinFunctions::createMemory()
{
    std::vector<Type*> malloc_types;
    malloc_types.push_back(Type::getInt32Ty(getGlobalContext()));

    FunctionType* malloc_type = FunctionType::get(Type::getInt8PtrTy(getGlobalContext()), malloc_types, false);

    Function* malloc = Function::Create(malloc_type, Function::ExternalLinkage, "malloc", AST::module.get());
    malloc->setCallingConv(CallingConv::C);

    std::vector<Type*> free_types;
    free_types.push_back(Type::getInt8PtrTy(getGlobalContext()));

    FunctionType* free_type = FunctionType::get(Type::getVoidTy(getGlobalContext()), free_types, false);

    Function* free = Function::Create(free_type, Function::ExternalLinkage, "free", AST::module.get());
    free->setCallingConv(CallingConv::C);

    std::vector<Type*> memset_types;
    memset_types.push_back(Type::getInt8PtrTy(getGlobalContext()));
    memset_types.push_back(Type::getInt32Ty(getGlobalContext()));
    memset_types.push_back(Type::getInt32Ty(getGlobalContext()));

    FunctionType* memset_type = FunctionType::get(Type::getInt8PtrTy(getGlobalContext()), memset_types, false);

    Function* memset = Function::Create(memset_type, Function::ExternalLinkage, "memset", AST::module.get());
    memset->setCallingConv(CallingConv::C);
}
}

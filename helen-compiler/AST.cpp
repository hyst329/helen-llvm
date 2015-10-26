//
// Created by main on 20.09.2015.
//

#include "AST.h"
#include "Error.h"
#include "FunctionNameMangler.h"
#include "BuiltinFunctions.h"
#include <set>
#include <boost/algorithm/string.hpp>

namespace Helen
{
unique_ptr<Module> AST::module = 0;
unique_ptr<DataLayout> AST::dataLayout = 0;
unique_ptr<legacy::FunctionPassManager> AST::fpm = 0;
IRBuilder<> AST::builder(getGlobalContext());
map<string, AllocaInst*> AST::variables;
map<string, Function*> AST::functions;
map<string, Type*> AST::types;
map<string, vector<string> > AST::fields;
stack<string> AST::callstack;
bool AST::isMainModule = false;

AllocaInst* AST::createEntryBlockAlloca(Function* f, Type* t, const std::string& VarName)
{
    IRBuilder<> tmp(&f->getEntryBlock(), f->getEntryBlock().begin());
    return tmp.CreateAlloca(t, 0, VarName.c_str());
}

Value* ConstantIntAST::codegen()
{
    return ConstantInt::get(IntegerType::get(getGlobalContext(), bitwidth), value);
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
        initValue = Constant::getNullValue(type);
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
    Type* condtype = cond->getType();
    if(!cond)
        return 0;
    if(condtype->isIntegerTy())
        cond = builder.CreateICmpNE(cond, Constant::getNullValue(condtype), "ifcond");
    if(condtype->isDoubleTy())
        cond = builder.CreateFCmpONE(cond, Constant::getNullValue(condtype), "ifcond");
    Function* f = builder.GetInsertBlock()->getParent();
    BasicBlock* thenBB = BasicBlock::Create(getGlobalContext(), "then", f);
    BasicBlock* elseBB = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock* mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");

    builder.CreateCondBr(cond, thenBB, elseBB);
    // Emit then value.
    builder.SetInsertPoint(thenBB);

    Value* thenValue = thenBranch->codegen();
    if(!thenValue)
        thenValue = ConstantInt::get(IntegerType::get(getGlobalContext(), 64), 0);

    builder.CreateBr(mergeBB);
    thenBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(elseBB);
    builder.SetInsertPoint(elseBB);

    Value* elseValue = elseBranch->codegen();
    if(!elseValue)
        elseValue = ConstantInt::get(IntegerType::get(getGlobalContext(), 64), 0);

    builder.CreateBr(mergeBB);
    elseBB = builder.GetInsertBlock();

    f->getBasicBlockList().push_back(mergeBB);
    builder.SetInsertPoint(mergeBB);
    /*PHINode* PN = builder.CreatePHI(Type::getInt64Ty(getGlobalContext()), 2, "iftmp");

    PN->addIncoming(thenValue, thenBB);
    PN->addIncoming(elseValue, elseBB);
    return PN;*/
    return 0;
}

Value* LoopAST::codegen()
{
    if(initial)
        initial->codegen();
    Function* f = builder.GetInsertBlock()->getParent();
    BasicBlock* preheaderBB = builder.GetInsertBlock();
    BasicBlock* loopBB = BasicBlock::Create(getGlobalContext(), "loop", f);
    builder.CreateBr(loopBB);
    builder.SetInsertPoint(loopBB);
    body->codegen();
    if(iteration)
        iteration->codegen();
    Value* cond = condition->codegen();
    Type* condtype = cond->getType();
    if(condtype->isIntegerTy())
        cond = builder.CreateICmpNE(cond, Constant::getNullValue(condtype), "condtmp");
    if(condtype->isDoubleTy())
        cond = builder.CreateFCmpONE(cond, Constant::getNullValue(condtype), "condtmp");
    BasicBlock* loopEndBB = builder.GetInsertBlock();
    BasicBlock* afterBB = BasicBlock::Create(getGlobalContext(), "afterloop", f);
    builder.CreateCondBr(cond, loopBB, afterBB);
    builder.SetInsertPoint(afterBB);
    return Constant::getNullValue(Type::getInt64Ty(getGlobalContext()));
}

Value* FunctionCallAST::codegen()
{
    // Assignment is the special case
    if(functionName == BuiltinFunctions::operatorMarker + "=") {
        shared_ptr<AST> left = arguments[0], right = arguments[1];
        FunctionCallAST* leftf = dynamic_cast<FunctionCallAST*>(left.get());
        if(leftf) {
            string fname = leftf->getFunctionName();
            // Here must be check for reference
            if(fname == "__index") {
                Value* arr = leftf->arguments[0]->codegen();
                if(arr->getType()->isArrayTy()) {
                    Value* idx = leftf->arguments[1]->codegen();
                    if(!idx->getType()->isIntegerTy(64))
                        return Error::errorValue(ErrorType::WrongArgumentType, { "must be int" });
                    Constant* one = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 1);
                    Constant* zero = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0);
                    idx = builder.CreateSub(idx, one);
                    Value* ind[] = { zero, idx };
                    if(!dynamic_cast<VariableAST*>(leftf->arguments[0].get())) {
                        return Error::errorValue(ErrorType::AssignmentError, { std::to_string((size_t)left.get()) });
                    }
                    AllocaInst* aarr = variables.at(((VariableAST*)(leftf->arguments[0].get()))->getName());
                    ;
                    builder.CreateStore(arr, aarr);
                    Value* v = builder.CreateInBoundsGEP(aarr, ind, "indtmpptr");
                    //                    if (dynamic_cast<VariableAST*>(leftf->arguments[0].get())) {
                    //                        Value* var =
                    //                        variables.at(((VariableAST*)(leftf->arguments[0].get()))->getName());
                    //                        return builder.CreateStore(v, var);
                    //                    }
                    return builder.CreateStore(right->codegen(), v);
                } else if(arr->getType()->isPointerTy()) {
                    Type* elTy = cast<PointerType>(arr->getType())->getElementType();
                    if(!elTy->isStructTy())
                        return Error::errorValue(ErrorType::IndexArgumentError);
                    if(!dynamic_cast<VariableAST*>(leftf->arguments[1].get()))
                        return Error::errorValue(ErrorType::WrongArgumentType, { "must be ID" });
                    string name = ((VariableAST*)(leftf->arguments[1].get()))->getName();
                    vector<string> fie = fields[static_cast<StructType*>(elTy)->getName()];
                    auto field = std::find(fie.begin(), fie.end(), name);
                    if(field == fie.end()) {
                        return Error::errorValue(ErrorType::UndeclaredVariable, { name });
                    }
                    int pos = field - fie.begin();
                    Value* zero = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
                    Value* ind = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), pos);
                    Value* idx[] = { zero, ind };
                    if(!dynamic_cast<VariableAST*>(leftf->arguments[0].get())) {
                        return Error::errorValue(ErrorType::AssignmentError, { std::to_string((size_t)left.get()) });
                    }
                    Value* tmpptr = builder.CreateInBoundsGEP(arr, idx, "indtmpptr");
                    Value* v = builder.CreateStore(right->codegen(), tmpptr);
                    return v;
                } else {
                    return Error::errorValue(ErrorType::IndexArgumentError);
                }
            }
        }
        VariableAST* lefte = dynamic_cast<VariableAST*>(left.get());
        if(!lefte)
            return Error::errorValue(ErrorType::AssignmentError, { std::to_string((size_t)left.get()) });
        Value* v = right->codegen();
        if(!v)
            return nullptr;
        try {
            Value* var = variables.at(lefte->getName());
            builder.CreateStore(v, var);
            return v;
        } catch(out_of_range) {
            return Error::errorValue(ErrorType::UndeclaredVariable, { lefte->getName() });
        }
    }
    // Index (temporary, should be in BuiltFunction::createIndex)
    if(functionName == "__index") {
        Value* left = arguments[0]->codegen();
        if(left->getType()->isArrayTy()) {
            Value* right = arguments[1]->codegen();
            if(!right->getType()->isIntegerTy(64))
                return Error::errorValue(ErrorType::WrongArgumentType, { "must be int" });
            Constant* one = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 1);
            Constant* zero = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0);
            right = builder.CreateSub(right, one);
            Value* ind[] = { zero, right };
            AllocaInst* aleft = dyn_cast<AllocaInst>(left);
            if(!aleft) {
                aleft = builder.CreateAlloca(left->getType(), 0, "atmp");
                builder.CreateStore(left, aleft);
            }
            Value* tmpptr = builder.CreateInBoundsGEP(aleft, ind, "indtmpptr");
            return builder.CreateLoad(tmpptr, "indtmp");

        } else if(left->getType()->isPointerTy()) {
            Type* elTy = cast<PointerType>(left->getType())->getElementType();
            if(!elTy->isStructTy())
                return Error::errorValue(ErrorType::IndexArgumentError);
            if(dynamic_cast<FunctionCallAST*>(arguments[1].get())) {
                FunctionCallAST* fca = dynamic_cast<FunctionCallAST*>(arguments[1].get());
                string tpn = cast<StructType>(elTy)->getName().str();
                printf("Calling method '%s' of type '%s'", fca->getFunctionName().c_str(), tpn.c_str());
                vector<Value*> vals;
                vals.push_back(left);
                for(auto a : fca->getArguments())
                    vals.push_back(a->codegen());
                vector<Type*> tps;
                for(auto v : vals)
                    tps.push_back(v->getType());
                string name = FunctionNameMangler::mangleName(fca->getFunctionName(), tps, "Helen", tpn);
                vector<string> fie = fields[tpn];
                auto field = std::find(fie.begin(), fie.end(), name);
                if(field == fie.end()) {
                    return Error::errorValue(ErrorType::UndeclaredFunction, { name, name });
                }
                int pos = field - fie.begin();
                Value* zero = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
                Value* ind = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), pos);
                Value* right[] = { zero, ind };
                Value* tmpptr = builder.CreateInBoundsGEP(left, right, "indtmpptr");
                Value* f = builder.CreateLoad(tmpptr, "indtmp");
                return builder.CreateCall(f, vals, "calltmp");
                // TODO: add 'real' method call!
            }
            if(!dynamic_cast<VariableAST*>(arguments[1].get()))
                return Error::errorValue(ErrorType::WrongArgumentType, { "must be ID" });
            string name = ((VariableAST*)arguments[1].get())->getName();
            vector<string> fie = fields[static_cast<StructType*>(elTy)->getName()];
            auto field = std::find(fie.begin(), fie.end(), name);
            if(field == fie.end()) {
                return Error::errorValue(ErrorType::UndeclaredVariable, { name });
            }
            int pos = field - fie.begin();
            Value* zero = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
            Value* ind = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), pos);
            Value* right[] = { zero, ind };
            /*AllocaInst* aleft = builder.CreateAlloca(left->getType(), 0, "atmp");
            builder.CreateStore(left, aleft);*/
            Value* tmpptr = builder.CreateInBoundsGEP(left, right, "indtmpptr");
            return builder.CreateLoad(tmpptr, "indtmp");
        } else {
            return Error::errorValue(ErrorType::IndexArgumentError);
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
    // try to find mangled function, then unmangled one
    Function* f = 0;
    string oldFName = functionName;
    for(string style : { "Helen", "C" }) {
        functionName = FunctionNameMangler::mangleName(oldFName, types, style, methodType);
        f = module->getFunction(functionName);
        if(f)
            break;
    }
    string hrName = FunctionNameMangler::humanReadableName(functionName);
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
    set<string> styles = { "C", "Helen" };
    string method = "__method_";
    string className = "";
    if(boost::algorithm::starts_with(style, method)) {
        className = style.substr(method.size());
        printf("Found a method %s of class %s\n", name.c_str(), className.c_str());
        style = "Helen"; // methods can be only Helen-style
    }
    if(!className.empty()) {
        // add 'this' parameter
        args.insert(args.begin(), PointerType::get(types[className], 0));
        argNames.insert(argNames.begin(), "this");
    }
    if(!styles.count(style))
        return (Function*)Error::errorValue(ErrorType::UnknownStyle);
    FunctionType* ft = FunctionType::get(returnType, args, false);
    name = FunctionNameMangler::mangleName(name, args, style, className);
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
    // callstack.push(proto->getName());
    BasicBlock* parent = builder.GetInsertBlock();
    BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "entry", f);
    builder.SetInsertPoint(bb);
    for(auto& arg : f->args()) {
        AllocaInst* alloca = createEntryBlockAlloca(f, arg.getType(), arg.getName());
        builder.CreateStore(&arg, alloca);
        variables[arg.getName()] = alloca;
    }
    if(!proto->getReturnType()->isVoidTy()) {
        AllocaInst* alloca = createEntryBlockAlloca(f, proto->getReturnType(), proto->getName());
        builder.CreateStore(Constant::getNullValue(proto->getReturnType()), alloca);
        variables[proto->getOriginalName()] = alloca;
    }
    Value* ret = body->codegen();
    if(!ret && !proto->getReturnType()->isVoidTy())
        ret = builder.CreateLoad(variables[proto->getOriginalName()]);
    builder.CreateRet(ret);
    verifyFunction(*f);
    fpm->run(*f);
    //        callstack.pop();
    //        string previous = callstack.empty() ? "main" : callstack.top();
    //        BasicBlock* bb = BasicBlock::Create(getGlobalContext(), "resume", module->getFunction(previous));
    //        builder.SetInsertPoint(bb);
    builder.SetInsertPoint(parent);
    return f;
    /*callstack.pop();
    string previous = callstack.empty() ? "_main_v" : callstack.top();
    bb = BasicBlock::Create(getGlobalContext(), "resume", module->getFunction(previous));
    builder.SetInsertPoint(bb);*/
    f->eraseFromParent();
    builder.SetInsertPoint(parent);
    return nullptr;
}
Value* ReturnAST::codegen()
{
    Value* ret = result->codegen();
    return ret;
}
Value* CustomTypeAST::codegen()
{
    int ind = 0;
    for(shared_ptr<AST> i : instructions) {
        if(dynamic_cast<FunctionPrototypeAST*>(i.get())) {
            FunctionPrototypeAST* fpi = (FunctionPrototypeAST*)i.get();
            fpi->getStyle() = "__method_" + typeName;
            fpi->codegen();
            //string name = FunctionNameMangler::mangleName(fpi->getOriginalName(), fpi->getArgs(), "Helen", typeName);
            fields[typeName][ind] = fpi->getName();
        }
        ind++;
    }
    StructType* t = cast<StructType>(types[typeName]);
    for(int i = 0; i < t->getNumElements(); i++) {
    }
    return 0;
}
void CustomTypeAST::compileTime()
{

    std::vector<string> fieldNames;
    std::vector<Type*> fieldTypes;
    StructType* st = StructType::create(getGlobalContext(), typeName);
    for(shared_ptr<AST> i : instructions) {
        if(dynamic_cast<DeclarationAST*>(i.get())) {
            DeclarationAST* di = (DeclarationAST*)i.get();
            fieldNames.push_back(di->getName());
            fieldTypes.push_back(di->getType());
        }
        if(dynamic_cast<FunctionPrototypeAST*>(i.get())) {
            FunctionPrototypeAST* fpi = (FunctionPrototypeAST*)i.get();
            // printf("Got method '%s'\n", fpi->getOriginalName().c_str());
            fieldNames.push_back(fpi->getOriginalName());
            vector<Type*> args = fpi->getArgs();
            args.insert(args.begin(), PointerType::get(st, 0));
            Type* pf = PointerType::get(FunctionType::get(fpi->getReturnType(), args, false), 0);
            fieldTypes.push_back(pf);
        }
    }
    st->setBody(ArrayRef<Type*>(fieldTypes), false);
    fields[typeName] = fieldNames;
    types[typeName] = st;
}

Value* NewAST::codegen()
{
    if(!type->isPointerTy())
        return Error::errorValue(ErrorType::WrongArgumentType, { "non-struct type" });
    if(!cast<PointerType>(type)->getElementType()->isStructTy())
        return Error::errorValue(ErrorType::WrongArgumentType, { "non-struct type" });
    Type* ptrType = type;
    type = cast<PointerType>(type)->getElementType();
    size_t size = dataLayout->getTypeStoreSize(type);
    Value* memoryPtr = builder.CreateCall(
        module->getFunction("malloc"), ConstantInt::get(Type::getInt32Ty(getGlobalContext()), size), "memtmp");
    Value* v = builder.CreateBitCast(memoryPtr, ptrType, "newtmp");
    if(((PointerType*)(v->getType()))->getElementType()->isStructTy()) {
        StructType* s = (StructType*)(((PointerType*)(v->getType()))->getElementType());
        for(int i = 0; i < s->getNumElements(); i++) {
            if(s->getElementType(i)->isPointerTy()) {
                if(((PointerType*)(s->getElementType(i)))->getElementType()->isFunctionTy()) {
                    // Load the method
                    Function* f = module->getFunction(fields[s->getName()][i]);
                    Constant* idx = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), i);
                    Constant* zero = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
                    Value* ind[] = { zero, idx };
                    Value* tmp = builder.CreateInBoundsGEP(v, ind, "tmp");
                    Value* ft = builder.CreateInBoundsGEP(f, zero, "mtmp");
                    builder.CreateStore(ft, tmp);
                }
            }
        }
    }
    return v;
}

Value* DeleteAST::codegen()
{
    try {
        Value* val = variables.at(var);
        Value* addr = builder.CreateLoad(val, "freetmp");
        if(!addr->getType()->isPointerTy())
            return Error::errorValue(ErrorType::NonObjectType);
        if(!cast<PointerType>(addr->getType())->getElementType()->isStructTy())
            return Error::errorValue(ErrorType::NonObjectType);
        addr = builder.CreateBitCast(addr, Type::getInt8PtrTy(getGlobalContext()), "freetmp");
        builder.CreateCall(module->getFunction("free"), addr);
    } catch(out_of_range) {
        return Error::errorValue(ErrorType::UndeclaredVariable, { var });
    }
    return 0;
}
}

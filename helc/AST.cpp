//
// Created by main on 20.09.2015.
//

#include "AST.h"
#include "BuiltinFunctions.h"
#include "Error.h"
#include "FunctionNameMangler.h"
#include "Generics.h"
#include <boost/algorithm/string.hpp>
#include <set>

namespace Helen {
LLVMContext AST::context;
unique_ptr<Module> AST::module = nullptr;
unique_ptr<DataLayout> AST::dataLayout = nullptr;
unique_ptr<legacy::FunctionPassManager> AST::fpm = nullptr;
IRBuilder<> AST::builder(AST::context);
map<string, AllocaInst *> AST::variables;
map<string, Function *> AST::functions;
map<string, Type *> AST::types;
set<string> AST::typesCStyle;
map<string, vector<string>> AST::fields;
map<string, GenericFunction *> AST::genericFunctions;
stack<string> AST::callstack;
bool AST::isMainModule = false;

Type *TypeInfo::getType() { return type ? type : AST::types[name]; }

AllocaInst *AST::createEntryBlockAlloca(Function *f, Type *t,
                                        const std::string &VarName) {
  IRBuilder<> tmp(&f->getEntryBlock(), f->getEntryBlock().begin());
  return tmp.CreateAlloca(t, nullptr, VarName.c_str());
}

Value *ConstantIntAST::codegen() {
  return ConstantInt::get(IntegerType::get(AST::context, bitwidth), value);
}

Value *ConstantRealAST::codegen() {
  return ConstantFP::get(AST::context, APFloat(value));
}

Value *ConstantCharAST::codegen() {
  return ConstantInt::get(Type::getInt8Ty(AST::context), value);
}

Value *ConstantStringAST::codegen() {
  if (types.find("String") == types.end())
    return Error::errorValue(ErrorType::UndeclaredType, {"String"});
  Constant *gs = ConstantDataArray::getString(AST::context, StringRef(value));
  Type *stype =
      ArrayType::get(IntegerType::get(AST::context, 8), value.size() + 1);
  GlobalVariable *var = new GlobalVariable(
      *module.get(), stype, true, GlobalValue::PrivateLinkage, gs, ".strconst");
  Constant *zero =
      Constant::getNullValue(llvm::IntegerType::getInt32Ty(AST::context));
  Constant *ind[] = {zero, zero};
  Constant *v = ConstantExpr::getGetElementPtr(stype, var, ind);
  Value *len = ConstantInt::get(Type::getInt32Ty(AST::context), value.size());
  Type *type = types["String"];
  Type *ptrType = PointerType::get(type, 0);
  size_t size = dataLayout->getTypeStoreSize(type);
  Value *memoryPtr = builder.CreateCall(
      module->getFunction("malloc"),
      ConstantInt::get(Type::getInt32Ty(AST::context), size), "memtmp");
  Value *msvals[] = {memoryPtr,
                     ConstantInt::get(Type::getInt32Ty(AST::context), 0),
                     ConstantInt::get(Type::getInt32Ty(AST::context), size)};
  builder.CreateCall(module->getFunction("memset"), msvals);
  Value *str = builder.CreateBitCast(memoryPtr, ptrType, "newtmp");
  // Loading methods
  StructType *s = static_cast<StructType *>(type);
  for (uint32_t i = 0; i < s->getNumElements(); i++) {
    if (s->getElementType(i)->isPointerTy()) {
      if ((static_cast<PointerType *>(s->getElementType(i)))
              ->getElementType()
              ->isFunctionTy()) {
        // Load the method
        Function *f = module->getFunction(fields[s->getName()][i]);
        Constant *idx = ConstantInt::get(Type::getInt32Ty(AST::context), i);
        Constant *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
        Value *ind[] = {zero, idx};
        Value *tmp = builder.CreateInBoundsGEP(str, ind, "tmp");
        if (f) {
          // Value *ft = builder.CreateInBoundsGEP(f, zero, "mtmp");
          builder.CreateStore(f, tmp);
        }
      }
    }
  }
  std::vector<Value *> argValues = {str, v, len};
  std::vector<Type *> argTypes = {str->getType(), v->getType(), len->getType()};
  string ctorname = FunctionNameMangler::mangleName(
      "__ctor", argTypes, "Helen",
      (static_cast<StructType *>(type))->getName());
  Function *ctor = module->getFunction(ctorname); // functions[ctorname];
  // printf("ctor=%d %s\n", ctor, ctorname.c_str());
  if (ctor) {
    builder.CreateCall(ctor, argValues);
  } else {
    // TODO: Error
  }
  return str;
}

Value *DeclarationAST::codegen() {
  Function *f = builder.GetInsertBlock()->getParent();

  Value *initValue;
  if (initialiser) {
    initValue = initialiser->codegen();
    if (!initValue)
      return nullptr;
  } else {
    initValue = Constant::getNullValue(typeInfo.getType());
  }
  AllocaInst *alloca = createEntryBlockAlloca(f, typeInfo.getType(), name);
  if (initValue)
    builder.CreateStore(initValue, alloca);
  variables[name] = alloca;
  // TODO: Why nullptr here?
  return nullptr;
}

Value *VariableAST::codegen() {
  try {
    Value *val = variables.at(name);
    return builder.CreateLoad(val, name.c_str());
  } catch (out_of_range) {
    return Error::errorValue(ErrorType::UndeclaredVariable, {name});
  }
}

Value *ArrayInitialiserAST::codegen() {
  // TODO: Replace with generic array class
  Constant *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
  ArrayType *atp = ArrayType::get(elementTypeInfo.type, arguments.size());
  Type *etp = atp->getElementType();
  size_t size = dataLayout->getTypeStoreSize(atp);
  Value *memoryPtr = builder.CreateCall(
      module->getFunction("malloc"),
      ConstantInt::get(Type::getInt32Ty(AST::context), size), "memtmp");
  Value *arr = builder.CreateBitCast(memoryPtr, atp, "arrtmp");
  for (uint32_t i = 0; i < arguments.size(); i++) {
    Constant *idx = ConstantInt::get(Type::getInt32Ty(AST::context), i);
    Value *ind[] = {zero, idx};
    Value *v = builder.CreateInBoundsGEP(arr, ind, "indtmpptr");
    Value *elem = arguments[i]->codegen();
    if (CastInst::isCastable(elem->getType(), etp)) {
      auto opc = CastInst::getCastOpcode(elem, true, etp, true);
      Value *casted = builder.CreateCast(opc, elem, etp, "casttmp");
      builder.CreateStore(casted, v);

    } else if (CastInst::isBitOrNoopPointerCastable(v->getType(), etp,
                                                    *dataLayout.get())) {
      Value *casted = builder.CreateBitOrPointerCast(elem, etp, "casttmp");
      builder.CreateStore(casted, v);
    }
    // TODO: else error (uncastable)
  }
  // TODO: why nullptr here?
  return nullptr;
}

Value *ConditionAST::codegen() {
  Value *cond = condition->codegen();
  if (!cond)
    return nullptr;
  Type *condtype = cond->getType();
  if (condtype->isIntegerTy())
    cond =
        builder.CreateICmpNE(cond, Constant::getNullValue(condtype), "ifcond");
  if (condtype->isDoubleTy())
    cond =
        builder.CreateFCmpONE(cond, Constant::getNullValue(condtype), "ifcond");
  Function *f = builder.GetInsertBlock()->getParent();
  BasicBlock *thenBB = BasicBlock::Create(AST::context, "then", f);
  BasicBlock *elseBB = BasicBlock::Create(AST::context, "else");
  BasicBlock *mergeBB = BasicBlock::Create(AST::context, "ifcont");

  builder.CreateCondBr(cond, thenBB, elseBB);
  // Emit then value.
  builder.SetInsertPoint(thenBB);

  Value *thenValue = thenBranch->codegen();
  if (!thenValue)
    thenValue = ConstantInt::get(IntegerType::get(AST::context, 64), 0);

  builder.CreateBr(mergeBB);
  thenBB = builder.GetInsertBlock();

  f->getBasicBlockList().push_back(elseBB);
  builder.SetInsertPoint(elseBB);

  Value *elseValue = elseBranch->codegen();
  if (!elseValue)
    elseValue = ConstantInt::get(IntegerType::get(AST::context, 64), 0);

  builder.CreateBr(mergeBB);
  elseBB = builder.GetInsertBlock();

  f->getBasicBlockList().push_back(mergeBB);
  builder.SetInsertPoint(mergeBB);
  /*PHINode* PN = builder.CreatePHI(Type::getInt32Ty(AST::context), 2,
  "iftmp");

  PN->addIncoming(thenValue, thenBB);
  PN->addIncoming(elseValue, elseBB);
  return PN;*/
  return nullptr;
}

Value *LoopAST::codegen() {
  if (initial)
    initial->codegen();
  Function *f = builder.GetInsertBlock()->getParent();
  // BasicBlock *preheaderBB = builder.GetInsertBlock();
  BasicBlock *loopBB = BasicBlock::Create(AST::context, "loop", f);
  builder.CreateBr(loopBB);
  builder.SetInsertPoint(loopBB);
  body->codegen();
  if (iteration)
    iteration->codegen();
  Value *cond = condition->codegen();
  Type *condtype = cond->getType();
  if (condtype->isIntegerTy())
    cond =
        builder.CreateICmpNE(cond, Constant::getNullValue(condtype), "condtmp");
  if (condtype->isDoubleTy())
    cond = builder.CreateFCmpONE(cond, Constant::getNullValue(condtype),
                                 "condtmp");
  // BasicBlock *loopEndBB = builder.GetInsertBlock();
  BasicBlock *afterBB = BasicBlock::Create(AST::context, "afterloop", f);
  builder.CreateCondBr(cond, loopBB, afterBB);
  builder.SetInsertPoint(afterBB);
  return Constant::getNullValue(Type::getInt32Ty(AST::context));
}

Value *FunctionCallAST::codegen() {
  // Assignment is the special case
  if (functionName == BuiltinFunctions::operatorMarker + "=") {
    shared_ptr<AST> left = arguments[0], right = arguments[1];
    FunctionCallAST *leftf = dynamic_cast<FunctionCallAST *>(left.get());
    if (leftf) {
      string fname = leftf->getFunctionName();
      // Here must be check for reference
      if (fname == "__index") {
        Value *arr = leftf->arguments[0]->codegen();
        if (arr->getType()->isArrayTy()) {
          Value *idx = leftf->arguments[1]->codegen();
          if (!idx->getType()->isIntegerTy(64))
            return Error::errorValue(ErrorType::WrongArgumentType,
                                     {"must be int"});
          Constant *one = ConstantInt::get(Type::getInt32Ty(AST::context), 1);
          Constant *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
          idx = builder.CreateSub(idx, one);
          Value *ind[] = {zero, idx};
          if (!dynamic_cast<VariableAST *>(leftf->arguments[0].get())) {
            return Error::errorValue(
                ErrorType::AssignmentError,
                {std::to_string(reinterpret_cast<size_t>(left.get()))});
          }
          AllocaInst *aarr = variables.at(
              (static_cast<VariableAST *>(leftf->arguments[0].get()))
                  ->getName());
          ;
          builder.CreateStore(arr, aarr);
          Value *v = builder.CreateInBoundsGEP(aarr, ind, "indtmpptr");
          return builder.CreateStore(right->codegen(), v);
        } else if (arr->getType()->isPointerTy()) {
          Type *elTy = cast<PointerType>(arr->getType())->getElementType();
          if (!elTy->isStructTy())
            return Error::errorValue(ErrorType::IndexArgumentError);
          if (!dynamic_cast<VariableAST *>(leftf->arguments[1].get()))
            return Error::errorValue(ErrorType::WrongArgumentType,
                                     {"must be ID"});
          string name = (static_cast<VariableAST *>(leftf->arguments[1].get()))
                            ->getName();
          vector<string> fie =
              fields[static_cast<StructType *>(elTy)->getName()];
          auto field = std::find(fie.begin(), fie.end(), name);
          if (field == fie.end()) {
            return Error::errorValue(ErrorType::UndeclaredVariable, {name});
          }
          int64_t pos = field - fie.begin();
          Value *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
          Value *ind = ConstantInt::get(Type::getInt32Ty(AST::context),
                                        static_cast<uint64_t>(pos));
          Value *idx[] = {zero, ind};
          if (!dynamic_cast<VariableAST *>(leftf->arguments[0].get())) {
            return Error::errorValue(
                ErrorType::AssignmentError,
                {std::to_string(reinterpret_cast<size_t>(left.get()))});
          }
          Value *tmpptr = builder.CreateInBoundsGEP(arr, idx, "indtmpptr");
          Value *v = builder.CreateStore(right->codegen(), tmpptr);
          return v;
        } else {
          return Error::errorValue(ErrorType::IndexArgumentError);
        }
      }
    }
    VariableAST *lefte = dynamic_cast<VariableAST *>(left.get());
    if (!lefte)
      return Error::errorValue(
          ErrorType::AssignmentError,
          {std::to_string(reinterpret_cast<size_t>(left.get()))});
    Value *v = right->codegen();
    if (!v)
      return nullptr;
    try {
      Value *var = variables.at(lefte->getName());
      builder.CreateStore(v, var);
      return v;
    } catch (out_of_range) {
      return Error::errorValue(ErrorType::UndeclaredVariable,
                               {lefte->getName()});
    }
  }
  // Index (temporary, should be in BuiltFunction::createIndex)
  if (functionName == "__index") {
    Value *left = arguments[0]->codegen();
    if (left->getType()->isArrayTy()) {
      Value *right = arguments[1]->codegen();
      if (!right->getType()->isIntegerTy(64))
        return Error::errorValue(ErrorType::WrongArgumentType, {"must be int"});
      Constant *one = ConstantInt::get(Type::getInt32Ty(AST::context), 1);
      Constant *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
      right = builder.CreateSub(right, one);
      Value *ind[] = {zero, right};
      AllocaInst *aleft = dyn_cast<AllocaInst>(left);
      if (!aleft) {
        aleft = builder.CreateAlloca(left->getType(), nullptr, "atmp");
        builder.CreateStore(left, aleft);
      }
      Value *tmpptr = builder.CreateInBoundsGEP(aleft, ind, "indtmpptr");
      return builder.CreateLoad(tmpptr, "indtmp");

    } else if (left->getType()->isPointerTy()) {
      Type *elTy = cast<PointerType>(left->getType())->getElementType();
      if (!elTy->isStructTy())
        return Error::errorValue(ErrorType::IndexArgumentError);
      if (dynamic_cast<FunctionCallAST *>(arguments[1].get())) {
        FunctionCallAST *fca =
            dynamic_cast<FunctionCallAST *>(arguments[1].get());
        string tpn = cast<StructType>(elTy)->getName().str();
        vector<Value *> vals;
        vals.push_back(left);
        for (auto a : fca->getArguments())
          vals.push_back(a->codegen());
        vector<Type *> tps;
        for (auto v : vals)
          tps.push_back(v->getType());
        string name = FunctionNameMangler::mangleName(fca->getFunctionName(),
                                                      tps, "Helen", tpn);
        vector<string> fie = fields[tpn];
        auto field = std::find(fie.begin(), fie.end(), name);
        // printf("***fie***\n");
        // for(auto f : fie)
        //    printf("%s\n\n", f.c_str());
        if (field == fie.end()) {
          return Error::errorValue(ErrorType::UndeclaredFunction, {name, name});
        }
        int64_t pos = field - fie.begin();
        Value *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
        Value *ind = ConstantInt::get(Type::getInt32Ty(AST::context),
                                      static_cast<uint64_t>(pos));
        Value *right[] = {zero, ind};
        Value *tmpptr = builder.CreateInBoundsGEP(left, right, "indtmpptr");
        Value *f = builder.CreateLoad(tmpptr, "indtmp");
        return builder.CreateCall(f, vals, "calltmp");
      }
      if (!dynamic_cast<VariableAST *>(arguments[1].get()))
        return Error::errorValue(ErrorType::WrongArgumentType, {"must be ID"});
      string name = (static_cast<VariableAST *>(arguments[1].get()))->getName();
      vector<string> fie = fields[static_cast<StructType *>(elTy)->getName()];
      auto field = std::find(fie.begin(), fie.end(), name);
      if (field == fie.end()) {
        return Error::errorValue(ErrorType::UndeclaredVariable, {name});
      }
      int64_t pos = field - fie.begin();
      Value *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
      Value *ind = ConstantInt::get(Type::getInt32Ty(AST::context),
                                    static_cast<uint64_t>(pos));
      Value *right[] = {zero, ind};
      Value *tmpptr = builder.CreateInBoundsGEP(left, right, "indtmpptr");
      return builder.CreateLoad(tmpptr, "indtmp");
    } else {
      return Error::errorValue(ErrorType::IndexArgumentError);
    }
  }
  std::vector<Value *> vargs;
  std::vector<Type *> types;
  for (uint64_t i = 0, e = arguments.size(); i != e; ++i) {
    // TODO: add type checking
    vargs.push_back(arguments[i]->codegen());
    if (!vargs.back())
      return nullptr;
  }
  for (Value *v : vargs)
    types.push_back(v->getType());
  // try to find mangled function, then unmangled one
  Function *f = nullptr;
  string oldFName = functionName;
  for (string style : {"Helen", "C"}) {
    functionName =
        FunctionNameMangler::mangleName(oldFName, types, style, methodType);
    f = module->getFunction(functionName);
    if (f)
      break;
  }
  string hrName = FunctionNameMangler::humanReadableName(functionName);
  string fName = FunctionNameMangler::functionName(functionName);
  // printf("hrName = %s; fName = %s\n", hrName.c_str(), fName.c_str());
  bool shouldCast = 0;
  if (!f) {
    // TODO: Try to find the suitable function with same name but different
    // signature
    vector<Function *> suitableFunctions;
    for (Function &fu : module->getFunctionList()) {
      if (FunctionNameMangler::functionName(fu.getName()) == fName) {
        // TODO: Check for arguments' castability, then call the most suitable
        ArrayRef<Type *> params = fu.getFunctionType()->params();
        if (params.size() != vargs.size() ||
            (fu.isVarArg() && params.size() > vargs.size()))
          continue;
        bool available = 1;
        for (unsigned i = 0; i < (fu.isVarArg() ? params.size() : vargs.size());
             i++)
          if (!CastInst::isCastable(vargs[i]->getType(), params[i])) {
            available = 0;
            break;
          }
        if (!available)
          continue;
        suitableFunctions.push_back(&fu);
      }
    }
    if (suitableFunctions.empty())
      return Error::errorValue(ErrorType::UndeclaredFunction,
                               {hrName, functionName});
    // printf("%d function candidates found\n", suitableFunctions.size());
    f = suitableFunctions[0];
    shouldCast = 1;
    // TODO:
  }
  ArrayRef<Type *> params = f->getFunctionType()->params();
  for (unsigned i = 0; i < (f->isVarArg() ? params.size() : vargs.size()); i++)
    if (!shouldCast && vargs[i]->getType() != params[i])
      return Error::errorValue(ErrorType::WrongArgumentType,
                               {std::to_string(i)});
  if (shouldCast)
    for (unsigned i = 0; i < (f->isVarArg() ? params.size() : vargs.size());
         i++) {
      auto opc = CastInst::getCastOpcode(vargs[i], true, params[i], true);
      vargs[i] = builder.CreateCast(opc, vargs[i], params[i]);
    }
  return f->getReturnType()->isVoidTy()
             ? builder.CreateCall(f, vargs)
             : builder.CreateCall(f, vargs, "calltmp");
}

Value *SequenceAST::codegen() {
  for (shared_ptr<Helen::AST> &a : instructions) {
    Value *v = a->codegen();
    if (dynamic_cast<ReturnAST *>(a.get()))
      return v;
  }
  return nullptr;
}

Value *NullAST::codegen() { return nullptr; }

Function *FunctionPrototypeAST::codegen() {
  set<string> styles = {"C", "Helen"};
  string method = "__method_";
  string className = "";
  if (boost::algorithm::starts_with(style, method)) {
    className = style.substr(method.size());
    style = "Helen"; // methods can be only Helen-style
  }
  if (!className.empty()) {
    // add 'this' parameter
    auto thistype = PointerType::get(types[className], 0);

    args.insert(args.begin(), {className, thistype});
    argNames.insert(argNames.begin(), "this");
  }
  vector<string> genTypenames;
  // if it's generic, we must adjust type params and mangle them into the name
  if (!genericParams.empty()) {
    for (string s : genericParams) {
      Type *tp = genericInstTypes[s].getType();
      string st = FunctionNameMangler::typeString(tp);
      genTypenames.push_back(st);
    }
  }
  if (!styles.count(style))
    return static_cast<Function *>(Error::errorValue(ErrorType::UnknownStyle));
  std::vector<Type *> argtypes;
  for (auto p : args)
    argtypes.push_back(p.getType());
  // a "kludge" (workaround) for generic functions
  Type *rtype = genericParams.empty() ? returnType.type : returnType.getType();
  FunctionType *ft = FunctionType::get(
      rtype ? rtype : PointerType::get(types[className], 0), argtypes, vararg);
  name = FunctionNameMangler::mangleName(name, argtypes, style, className,
                                         genTypenames);
  Function *f = functions[name];
  if (f)
    return f;
  f = Function::Create(ft, Function::ExternalLinkage, name, module.get());
  functions[name] = f;

  unsigned i = 0;
  for (auto &arg : f->args()) {
    arg.setName(argNames[i++]);
  }
  return f;
}

Function *FunctionAST::codegen() {
  // If it's generic and no need for instantiation, skip this step
  if (!proto->getGenericParams().empty() && !shouldInstantiate) {
    if (!genericFunctions[proto->getName()])
      genericFunctions[proto->getName()] = new GenericFunction(this);
    return nullptr;
  }
  shouldInstantiate = 0;

  Function *f = module->getFunction(proto->getName());

  if (!f)
    f = proto->codegen();

  if (!f)
    return nullptr;

  if (!f->empty())
    return static_cast<Function *>(
        Error::errorValue(ErrorType::FunctionRedefined, {proto->getName()}));
  BasicBlock *parent = builder.GetInsertBlock();
  BasicBlock *bb = BasicBlock::Create(AST::context, "entry", f);
  builder.SetInsertPoint(bb);
  for (auto &arg : f->args()) {
    AllocaInst *alloca =
        createEntryBlockAlloca(f, arg.getType(), arg.getName());
    builder.CreateStore(&arg, alloca);
    variables[arg.getName()] = alloca;
  }
  if (!proto->getReturnType()->isVoidTy()) {
    AllocaInst *alloca =
        createEntryBlockAlloca(f, proto->getReturnType(), proto->getName());
    builder.CreateStore(Constant::getNullValue(proto->getReturnType()), alloca);
    variables[proto->getOriginalName()] = alloca;
  }
  Value *ret = body->codegen();
  if (!ret && !proto->getReturnType()->isVoidTy())
    ret = builder.CreateLoad(variables[proto->getOriginalName()]);
  builder.CreateRet(ret);
  verifyFunction(*f);
  fpm->run(*f);
  builder.SetInsertPoint(parent);
  return f;
}

Value *GenericFunctionInstanceAST::codegen() {
  GenericFunction *f = genericFunctions[genObjectName];
  if (!f) {
    return Error::errorValue(ErrorType::InvalidInstantiation, {genObjectName});
  }
  map<string, Type *> instanceTypes;
  vector<string> p = f->getAST()->getPrototype()->getGenericParams();
  uint64_t idx = 0;
  for (TypeInfo t : typeParams) {
    // TODO: if (!types[s])
    instanceTypes[p[idx++]] = t.getType();
  }
  if (p.size() != idx)
    return Error::errorValue(ErrorType::WrongArgumentNumber,
                             {to_string(p.size()), to_string(idx)});
  return f->instantiate(instanceTypes);
}

Value *ShiftbyAST::codegen() {
  Value *ptr = pointer->codegen(), *amt = amount->codegen();
  if (!ptr->getType()->isPointerTy() || !amt->getType()->isIntegerTy())
    return Error::errorValue(ErrorType::InvalidShiftbyUse);
  return builder.CreateGEP(ptr, amt);
}

Value *ReturnAST::codegen() {
  Value *ret = result->codegen();
  return ret;
}

Value *CustomTypeAST::codegen() {
  uint64_t ind = 0;
  if (!baseTypeName.empty())
    ind = (static_cast<StructType *>(types[baseTypeName])->getNumElements());
  for (shared_ptr<AST> i : instructions) {
    // printf("ind=%d\n", ind);
    if (dynamic_cast<FunctionPrototypeAST *>(i.get())) {
      FunctionPrototypeAST *fpi = static_cast<FunctionPrototypeAST *>(i.get());
      fpi->getStyle() = "__method_" + typeName;
      fpi->codegen();
      vector<Type *> fpitypes;
      for (auto a : fpi->getArgs())
        fpitypes.push_back(a.getType());
      string name = FunctionNameMangler::mangleName(
          fpi->getOriginalName(), fpitypes, "Helen", typeName);
      if (find(fields[typeName].begin(), fields[typeName].end(), name) ==
          fields[typeName].end()) {
        // printf("ind=%d f=%40s len=%d\n", ind, fpi->getName().c_str(),
        // fields[typeName].size());  fflush(stdout);
        fields[typeName][ind] = fpi->getName();
        ind++;
      }
    } else
      ind++;
  }
  vector<string> fieldNames = fields[typeName];
  vector<Type *> fieldTypes =
      (static_cast<StructType *>(types[typeName]))->elements();
  for (uint64_t i = 0; i < bstc; i++) {
    if (fieldTypes[i]->isPointerTy()) {
      if ((static_cast<PointerType *>(fieldTypes[i]))
              ->getElementType()
              ->isFunctionTy() &&
          find(overriddenMethods.begin(), overriddenMethods.end(),
               fieldNames[i]) == overriddenMethods.end()) {
        // printf("Method '%s' generated as base-class\n",
        // fieldNames[i].c_str());
        string mname = fieldNames[i];
        string bmname = mname;
        // Replace method type name
        boost::algorithm::replace_first(bmname, typeName + "-",
                                        baseTypeName + "-");
        // Replace type parameter for 'this'
        boost::algorithm::replace_first(bmname, "_type." + typeName,
                                        "_type." + baseTypeName);
        // printf("The base class method is '%s'\n", bmname.c_str());
        FunctionType *ft = cast<FunctionType>(
            (static_cast<PointerType *>(fieldTypes[i]))->getElementType());
        Function *f = Function::Create(ft, Function::ExternalLinkage, mname,
                                       module.get());
        BasicBlock *parent = builder.GetInsertBlock();
        BasicBlock *bb = BasicBlock::Create(AST::context, "entry", f);
        builder.SetInsertPoint(bb);
        vector<Value *> baseargs;
        Function *bf = module->getFunction(bmname);
        // TODO: error if not bf
        int idx = 0;
        for (auto &arg : f->args()) {
          arg.setName(
              (static_cast<Argument *>(bf->arg_begin()))[idx++].getName());
          AllocaInst *alloca =
              createEntryBlockAlloca(f, arg.getType(), arg.getName());
          builder.CreateStore(&arg, alloca);
          variables[arg.getName()] = alloca;
          baseargs.push_back(builder.CreateLoad(alloca));
        }
        Value *basethis = builder.CreateBitCast(
            builder.CreateLoad(variables["this"]),
            PointerType::get(types[baseTypeName], 0), "base");
        baseargs[0] = basethis;
        Value *ret = bf->getReturnType()->isVoidTy()
                         ? builder.CreateCall(bf, baseargs)
                         : builder.CreateCall(bf, baseargs, "calltmp");
        bf->getReturnType()->isVoidTy() ? builder.CreateRetVoid()
                                        : builder.CreateRet(ret);
        builder.SetInsertPoint(parent);
      }
    }
  }
  StructType *t = cast<StructType>(types[typeName]);
  for (uint64_t i = 0; i < t->getNumElements(); i++) {
  }
  return nullptr;
}

void CustomTypeAST::compileTime() {
  vector<string> fieldNames;
  vector<Type *> fieldTypes;
  StructType *st;
  if (Type *tp = AST::types[typeName]) {
    if ((static_cast<StructType *>(tp))->isOpaque()) {
      st = static_cast<StructType *>(tp);
    } else {
      Error::error(ErrorType::UndeclaredType, {typeName});
      return;
    }
  }

  st = StructType::create(AST::context, typeName);
  bstc = 0;
  if (!baseTypeName.empty()) {
    if (isInterface) {
      Error::error(ErrorType::InterfaceInheritedFromType);
      return;
    }
    if (!types[baseTypeName]) {
      Error::error(ErrorType::UndeclaredType, {baseTypeName});
      return;
    }
    fieldNames = fields[baseTypeName];
    Type *bs = types[baseTypeName];
    StructType *bst = cast<StructType>(bs);
    bstc = bst->getNumElements();
    for (uint64_t i = 0; i < bstc; i++) {
      Type *tp = bst->getElementType(static_cast<uint32_t>(i));
      if (tp->isPointerTy()) {
        if ((static_cast<PointerType *>(tp))
                ->getElementType()
                ->isFunctionTy()) {
          FunctionType *ft = static_cast<FunctionType *>(
              (static_cast<PointerType *>(tp))->getElementType());
          auto ret = ft->getReturnType();
          std::vector<Type *> params = ft->params();
          params[0] = PointerType::get(st, 0);
          ft = FunctionType::get(ret, params, false);
          tp = PointerType::get(ft, 0);
          // must mangle here
          // printf("Mangling %s [%d]\n", fieldNames[i].c_str(), i);
          fieldNames[i] = FunctionNameMangler::mangleName(fieldNames[i], params,
                                                          "Helen", typeName);
        }
      }
      fieldTypes.push_back(tp);
    }
  }
  for (shared_ptr<AST> i : instructions) {
    if (dynamic_cast<DeclarationAST *>(i.get())) {
      DeclarationAST *di = static_cast<DeclarationAST *>(i.get());
      if (find(fieldNames.begin(), fieldNames.end(), di->getName()) !=
          fieldNames.end()) {
        // TODO: Add error on re-declaration
        continue;
      }
      fieldNames.push_back(di->getName());
      fieldTypes.push_back(di->getType());
    }
    if (dynamic_cast<FunctionPrototypeAST *>(i.get())) {
      FunctionPrototypeAST *fpi = static_cast<FunctionPrototypeAST *>(i.get());
      vector<TypeInfo> args = fpi->getArgs();
      vector<Type *> argtypes;
      for (auto a : args)
        argtypes.push_back(a.getType());
      argtypes.insert(argtypes.begin(), PointerType::get(st, 0));
      // Finding mangled name inherited from parent class
      string mname = FunctionNameMangler::mangleName(
          fpi->getOriginalName(), argtypes, "Helen", typeName);
      // workaround, not sure for full correctness
      if (/*find(fieldNames.begin(), fieldNames.end(), fpi->getOriginalName())
             != fieldNames.end() ||*/
          find(fieldNames.begin(), fieldNames.end(), mname) !=
          fieldNames.end()) {
        // No need for error, re-declaration means override
        overriddenMethods.push_back(mname);
        continue;
      }
      fieldNames.push_back(fpi->getOriginalName());
      FunctionType *ft = FunctionType::get(
          fpi->getReturnType() ? fpi->getReturnType() : PointerType::get(st, 0),
          argtypes, false);
      Type *pf = PointerType::get(ft, 0);
      fieldTypes.push_back(pf);
    }
  }
  st->setBody(ArrayRef<Type *>(fieldTypes), false);
  fields[typeName] = fieldNames;
  // printf("length=%d\n", fieldNames.size());
  // fflush(stdout);
  types[typeName] = st;
}

Value *NewAST::codegen() {
  Type *ttype = type.getType();
  if (!ttype->isPointerTy())
    return Error::errorValue(ErrorType::WrongArgumentType, {"non-struct type"});
  if (!cast<PointerType>(ttype)->getElementType()->isStructTy())
    return Error::errorValue(ErrorType::WrongArgumentType, {"non-struct type"});
  Type *ptrType = ttype;
  ttype = cast<PointerType>(ttype)->getElementType();
  size_t size = dataLayout->getTypeStoreSize(ttype);
  Value *memoryPtr = builder.CreateCall(
      module->getFunction("malloc"),
      ConstantInt::get(Type::getInt32Ty(AST::context), size), "memtmp");
  Value *msvals[] = {memoryPtr,
                     ConstantInt::get(Type::getInt32Ty(AST::context), 0),
                     ConstantInt::get(Type::getInt32Ty(AST::context), size)};
  builder.CreateCall(module->getFunction("memset"), msvals);
  Value *v = builder.CreateBitCast(memoryPtr, ptrType, "newtmp");
  if ((static_cast<PointerType *>(v->getType()))
          ->getElementType()
          ->isStructTy()) {
    StructType *s = static_cast<StructType *>(
        (static_cast<PointerType *>(v->getType()))->getElementType());
    for (uint64_t i = 0; i < s->getNumElements(); i++) {
      if (s->getElementType(static_cast<uint32_t>(i))->isPointerTy()) {
        if (static_cast<PointerType *>(
                s->getElementType(static_cast<uint32_t>(i)))
                ->getElementType()
                ->isFunctionTy()) {
          // Load the method
          Function *f = module->getFunction(fields[s->getName()][i]);
          Constant *idx = ConstantInt::get(Type::getInt32Ty(AST::context), i);
          Constant *zero = ConstantInt::get(Type::getInt32Ty(AST::context), 0);
          Value *ind[] = {zero, idx};
          Value *tmp = builder.CreateInBoundsGEP(v, ind, "tmp");
          if (f) {
            // Value *ft = builder.CreateInBoundsGEP(f, zero, "mtmp");
            builder.CreateStore(f, tmp);
          }
        }
      }
    }
  }
  std::vector<Value *> argValues = {v};
  std::vector<Type *> argTypes = {v->getType()};
  for (shared_ptr<AST> a : arguments) {
    Value *av = a->codegen();
    argValues.push_back(av);
    argTypes.push_back(av->getType());
  }
  string ctorname = FunctionNameMangler::mangleName(
      "__ctor", argTypes, "Helen",
      (static_cast<StructType *>(ttype))->getName());
  Function *ctor = module->getFunction(ctorname); // functions[ctorname];
  // printf("ctor=%d %s\n", ctor, ctorname.c_str());
  if (ctor) {
    builder.CreateCall(ctor, argValues);
  } else if (!arguments.empty()) {
    // TODO: Error
  }
  return v;
}

Value *DeleteAST::codegen() {
  try {
    Value *val = variables.at(var);
    Value *addr = builder.CreateLoad(val, "freetmp");
    if (!addr->getType()->isPointerTy())
      return Error::errorValue(ErrorType::NonObjectType);
    if (!cast<PointerType>(addr->getType())->getElementType()->isStructTy())
      return Error::errorValue(ErrorType::NonObjectType);
    Type *type = cast<PointerType>(addr->getType())->getElementType();
    string dtorname = FunctionNameMangler::mangleName(
        "__dtor", vector<Type *>(), "Helen",
        (static_cast<StructType *>(type))->getName());
    Function *dtor = module->getFunction(dtorname); // functions[dtorname];
    // printf("dtor=%d %s\n", dtor, dtorname.c_str());
    if (dtor) {
      builder.CreateCall(dtor, addr);
    }
    addr = builder.CreateBitCast(addr, Type::getInt8PtrTy(AST::context),
                                 "freetmp");
    builder.CreateCall(module->getFunction("free"), addr);
  } catch (out_of_range) {
    return Error::errorValue(ErrorType::UndeclaredVariable, {var});
  }
  return nullptr;
}

Value *CastAST::codegen() {
  Value *v = value->codegen();
  if (!v)
    return nullptr;
  // v->getType()->dump();
  // destinationType->dump();
  Type *dtype = destinationType.getType();
  if (CastInst::isCastable(v->getType(), dtype)) {
    auto opc = CastInst::getCastOpcode(v, true, dtype, true);
    return builder.CreateCast(opc, v, dtype, "casttmp");
  } else if (CastInst::isBitOrNoopPointerCastable(v->getType(), dtype,
                                                  *dataLayout.get())) {
    return builder.CreateBitOrPointerCast(v, dtype, "casttmp");
  } else
    return Error::errorValue(ErrorType::UncastableTypes);
}
} // namespace Helen

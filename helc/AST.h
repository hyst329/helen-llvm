//
// Created by main on 20.09.2015.
//

#ifndef PROJECT_AST_H
#define PROJECT_AST_H

#include <llvm/Analysis/Passes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <vector>

using namespace llvm;
using namespace std;
namespace Helen {
class GenericFunction;

/**
 * Struct storing a relationship between Helen's types and LLVM underlying ones.
 * This structure helps to get LLVM type by identifier defined in Helen sources.
 */
struct TypeInfo {
  /**
   * Name of a type defined in Helen.
   * An identifier (such as `String`) or type keyword (such as `int`).
   */
  string name;
  /**
   * Pointer to LLVM underlying type.
   * .
   */
  Type *type;
  /**
   * Get the type, either primitive or Helen's composite one.
   * @return Pointer to a type (by name or if otherwise defined)
   */
  Type *getType();
};

/**
 * Basic abstract class for Helen's Abstract Syntax Tree (AST) node.
 * This class serves as a base class for all AST nodes.
 */
class AST {
public:
  /**
   * This method does the main job - namely, generating LLVM IR code.
   */
  virtual Value *codegen() = 0;
  static LLVMContext context;
  static unique_ptr<Module> module;
  static unique_ptr<DataLayout> dataLayout;
  static unique_ptr<legacy::FunctionPassManager> fpm;
  static IRBuilder<> builder;
  static map<string, AllocaInst *> variables;
  static stack<string> callstack;
  static map<string, Function *> functions;
  static map<string, Type *> types;
  static set<string> typesCStyle;
  static map<string, vector<string>> fields;
  static map<string, GenericFunction *> genericFunctions;
  static AllocaInst *createEntryBlockAlloca(Function *f, Type *t,
                                            const std::string &VarName);
  static bool isMainModule;
};

class ConstantIntAST : public AST {
  // TODO: Signedness
  uint64_t value;
  uint32_t bitwidth;

public:
  ConstantIntAST(int64_t value, int bitwidth = 64)
      : value(value), bitwidth(bitwidth) {}

  int64_t getValue() { return value; }
  virtual Value *codegen();
};

class ConstantRealAST : public AST {
  double value;

public:
  ConstantRealAST(double value) : value(value) {}
  virtual Value *codegen();
};

class ConstantCharAST : public AST {
  uint8_t value;

public:
  ConstantCharAST(char value) : value(value) {}
  virtual Value *codegen();
};

class ConstantStringAST : public AST {
  string value;

public:
  ConstantStringAST(string value) : value(value) {}
  virtual Value *codegen();
};

class VariableAST : public AST {
  string name;

public:
  VariableAST(string name) : name(name) {}

  const string &getName() const { return name; }
  virtual Value *codegen();
};

class DeclarationAST : public AST {
  TypeInfo typeInfo;
  string name;
  shared_ptr<AST> initialiser;

public:
  DeclarationAST(TypeInfo &type, string name, shared_ptr<AST> initialiser = 0)
      : typeInfo(type), name(name), initialiser(initialiser) {}

  Type *getType() { return typeInfo.type; }

  string getName() { return name; }
  virtual Value *codegen();
};

class ArrayInitialiserAST : public AST {
  TypeInfo elementTypeInfo;
  vector<shared_ptr<AST>> arguments;

public:
  ArrayInitialiserAST(TypeInfo elementTypeInfo,
                      vector<shared_ptr<AST>> arguments)
      : elementTypeInfo(elementTypeInfo), arguments(arguments) {}
  virtual Value *codegen();
};

class ConditionAST : public AST {
  shared_ptr<AST> condition;
  shared_ptr<AST> thenBranch;
  shared_ptr<AST> elseBranch;

public:
  ConditionAST(shared_ptr<AST> condition, shared_ptr<AST> thenBranch,
               shared_ptr<AST> elseBranch)
      : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
  virtual Value *codegen();
};

class LoopAST : public AST {
  shared_ptr<AST> initial;
  shared_ptr<AST> condition;
  shared_ptr<AST> iteration;
  shared_ptr<AST> body;

public:
  LoopAST(shared_ptr<AST> initial, shared_ptr<AST> condition,
          shared_ptr<AST> iteration, shared_ptr<AST> body)
      : initial(initial), condition(condition), iteration(iteration),
        body(body) {}
  virtual Value *codegen();
};

class FunctionCallAST : public AST {
  string functionName;
  vector<shared_ptr<AST>> arguments;
  string methodType = "";

public:
  FunctionCallAST(string functionName,
                  vector<shared_ptr<AST>> arguments = vector<shared_ptr<AST>>())
      : functionName(functionName), arguments(arguments) {}

  string getFunctionName() { return functionName; }

  string &getMethodType() { return methodType; }

  vector<shared_ptr<AST>> &getArguments() { return arguments; }
  virtual Value *codegen();
};

class SequenceAST : public AST {
  vector<shared_ptr<AST>> instructions;

public:
  vector<shared_ptr<AST>> &getInstructions() { return instructions; }

  SequenceAST(vector<shared_ptr<AST>> instructions = vector<shared_ptr<AST>>())
      : instructions(instructions) {}
  virtual Value *codegen();
};

class FunctionPrototypeAST : public AST {
  string name, origName;
  vector<TypeInfo> args;
  vector<string> argNames;
  TypeInfo returnType;
  string style;
  vector<string> genericParams;
  map<string, TypeInfo> genericInstTypes;
  bool vararg;

public:
  FunctionPrototypeAST(string name, vector<TypeInfo> args,
                       vector<string> argNames, TypeInfo returnType,
                       string style, vector<string> genericParams, bool vararg)
      : name(name), origName(name), args(args), argNames(argNames),
        returnType(returnType), style(style), genericParams(genericParams),
        vararg(vararg) {}

  const string &getName() const { return name; }

  const string &getOriginalName() const { return origName; }

  Type *getReturnType() {
    return genericParams.empty() ? returnType.type : returnType.getType();
  }

  string &getStyle() { return style; }

  const vector<TypeInfo> &getArgs() const { return args; }

  const map<string, TypeInfo> &getGenericInstTypes() const {
    return genericInstTypes;
  }

  void setGenericInstTypes(const map<string, TypeInfo> &val) {
    genericInstTypes = val;
  }

  vector<string> getGenericParams() const { return genericParams; }

  bool isVararg() const { return vararg; }

  Function *codegen();
};

class FunctionAST : public AST {
  shared_ptr<FunctionPrototypeAST> proto;
  shared_ptr<AST> body;
  bool shouldInstantiate;

public:
  FunctionAST(shared_ptr<FunctionPrototypeAST> proto, shared_ptr<AST> body)
      : proto(proto), body(body), shouldInstantiate(0) {}

  FunctionPrototypeAST *getPrototype() { return proto.get(); }

  void prepareForInstantiation() { shouldInstantiate = 1; }

  Function *codegen();
};

class GenericFunctionInstanceAST : public AST {
  string genObjectName;
  vector<TypeInfo> typeParams;

public:
  GenericFunctionInstanceAST(string genObjectName, vector<TypeInfo> typeParams)
      : genObjectName(genObjectName), typeParams(typeParams) {}
  virtual Value *codegen();
};

class ShiftbyAST : public AST {
  shared_ptr<AST> pointer;
  shared_ptr<AST> amount;

public:
  ShiftbyAST(shared_ptr<AST> pointer, shared_ptr<AST> amount)
      : pointer(pointer), amount(amount) {}
  virtual Value *codegen();
};

class ReturnAST : public AST {
  shared_ptr<AST> result;

public:
  ReturnAST(shared_ptr<AST> result) : result(result) {}
  virtual Value *codegen();
};

class CustomTypeAST : public AST {
  string typeName;
  string baseTypeName;
  bool isInterface;
  vector<string> baseInterfaces;
  vector<shared_ptr<AST>> instructions;
  uint64_t bstc;                         // helper field
  vector<string> overriddenMethods; // helper field
public:
  CustomTypeAST(string typeName, vector<shared_ptr<AST>> instructions,
                string baseTypeName = "", bool isInterface = 0,
                vector<string> baseInterfaces = vector<string>())
      : typeName(typeName), instructions(instructions),
        baseTypeName(baseTypeName), isInterface(isInterface),
        baseInterfaces(baseInterfaces) {}
  virtual Value *codegen();
  void compileTime();
};

class NewAST : public AST {
  TypeInfo type;
  vector<shared_ptr<AST>> arguments;

public:
  NewAST(TypeInfo &type,
         vector<shared_ptr<AST>> arguments = vector<shared_ptr<AST>>())
      : type(type), arguments(arguments) {}
  virtual Value *codegen();
};

class DeleteAST : public AST {
  string var;

public:
  DeleteAST(string var) : var(var) {}
  virtual Value *codegen();
};

class CastAST : public AST {
  shared_ptr<AST> value;
  TypeInfo destinationType;

public:
  CastAST(shared_ptr<AST> value, TypeInfo &destinationType)
      : value(value), destinationType(destinationType) {}
  virtual Value *codegen();
};

class NullAST : public AST {
public:
  virtual Value *codegen();
};
} // namespace Helen
#endif // PROJECT_AST_H

//
// Created by main on 20.09.2015.
//

#ifndef PROJECT_AST_H
#define PROJECT_AST_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <map>
#include <stack>
#include <vector>
#include <memory>
#include "Generics.h"

using namespace llvm;
using namespace std;
namespace Helen
{

class AST
{
public:
    virtual Value* codegen() = 0;
    static unique_ptr<Module> module;
    static unique_ptr<DataLayout> dataLayout;
    static unique_ptr<legacy::FunctionPassManager> fpm;
    static IRBuilder<> builder;
    static map<string, AllocaInst*> variables;
    static stack<string> callstack;
    static map<string, Function*> functions;
    static map<string, Type*> types;
    static map<string, vector<string> > fields;
    static map<string, GenericFunction> genericFunctions;
    static AllocaInst* createEntryBlockAlloca(Function* f, Type* t, const std::string& VarName);
    static bool isMainModule;
};

class ConstantIntAST : public AST
{
    int64_t value;
    int bitwidth;

public:

    ConstantIntAST(int64_t value, int bitwidth = 64)
    : value(value)
    , bitwidth(bitwidth)
    {
    }

    int64_t getValue()
    {
        return value;
    }
    virtual Value* codegen();
};

class ConstantRealAST : public AST
{
    double value;

public:

    ConstantRealAST(double value)
    : value(value)
    {
    }
    virtual Value* codegen();
};

class ConstantCharAST : public AST
{
    char value;

public:

    ConstantCharAST(char value)
    : value(value)
    {
    }
    virtual Value* codegen();
};

class ConstantStringAST : public AST
{
    string value;

public:

    ConstantStringAST(string value)
    : value(value)
    {
    }
    virtual Value* codegen();
};

class VariableAST : public AST
{
    string name;

public:

    VariableAST(string name)
    : name(name)
    {
    }

    const string& getName() const
    {
        return name;
    }
    virtual Value* codegen();
};

class DeclarationAST : public AST
{
    Type* type;
    string name;
    shared_ptr<AST> initialiser;

public:

    DeclarationAST(Type* type, string name, shared_ptr<AST> initialiser = 0)
    : type(type)
    , name(name)
    , initialiser(initialiser)
    {
    }

    Type* getType()
    {
        return type;
    }

    string getName()
    {
        return name;
    }
    virtual Value* codegen();
};

class ConditionAST : public AST
{
    shared_ptr<AST> condition;
    shared_ptr<AST> thenBranch;
    shared_ptr<AST> elseBranch;

public:

    ConditionAST(shared_ptr<AST> condition, shared_ptr<AST> thenBranch, shared_ptr<AST> elseBranch)
    : condition(condition)
    , thenBranch(thenBranch)
    , elseBranch(elseBranch)
    {
    }
    virtual Value* codegen();
};

class LoopAST : public AST
{
    shared_ptr<AST> initial;
    shared_ptr<AST> condition;
    shared_ptr<AST> iteration;
    shared_ptr<AST> body;

public:

    LoopAST(shared_ptr<AST> initial, shared_ptr<AST> condition, shared_ptr<AST> iteration, shared_ptr<AST> body)
    : initial(initial)
    , condition(condition)
    , iteration(iteration)
    , body(body)
    {
    }
    virtual Value* codegen();
};

class FunctionCallAST : public AST
{
    string functionName;
    vector<shared_ptr<AST> > arguments;
    string methodType = "";

public:

    FunctionCallAST(string functionName, vector<shared_ptr<AST> > arguments = vector<shared_ptr<AST> >())
    : functionName(functionName)
    , arguments(arguments)
    {
    }

    string getFunctionName()
    {
        return functionName;
    }

    string& getMethodType()
    {
        return methodType;
    }

    vector<shared_ptr<AST> >& getArguments()
    {
        return arguments;
    }
    virtual Value* codegen();
};

class SequenceAST : public AST
{
    vector<shared_ptr<AST> > instructions;

public:

    vector<shared_ptr<AST> >& getInstructions()
    {
        return instructions;
    }

    SequenceAST(vector<shared_ptr<AST> > instructions = vector<shared_ptr<AST> >())
    : instructions(instructions)
    {
    }
    virtual Value* codegen();
};

class FunctionPrototypeAST : public AST
{
    string name, origName;
    vector<Type*> args;
    vector<string> argNames;
    Type* returnType;
    string style;
    vector<string> genericParams;
    map<string, Type*> genericInstTypes;
public:

    FunctionPrototypeAST(string name, vector<Type*> args, vector<string> argNames, Type* returnType, string style, vector<string> genericParams)
    : name(name)
    , origName(name)
    , args(args)
    , argNames(argNames)
    , returnType(returnType)
    , style(style)
    , genericParams(genericParams)
    {
    }

    const string& getName() const
    {
        return name;
    }

    const string& getOriginalName() const
    {
        return origName;
    }

    Type* getReturnType() const
    {
        return returnType;
    }

    string& getStyle()
    {
        return style;
    }

    const vector<Type*>& getArgs() const
    {
        return args;
    }
    
    const map<string, Type*>& getGenericInstTypes() const
    {
        return genericInstTypes;
    }
    
    void setGenericInstTypes(const map<string, Type*>& val)
    {
        genericInstTypes = val;
    }
    
    Function* codegen();
};

class FunctionAST : public AST
{
    shared_ptr<FunctionPrototypeAST> proto;
    shared_ptr<AST> body;
    bool shouldInstantiate;

public:

    FunctionAST(shared_ptr<FunctionPrototypeAST> proto, shared_ptr<AST> body)
    : proto(proto)
    , body(body)
    , shouldInstantiate(0)
    {
    }
    
    FunctionPrototypeAST* getPrototype() 
    {
        return proto.get();
    }
    
    void prepareForInstantiation()
    {
        shouldInstantiate = 1;
    }
    
    Function* codegen();
};

class ShiftbyAST : public AST
{
    shared_ptr<AST> pointer;
    shared_ptr<AST> amount;

public:

    ShiftbyAST(shared_ptr<AST> pointer, shared_ptr<AST> amount)
    : pointer(pointer)
    , amount(amount)
    {
    }
    virtual Value* codegen();
};

class ReturnAST : public AST
{
    shared_ptr<AST> result;

public:

    ReturnAST(shared_ptr<AST> result)
    : result(result)
    {
    }
    virtual Value* codegen();
};

class CustomTypeAST : public AST
{
    string typeName;
    string baseTypeName;
    bool isInterface;
    vector<string> baseInterfaces;
    vector<shared_ptr<AST> > instructions;
    int bstc; // helper field
    vector<string> overriddenMethods; // helper field
public:

    CustomTypeAST(string typeName, vector<shared_ptr<AST> > instructions, string baseTypeName = "",
                  bool isInterface = 0, vector<string> baseInterfaces = vector<string>())
    : typeName(typeName)
    , instructions(instructions)
    , baseTypeName(baseTypeName)
    , isInterface(isInterface)
    , baseInterfaces(baseInterfaces)
    {
    }
    virtual Value* codegen();
    void compileTime();
};

class NewAST : public AST
{
    Type* type;
    vector<shared_ptr<AST> > arguments;

public:

    NewAST(Type* type, vector<shared_ptr<AST> > arguments = vector<shared_ptr<AST> >())
    : type(type)
    , arguments(arguments)
    {
    }
    virtual Value* codegen();
};

class DeleteAST : public AST
{
    string var;

public:

    DeleteAST(string var)
    : var(var)
    {
    }
    virtual Value* codegen();
};

class CastAST : public AST
{
    shared_ptr<AST> value;
    Type* destinationType;

public:

    CastAST(shared_ptr<AST> value, Type* destinationType)
    : value(value)
    , destinationType(destinationType)
    {
    }
    virtual Value* codegen();
};

class NullAST : public AST
{
public:
    virtual Value* codegen();
};
}
#endif // PROJECT_AST_H

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

using namespace llvm;
using namespace std;
namespace Helen
{
class AST
{
public:
    virtual Value* codegen() = 0;
    static unique_ptr<Module> module;
    static unique_ptr<legacy::FunctionPassManager> fpm;
    static IRBuilder<> builder;
    static map<string, AllocaInst*> variables;
    static stack<string> callstack;
    static map<string, Function*> functions;
    static map<string, Type*> types;
    static map<string, vector<string> > fields;
    static AllocaInst* createEntryBlockAlloca(Function* f, Type* t, const std::string& VarName);
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
    virtual Value* codegen();
};

class ConstantRealAST : public AST
{
    double value;

public:
    ConstantRealAST(double value) : value(value)
    {
    }
    virtual Value* codegen();
};

class ConstantCharAST : public AST
{
    char value;

public:
    ConstantCharAST(char value) : value(value)
    {
    }
    virtual Value* codegen();
};

class ConstantStringAST : public AST
{
    string value;

public:
    ConstantStringAST(string value) : value(value)
    {
    }
    virtual Value* codegen();
};

class VariableAST : public AST
{
    string name;

public:
    VariableAST(string name) : name(name)
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

class FunctionCallAST : public AST
{
    string functionName;
    vector<shared_ptr<AST> > arguments;

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

    SequenceAST(vector<shared_ptr<AST> > instructions = vector<shared_ptr<AST> >()) : instructions(instructions)
    {
    }
    virtual Value* codegen();
};

class FunctionPrototypeAST : public AST
{
    string name;
    vector<Type*> args;
    vector<string> argNames;
    Type* returnType;
    string style;

public:
    FunctionPrototypeAST(string name, vector<Type*> args, vector<string> argNames, Type* returnType, string style)
        : name(name)
        , args(args)
        , argNames(argNames)
        , returnType(returnType)
        , style(style)
    {
    }
    const string& getName() const
    {
        return name;
    }
    const vector<Type*>& getArgs() const
    {
        return args;
    }
    Function* codegen();
};

class FunctionAST : public AST
{
    shared_ptr<FunctionPrototypeAST> proto;
    shared_ptr<AST> body;

public:
    FunctionAST(shared_ptr<FunctionPrototypeAST> proto, shared_ptr<AST> body)
        : proto(proto)
        , body(body)
    {
    }
    Function* codegen();
};

class ReturnAST : public AST
{
    shared_ptr<AST> result;

public:
    ReturnAST(shared_ptr<AST> result) : result(result)
    {
    }
    virtual Value* codegen();
};

class CustomTypeAST : public AST
{
    string typeName;
    vector<shared_ptr<AST> > instructions;

public:
    CustomTypeAST(string typeName, vector<shared_ptr<AST> > instructions)
        : typeName(typeName)
        , instructions(instructions)
    {
    }
    virtual Value* codegen();
    void compileTime();
};

class NullAST : public AST
{
public:
    virtual Value* codegen();
};
}
#endif // PROJECT_AST_H

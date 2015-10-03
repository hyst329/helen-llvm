//
// Created by main on 20.09.2015.
//

#ifndef PROJECT_AST_H
#define PROJECT_AST_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
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

protected:
    static IRBuilder<> builder;
    static map<string, Value*> variables;
    static stack<string> callstack;
    static map<string, Function*> functions;
};

class ConstantIntAST : public AST
{
    int64_t value;

public:
    ConstantIntAST(int64_t value)
        : value(value)
    {
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
    string name;
    vector<string> args;

public:
    Function* codegen();
};

class FunctionAST : public AST
{
    shared_ptr<FunctionPrototypeAST> proto;
    shared_ptr<AST> body;

public:
    Function* codegen();
};

class NullAST : public AST
{
public:
    virtual Value* codegen();
};
}
#endif // PROJECT_AST_H

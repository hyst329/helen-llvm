//
// Created by main on 20.09.2015.
//

#ifndef PROJECT_AST_H
#define PROJECT_AST_H

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <map>
#include <stack>
#include <memory>

using namespace llvm;
using namespace std;
namespace Helen
{
class AST
{
public:
    virtual Value* codegen() = 0;

protected:
    static shared_ptr<Module>* module;
    static IRBuilder<> builder;
    static map<string, Value*> variables;
    static stack<string> callstack;
    static map<string, Function*> functions;
};

class ConstantIntAST : public AST
{
    int64_t value;

public:
    virtual Value* codegen();
};

class ConstantRealAST : public AST
{
    double value;

public:
    virtual Value* codegen();
};

class VariableAST : public AST
{
    string name;

public:
    virtual Value* codegen();
};

class FunctionCallAST : public AST
{
    string functionName;
    vector<shared_ptr<AST> > arguments;

public:
    virtual Value* codegen();
};

class SequenceAST : public AST
{
    vector<shared_ptr<AST> > instructions;

public:
    SequenceAST(vector<shared_ptr<AST> >& instructions)
    {
        this->instructions = instructions;
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
}
#endif // PROJECT_AST_H

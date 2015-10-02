//
// Created by main on 20.09.2015.
//

#ifndef PROJECT_AST_H
#define PROJECT_AST_H

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <map>
#include <memory>

using namespace llvm;
using namespace std;
namespace Helen
{
class AST
{
    virtual Value* codegen() = 0;

protected:
    static unique_ptr<Module>* module;
    static IRBuilder<> builder;
    static map<string, Value*> variables;
    static map<string, Function*> functions;
};

class ConstantIntAST : public AST
{
    int64_t value;

    virtual Value* codegen();
};

class ConstantRealAST : public AST
{
    double value;

    virtual Value* codegen();
};

class VariableAST : public AST
{
    string name;
    virtual Value* codegen();
};

class FunctionCallAST : public AST
{
    string functionName;
    vector<unique_ptr<AST> > arguments;

    virtual Value* codegen();
};

class SequenceAST : public AST
{
    vector<unique_ptr<AST> > instructions;

    virtual Value* codegen();
};

class FunctionPrototypeAST : public AST
{
    string name;
    vector<string> args;

    Function* codegen();
};

class FunctionAST : public AST
{
    unique_ptr<FunctionPrototypeAST> proto;
    unique_ptr<AST> body;

    Function* codegen();
};
}
#endif // PROJECT_AST_H

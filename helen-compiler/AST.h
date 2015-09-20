//
// Created by main on 20.09.2015.
//

#ifndef PROJECT_AST_H
#define PROJECT_AST_H

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <map>
#include <memory>

using namespace llvm;
using namespace std;

class AST {
    virtual Value* codegen() = 0;
    static unique_ptr<Module> *module;
    static IRBuilder<> builder;
    static map<std::string, Value*> variables;
};

class ConstantIntAST : public AST {
    int64_t value;
    virtual Value* codegen();
};

class ConstantRealAST : public AST {
    double value;
    virtual Value* codegen();
};

class FunctionCallAST : public AST {
    string functionName;
    std::vector<std::unique_ptr<AST>> arguments;
};

class VariableAST : public AST {
    string variableName;
    virtual Value* codegen();
};

#endif //PROJECT_AST_H

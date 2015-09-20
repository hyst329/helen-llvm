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

template<typename T> class ValueAST : public AST {
    T value;
    virtual Value* codegen();
};

class VariableAST : public AST {
    string variableName;
    virtual Value* codegen();
};

#endif //PROJECT_AST_H

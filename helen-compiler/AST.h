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

class AST {
    virtual Value* codegen() = 0;
    static unique_ptr<Module> *module;
    static IRBuilder<> builder;
    static map<std::string, Value*> variables;
    static map<std::string, Function*> functions;
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

class FunctionPrototypeAST {
    std::string Name;
    std::vector <std::string> Args;
    Function *codegen();
};

class FunctionAST {
    std::unique_ptr <FunctionPrototypeAST> proto;
    std::unique_ptr <AST> body;
    Function *codegen();
};

class VariableAST : public AST {
    string variableName;
    virtual Value* codegen();
};

#endif //PROJECT_AST_H

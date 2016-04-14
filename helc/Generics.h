#include "AST.h"
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <map>
#include <string>

using namespace llvm;
using namespace std;

namespace Helen
{

class GenericFunction
{
    FunctionAST* theAST;
    Function* instantiate(map<string, Type*> typeParams);
};
}
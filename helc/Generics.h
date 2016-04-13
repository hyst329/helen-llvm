#include "AST.h"
#include <llvm/IR/Type.h>
#include <map>
#include <string>

using namespace llvm;
using namespace std;

namespace Helen
{
    class GenericFunction
    {
        FunctionAST* theAST;
        void instantiate(map<string, Type*> typeParams);
    }
}
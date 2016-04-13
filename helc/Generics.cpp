#include "Generics.h"

using namespace llvm;
using namespace std;

namespace Helen
{
    void GenericFunction::instantiate(map<string, Type*> typeParams)
    {
        // for (auto p : typeParams) add type from p to AST::types
        // theAST->codegen();
        // for (auto p : typeParams) remove type from p from AST::types
    }
}
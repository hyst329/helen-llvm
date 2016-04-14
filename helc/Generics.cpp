#include "Generics.h"

using namespace llvm;
using namespace std;

namespace Helen
{

Function* GenericFunction::instantiate(map<string, Type*> typeParams)
{
    // Backup the old type map 
    map<string, Type*> oldtypes = AST::types;
    AST::types.insert(typeParams.begin(), typeParams.end());
    Function* f = theAST->codegen(); // TODO: Mangle the name properly
    // Restore from backup
    AST::types = oldtypes;
    return f;
}
}
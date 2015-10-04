#ifndef FUNCTIONNAMEMANGLER_H
#define FUNCTIONNAMEMANGLER_H

#include <llvm/IR/Type.h>
#include <string>

namespace Helen
{

using namespace llvm;    
using namespace std;    
    
class FunctionNameMangler
{
public:
    static string mangleName(string name, vector<Type*> args);
    static string humanReadableName(string mangledName);
};

}

#endif // FUNCTIONNAMEMANGLER_H

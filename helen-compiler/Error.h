#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <llvm/IR/Value.h>

using namespace std;
using namespace llvm;

namespace Helen
{
class AST;

enum class ErrorType {
    SyntaxError,
    UndeclaredVariable
};

class Error
{
public:
    static AST* error(ErrorType et);
    static Value* errorValue(ErrorType et);
};
}

#endif // ERROR_H

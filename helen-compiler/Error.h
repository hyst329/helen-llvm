#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <map>
#include <llvm/IR/Value.h>

using namespace std;
using namespace llvm;

namespace Helen
{
class AST;

enum class ErrorType { UnknownError, SyntaxError, UndeclaredVariable, UndeclaredFunction,
WrongArgumentType };

class Error
{
public:
    static AST* error(ErrorType et);
    static Value* errorValue(ErrorType et);
    static map<ErrorType, string> errorMessages;
};
}

#endif // ERROR_H

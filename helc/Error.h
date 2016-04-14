#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <map>
#include <vector>
#include <llvm/IR/Value.h>

using namespace std;
using namespace llvm;

namespace Helen
{
class AST;

enum class ErrorType
{
    UnknownError,
    SyntaxError,
    FileNotFound,
    UndeclaredVariable,
    UndeclaredFunction,
    WrongArgumentType,
    FunctionRedefined,
    UnexpectedOperator,
    AssignmentError,
    IndexArgumentError,
    ZeroIndexError,
    UnknownStyle,
    TypeRedefined,
    UndeclaredType,
    NonObjectType,
    UncastableTypes,
    InvalidShiftbyUse,
    InterfaceInheritedFromType,
    InvalidInstantiation
};

class Error
{
public:
    static AST* error(ErrorType et, vector<string> args = vector<string>());
    static Value* errorValue(ErrorType et, vector<string> args = vector<string>());
    static bool errorFlag;

private:
    static map<ErrorType, string> errorMessages;
};
}

#endif // ERROR_H

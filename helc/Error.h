#ifndef ERROR_H
#define ERROR_H

#include <llvm/IR/Value.h>
#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace llvm;

namespace Helen {
class AST;

enum class ErrorType {
  UnknownError,
  SyntaxError,
  FileNotFound,
  UndeclaredVariable,
  UndeclaredFunction,
  WrongArgumentType,
  WrongArgumentNumber,
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

class Error {
public:
  static AST *error(ErrorType et, vector<string> args = vector<string>());
  static Value *errorValue(ErrorType et,
                           vector<string> args = vector<string>());
  static bool errorFlag;

private:
  static map<ErrorType, string> errorMessages;
};
} // namespace Helen

#endif // ERROR_H

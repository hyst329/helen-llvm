#include "Error.h"

namespace Helen
{

std::map<ErrorType, string> Error::errorMessages = { { ErrorType::SyntaxError, "Syntax error: %1%" },
                                                     { ErrorType::UndeclaredVariable, "Undeclared variable: %1%" },
                                                     { ErrorType::UndeclaredFunction, "Undeclared function: %1%" },
                                                     { ErrorType::WrongArgumentType, "Wrong argument type: %1%" } };

AST* Error::error(ErrorType et)
{
    fprintf(stderr, "Error [%04d]: %s", et, errorMessages[et].c_str());
    return 0;
}
Value* Error::errorValue(ErrorType et)
{
    error(et);
    return 0;
}
}

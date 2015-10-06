#include "Error.h"
#include <boost/format.hpp>

namespace Helen
{

using namespace std;

map<ErrorType, string> Error::errorMessages = {
    { ErrorType::SyntaxError, "Syntax error: %1%" },
    { ErrorType::UndeclaredVariable, "Undeclared variable: %1%" },
    { ErrorType::UndeclaredFunction, "Undeclared function: %1% (mangled name: %2%)" },
    { ErrorType::WrongArgumentType, "Wrong argument type: %1%" },
    { ErrorType::FunctionRedefined, "Function %1% is redefined" },
    { ErrorType::UnexpectedOperator, "Unexpected operator: %1%" },
    { ErrorType::AssignmentError, "Assignment error: %1% is not an lvalue" },
};

AST* Error::error(ErrorType et, vector<string> args)
{
    using boost::format;
    format fmt(errorMessages[et]);
    for(string& s : args)
        fmt = fmt % s;
    fprintf(stderr, "Error [%04d]: %s\n", et, fmt.str().c_str());
    return 0;
}
Value* Error::errorValue(ErrorType et, vector<string> args)
{
    error(et, args);
    return 0;
}
}

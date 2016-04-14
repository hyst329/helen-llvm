#include "Error.h"
#include <boost/format.hpp>

namespace Helen
{

using namespace std;
bool Error::errorFlag = 0;

map<ErrorType, string> Error::errorMessages = {
    { ErrorType::SyntaxError, "Syntax error: %1%"},
    { ErrorType::FileNotFound, "Cannot find include file: %1%"},
    { ErrorType::UndeclaredVariable, "Undeclared variable: %1%"},
    { ErrorType::UndeclaredFunction, "Undeclared function: %1% (mangled name: %2%)"},
    { ErrorType::WrongArgumentType, "Wrong argument type: %1%"},
    { ErrorType::FunctionRedefined, "Function %1% is redefined"},
    { ErrorType::UnexpectedOperator, "Unexpected operator: %1%"},
    { ErrorType::AssignmentError, "Assignment error: %1% is not an lvalue"},
    { ErrorType::IndexArgumentError, "Index argument must be an array"},
    { ErrorType::ZeroIndexError, "Array index must not be zero (it's not C++, so indices do start from 1)"},
    { ErrorType::UnknownStyle, "Unknown linkage style ('C' and 'Helen' are currently allowed)"},
    { ErrorType::TypeRedefined, "Function %1% is redefined"},
    { ErrorType::UndeclaredType, "Undeclared type: %1%"},
    { ErrorType::NonObjectType, "Type of the variable is probably non-object (i.e. primitive)"},
    { ErrorType::UncastableTypes, "Value cannot be casted to type"},
    { ErrorType::InvalidShiftbyUse, "Invalid use of SHIFTBY instruction"},
    { ErrorType::InterfaceInheritedFromType, "Interface cannot inherit from regular type"},
    { ErrorType::InvalidInstantiation, "Invalid instantiation: %1% is not generic"},
};

AST* Error::error(ErrorType et, vector<string> args)
{
    using boost::format;
    format fmt(errorMessages[et]);
    for (string& s : args)
        fmt = fmt % s;
    fprintf(stderr, "Error [%04d]: %s\n", et, fmt.str().c_str());
    errorFlag = 1;
    return 0;
}

Value* Error::errorValue(ErrorType et, vector<string> args)
{
    error(et, args);
    return 0;
}
}

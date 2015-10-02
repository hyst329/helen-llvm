#include "Error.h"

namespace Helen
{
AST* Error::error(ErrorType et)
{
    //fprintf(stderr, msg);
    return 0;
}
Value* Error::errorValue(ErrorType et)
{
    error(et);
    return 0;
}
}

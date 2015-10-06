#ifndef BUILTINFUNCTIONS_H
#define BUILTINFUNCTIONS_H

#include <string>

namespace Helen
{

class BuiltinFunctions
{
public:
    static void createMainFunction();
    static void createAllBuiltins();
    const static std::string operatorMarker;
    const static std::string unaryOperatorMarker;

private:
    static void createArith();
};
}

#endif // BUILTINFUNCTIONS_H

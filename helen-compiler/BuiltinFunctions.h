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

private:
    static void createArith();
    const static std::string operatorMarker;
    const static std::string unaryOperatorMarker;
};
}

#endif // BUILTINFUNCTIONS_H

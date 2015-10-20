#ifndef BUILTINFUNCTIONS_H
#define BUILTINFUNCTIONS_H

#include <string>

namespace Helen
{

class BuiltinFunctions
{
public:
    static void createMainFunction(bool isMainModule);
    static void createAllBuiltins();
    const static std::string operatorMarker;
    const static std::string unaryOperatorMarker;

private:
    static void createArith();
    static void createLnC(); // Logical and Comparison
    static void createIO();
    static void createIndex();
};
}

#endif // BUILTINFUNCTIONS_H

#ifndef BUILTINFUNCTIONS_H
#define BUILTINFUNCTIONS_H

namespace Helen
{

class BuiltinFunctions
{
public:
    static void createMainFunction();
    static void createAllBuiltins();

private:
    static void createAdds();
    static void createSubs();
    static void createMuls();
    static void createDivs();
};
}

#endif // BUILTINFUNCTIONS_H

#include "FunctionNameMangler.h"
#include <boost/algorithm/string.hpp>
#include <llvm/IR/IRBuilder.h>

namespace Helen
{
string FunctionNameMangler::mangleName(string name, vector<Type*> args, string style)
{
    if (style == "C")
        return name;
    if (style == "Helen") {
        string mangledName = "_" + name;
        if (args.empty())
            mangledName += "_v";
        for (Type* t : args) {
            mangledName += "_";
            if (t->isVectorTy()) {
                mangledName += "v";
                t = t->getScalarType();
            }
            if (t == Type::getInt64Ty(getGlobalContext()))
                mangledName += "i";
            if (t == Type::getDoubleTy(getGlobalContext()))
                mangledName += "r";
            if (t == Type::getInt8Ty(getGlobalContext()))
                mangledName += "c";
            if (t == Type::getInt8PtrTy(getGlobalContext()))
                mangledName += "s";
        }
        return mangledName;
    }
    return ""; // error
}
string FunctionNameMangler::humanReadableName(string mangledName)
{
    if (mangledName[0] != '_')
        return mangledName;
    string name = "";
    mangledName = mangledName.substr(1);
    const int BUILTIN_UNDERSCORES = 2;
    if (mangledName[1] == '_') {
        mangledName = mangledName.substr(BUILTIN_UNDERSCORES);
        name = "<built-in> ";
    }
    vector<string> strs;
    boost::split(strs, mangledName, boost::is_any_of("_"));
    name = strs[0] + "(";
    for (int i = 1; i < strs.size(); i++) {
        if (strs[i][0] == 'v') {
            name += "array ";
            strs[i] = strs[i].substr(1);
        }
        if (strs[i] == "i")
            name += "int";
        if (strs[i] == "r")
            name += "real";
        if (strs[i] == "c")
            name += "char";
        if (strs[i] == "s")
            name += "string";
        if (strs[i] == "v")
            name += "no arguments";
        if (i < strs.size() - 1)
            name += ", ";
    }
    name += ")";
    return name;
}
}

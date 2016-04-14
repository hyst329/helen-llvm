#include "FunctionNameMangler.h"
#include <boost/algorithm/string.hpp>
#include <llvm/IR/IRBuilder.h>

namespace Helen
{

string FunctionNameMangler::mangleName(string name, vector<Type*> args, 
                                       string style, string className,
                                       vector<string> genericInstParams)
{
    if (style == "C")
        return name;
    if (style == "Helen")
    {
        string cls = !className.empty() ? className + "-" : "";
        string mangledName = "_" + cls + name;
        if (!genericInstParams.empty())
        {
            for (string gip : genericInstParams)
                mangledName += ":" + gip;
        }
        if (args.empty())
            mangledName += "_v";
        for (Type* t : args)
        {
            mangledName += "_";
            if (t->isArrayTy())
            {
                mangledName += "a";
                t = t->getArrayElementType();
            }
            if (t->isPointerTy())
            {
                PointerType* pt = (PointerType*) t;
                Type* et = pt->getElementType();
                if (et->isStructTy())
                {
                    mangledName += "type." + string(((StructType*) et)->getName());
                }
                else
                {
                    mangledName += "p";
                    t = et;
                }
            }
            if (t->isIntegerTy())
                mangledName += "i" + std::to_string(t->getPrimitiveSizeInBits());
            if (t == Type::getDoubleTy(getGlobalContext()))
                mangledName += "r";
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
    if (mangledName[1] == '_')
    {
        mangledName = mangledName.substr(BUILTIN_UNDERSCORES);
        name = "<built-in> ";
    }
    vector<string> strs;
    boost::split(strs, mangledName, boost::is_any_of("_"));
    name = strs[0] + "(";
    for (int i = 1; i < strs.size(); i++)
    {
        if (strs[i][0] == 'v')
        {
            name += "array ";
            strs[i] = strs[i].substr(1);
        }
        if (strs[i][0] == 'i')
            name += "int" + strs[i].substr(1);
        if (strs[i] == "r")
            name += "real";
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

string FunctionNameMangler::functionName(string mangledName)
{
    const string operatorMarker = "__operator_";
    if (mangledName[0] != '_')
        return mangledName;
    mangledName = mangledName.substr(1);
    // check if it's an operator
    if (boost::starts_with(mangledName, operatorMarker))
    {
        mangledName = mangledName.substr(operatorMarker.size());
        auto it = boost::find_first(mangledName, "_");
        return operatorMarker + string(mangledName.begin(), it.begin());
    }
    // it isn't an operator, so run the "normal" algorithm
    for (auto it = mangledName.begin(); it != mangledName.end(); it++)
    {
        if (*it == '_' && *(it - 1) != '_' && *(it + 1) != '_')
        {
            // here parameters begin
            return string(mangledName.begin(), it);
        }
    }
    return "";
}
}

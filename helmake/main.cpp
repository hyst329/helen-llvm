#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <string>
#include <vector>
#include <process.h>

using namespace std;
namespace fs = boost::filesystem;

map<string, vector<boost::any> > variableMap;

int yyparse();
extern FILE* yyin;

int main(int argc, char** argv)
{
    fs::path p = argc < 2 ? "./build" : argv[1];
    fs::path src = argc < 3 ? "." : argv[2];
    yyin = fopen((src / "helmkf").generic_string().c_str(), "r");
    if(!yyin) {
        fprintf(stderr, "*** No helmkf file in the specified dir. Stop. ***\n");
        return 1;
    }
    // Setting default values
    variableMap["HELEN_COMPILER"] = vector<boost::any>(1, boost::any(string("helc")));
    variableMap["HELEN_INCLUDE"] = vector<boost::any>(1, boost::any(string("")));
    variableMap["HELEN_LIB"] = vector<boost::any>(1, boost::any(string("")));
    variableMap["PROJECT"] = vector<boost::any>(1, boost::any(string("untitled")));
    variableMap["FLAGS"] = vector<boost::any>(1, boost::any(string("")));
    yyparse();
    auto sources = variableMap["SOURCES"];
    if(sources.empty()) {
        fprintf(stderr, "*** Provided helmkf file contains no SOURCES variable. Stop. ***\n");
        return 1;
    }
    fs::create_directories(p);
    FILE* makefile = fopen((p / "Makefile").generic_string().c_str(), "w");
    string helc = boost::any_cast<string>(variableMap["HELEN_COMPILER"][0]);
    helc = fs::system_complete(helc).generic_string();
    fprintf(makefile, "HELC = %s\n", helc.c_str());
    fprintf(makefile, "LLC = llc\n", helc.c_str());
    fprintf(makefile, "LD = gcc\n");
    // general Helen project structure
    string include = boost::any_cast<string>(variableMap["HELEN_INCLUDE"][0]);
    string lib = boost::any_cast<string>(variableMap["HELEN_LIB"][0]);
    if(include.empty()) {
        include = fs::canonical(fs::path(helc).parent_path() / "../include").generic_string();
    }
    if(lib.empty()) {
        lib = fs::canonical(fs::path(helc).parent_path() / "../lib").generic_string();
    }
    fprintf(makefile, "\nHINCLUDE = %s\nHLIB = %s\n", include.c_str(), lib.c_str());
    string project = boost::any_cast<string>(variableMap["PROJECT"][0]);
    string flags = boost::any_cast<string>(variableMap["FLAGS"][0]);
    fprintf(makefile, "FLAGS = %s\n", flags.c_str());
    fprintf(makefile, "all: %s\n\n%s: ", project.c_str(), project.c_str());
    for(boost::any& s : sources) {
        fs::path p = boost::any_cast<string>(s);
        string abs = fs::canonical(p).stem().generic_string();
        fprintf(makefile, "%s.bc ", abs.c_str());
    }
    fprintf(makefile, "\n");
    for(boost::any& s : sources) {
        fs::path p = boost::any_cast<string>(s);
        string abs = fs::canonical(p).stem().generic_string();
        fprintf(makefile, "\t$(LLC) %s.bc -filetype=obj\n", abs.c_str());
    }
    fprintf(makefile, "\t$(LD) ");
    for(boost::any& s : sources) {
        fs::path p = boost::any_cast<string>(s);
        string abs = fs::canonical(p).generic_string();
        fprintf(makefile, "%s.obj ", abs.c_str());
    }
    fprintf(makefile, "-o %s -L$(HLIB) -lhelstd\n\n", project.c_str());
    for(boost::any& s : sources) {
        fs::path p = boost::any_cast<string>(s);
        string abs = fs::canonical(p).generic_string();
        string stem = fs::canonical(p).stem().generic_string();
        fprintf(makefile, "%s.bc: %s\n", stem.c_str(), abs.c_str());
        fprintf(makefile, "\t$(HELC) -I=$(HINCLUDE) -o %s.bc %s\n\n", stem.c_str(), abs.c_str());
    }
    fprintf(makefile, "clean:\n\trm -f *.obj *.bc *.o *.exe %s", project.c_str());
    fclose(makefile);
    return 0;
}
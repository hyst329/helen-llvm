#include <stdio.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int yyparse (void);
extern FILE *yyin;

int main(int argc, char** argv) {
    yyin = (argc == 1) ? stdin : fopen(argv[1], "r");
    yyparse();
    return 0;
}
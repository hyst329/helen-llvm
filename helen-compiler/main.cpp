#include <stdio.h>
#include <grammar/helen.parser.hpp>

int main(int argc, char** argv) {
    yyin = (argc == 1) ? stdin : fopen(argv[1], "r");
    yyparse();
    return 0;
}
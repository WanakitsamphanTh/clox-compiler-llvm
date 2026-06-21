#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "chunk.h"
#include "vm.h"

int main(int argc, char** argv) {
    initVM();

    if(argc < 2) {
        repl();
    } else {
        interpretFile(argv[1]);
    }

    freeVM();
    return 0;
}
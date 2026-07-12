#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "chunk.h"
#include "vm.h"

int main(int argc, char** argv) {
    VM vm;
    initVM(&vm);

    if(argc < 2) {
        repl(&vm);
    } else {
        interpretFile(&vm, argv[1]);
    }

    freeVM(&vm);
    return 0;
}
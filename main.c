#include <libgen.h>
#include <stdio.h>
#include "assembler.h"

// TODO: Support literal after a register in "register indirect literal mode"
//       For example as of today we support "SET [A + 0x200]",
//       20 but not "SET [0x200 + A], 20"
// TODO: Support list of numbers and strings as data
//       As of today we just support a single string enclosed in quotes.
//       Future plan is to allow something like the regular expression below
//       DAT (<num> | <str>) (, [<num> | <str>])+

int main(int argc, char *argv[]) {
    struct assembler a[1];
    char *infile;
    /* Sanity check */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <infile>\n", basename(argv[0]));
        return -1;
    } else {
        infile = argv[1];
    }
    /* Initialize the assembler */
    if (asm_init(a, infile) < 0) {
        return -1;
    }
    /* Process the input */
    if (asm_parse(a) < 0) {
        return -1;
    }
    /* Write the parsed output to file */
    if (asm_write(a) < 0) {
        return -1;
    }
    /* Done */
    asm_free(a);
    return 0;
}

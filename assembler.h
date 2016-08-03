//
// Created by Thirumal Venkat on 02/08/16.
//

#ifndef ASSEMBLER_ASSEMBLER_H
#define ASSEMBLER_ASSEMBLER_H

#include "common.h"

int  asm_init(struct assembler *a, char *file);
void asm_free(struct assembler *a);
int  asm_parse(struct assembler *a);
int  asm_write(struct assembler *a);

#endif //ASSEMBLER_ASSEMBLER_H

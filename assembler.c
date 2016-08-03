//
// Created by Thirumal Venkat on 02/08/16.
//

#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "tokenize.h"
#include "binary_code.h"

int asm_init(struct assembler *a, char *file) {
    FILE *fp;
    ssize_t ftsize;
    uint64_t size;

    /* See if the assembler structure is valid ? */
    if (!a) {
        LOGERROR("Assembler structure is NULL");
        return -1;
    }

    /* Sanity check on file name */
    if (!file) {
        LOGERROR("Input file is NULL");
        return -1;
    }

    /* Try to open the file */
    fp = fopen(file, "r");
    if (!fp) {
        LOGERROR("Unable to open file: %s", file);
        return -1;
    }

    /* Go till the end of the file */
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        LOGERROR("fseek() to end of file failed");
        return -1;
    }

    /* Get the size of the file */
    ftsize = ftell(fp);
    if (ftsize < 0) {
        fclose(fp);
        LOGERROR("ftell() failed");
        return -1;
    }
    /* Now safe to move it to an unsigned variable */
    size = (uint64_t) ftsize;

    /* Go back to the beginning of the file */
    rewind(fp);

    /* Zero out the assembler structure */
    memset(a, 0, sizeof(*a));

    /* Allocate memory for the input */
    a->input = malloc(sizeof(char) * (size + 1));
    if (!a->input) {
        fclose(fp);
        LOGERROR("Cannot allocate memory to store input file");
        return -1;
    }

    /* Read the file into input */
    if (fread(a->input, sizeof(char), size, fp) != size) {
        free(a->input);
        fclose(fp);
        LOGERROR("Could not read the file successfully");
        return -1;
    }
    a->input[size] = '\0';
    a->inp_size = size;
    a->input_file = file;

    /* Close the file */
    fclose(fp);

    return 0;
}

void asm_free(struct assembler *a) {
    if (!a) {
        // Silently fail if no assembler structure is found
        return;
    }
    {
        // Free the token list
        struct token *t1, *t2;
        t1 = a->tok_list.head;
        while (t1 != NULL) {
            t2 = t1->next;
            free(t1);
            t1 = t2;
        }
    }
    {
        // Free the binary code nodes
        struct bcode_node *b1, *b2;
        b1 = a->bcd_list.head;
        while (b1 != NULL) {
            b2 = b1->next;
            free(b1);
            b1 = b2;
        }
    }
    // Free the input storage
    if (a->input) {
        free(a->input);
    }
}

int asm_parse(struct assembler *a) {
    // tokenize
    if (construct_tokens(a) < 0) {
        return -1;
    }
    // pass #1: Build binary code.
    //        * Assume any label that has not been resolved yet to exceed offset of 0x20
    //        * If label pointer is found at a place give it an offset.
    //        * If label operand is found then find the label pointer
    //            * If the label pointer has been resolved create opcode (short or long)
    //            * If the label pointer has not been resolved. Allocate 1 word for label offset
    //            * If the label pointer is not found, raise an error
    if (pass1(a) < 0) {
        return -1;
    }
    // pass #2: Resolve any unresolved label operands.
    if (pass2(a) < 0) {
        return -1;
    }
    return 0;
}

int asm_write(struct assembler *a) {
    // For now we just print debug output.. no hassle of printing the output
    // to a file. If ever this goes into production (highly unlikely), I'll
    // cook up a routine that writes to a file.
    return bcode_debug(a);
}
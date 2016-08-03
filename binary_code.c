//
// Created by Thirumal Venkat on 02/08/16.
//

#include <stdlib.h>
#include <string.h>
#include "binary_code.h"

static inline struct bcode_node *get_bcode() {
    return calloc(1, sizeof(struct bcode_node));
}

static inline void label_concat(char *dst, char *src, uint64_t len) {
    uint64_t i;
    while (*dst != '\0') {
        ++dst;
    }
    for (i = 0; i < len; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static inline void append_to_bcode_list(struct assembler *a, struct bcode_node *node) {
    if (a->bcd_list.tail) {
        a->bcd_list.tail->next = node;
        a->bcd_list.tail = node;
    } else {
        a->bcd_list.head = a->bcd_list.tail = node;
    }
}

static inline struct label *get_label_pointer(struct assembler *a, char *lbl_name, uint64_t lbl_len) {
    struct label *cur;
    for (cur = a->label_pts.head; cur; cur = cur->next) {
        // easy check on label lengths
        if (lbl_len != cur->lbl_len) {
            continue;
        }
        // now compare the labels themselves
        if (!strncmp(lbl_name, cur->lbl_name, lbl_len)) {
            // we have a match
            break;
        }
    }
    return cur;
}

// String representation: op b, a.
// Notice operand 'a' has no right token
// NOTE: lbl_off is in bytes, while placing it convert it into words (divide by 2)
int build_operand(struct assembler *a, struct token *t, uint16_t *opcode, uint16_t *operand) {
    // decide if the operand is operand A or operand B
    int is_a = !t->right;
    // calculate the amount of left-shift needed
    int shift = is_a ? OPERAND_A_LSHIFT : OPERAND_B_LSHIFT;
    // decide on how to build the operand depending on what it is..
    if (t->type == tt_label) {
        // we have a label
        struct label *lptr = get_label_pointer(a, t->ttu_lab.lbl_name, t->ttu_lab.lbl_len);
        if (!lptr) {
            char err_str[256];
            err_str[0] = '\0';
            strcat(err_str, "Label '");
            label_concat(err_str, t->ttu_lab.lbl_name, t->ttu_lab.lbl_len);
            strcat(err_str, "' is not associated with any label pointer");
            ASMTOKERROR(a, t, err_str);
            return -1;
        }
        if (lptr->lbl_state == ls_pt_resolved) {
            // Mark this label as resolved :-)
            t->ttu_lab.lbl_state = ls_op_resolved;
            t->ttu_lab.lbl_off = lptr->lbl_off;
            if (lptr->lbl_off <= 0x1E) {
                // NOTE: As offset can't be negative we did not
                // check for lptr->lbl_off >= -1
                // Place the label's offset in the opcode itself
                *opcode |= ((lptr->lbl_off / 2 + 0x21) << shift);
                *operand = 0;
                // we did not use the two bytes in the operand...
                return 0;
            } else {
                // place the label's offset in a separate word
                *opcode |= (OPERAND_OP_MAX << shift);
                *operand = (uint16_t) (lptr->lbl_off / 2);
                // we've used up the two bytes in the operand...
                return 2;
            }
        } else if (lptr->lbl_state == ls_pt_unresolved) {
            // place the label's offset as 0 in a separate word
            *opcode |= (OPERAND_OP_MAX << shift);
            *operand = 0;
            // Tell the label operand that it should be placing the offset here...
            t->ttu_lab.lbl_bcp = operand;
            // we've used up the two bytes in the operand...
            return 2;
        } else {
            LOGERROR("Invalid label pointer state: %d", lptr->lbl_state);
            return -1;
        }
    } else if (t->type == tt_operand) {
        // we have a proper operand..
        struct operand *opd = &t->ttu_opd;
        int lit_not_needed = 0; // is op_literal_val needed or not?
        switch (opd->opd_type) {
            case ot_literal:
                // literal, just put it there
                if (is_a && (opd->opd_literal_val >= -1 &&
                        opd->opd_literal_val <= 30)) {
                    // short hand literal notation can be used
                    *opcode |= ((opd->opd_literal_val + 0x21) << shift);
                    *operand = 0;
                    // did not used the spare operand provided..
                    return 0;
                } else {
                    // short hand literal notation cannot be used
                    *opcode |= (opd->opd_opcode_val << shift);
                    *operand = (uint16_t) opd->opd_literal_val;
                    // used the spare operand provided..
                    return 2;
                }
            case ot_reg:
            case ot_ind_reg:
                lit_not_needed = 1;
            case ot_reg_literal:
            case ot_ind_reg_literal:
            case ot_ind_literal:
                // we have some opcode
                if (lit_not_needed) {
                    // we just have an operand opcode
                    *opcode |= (opd->opd_opcode_val << shift);
                    *operand = 0;
                    // we did not use the extra operand passed
                    return 0;
                } else {
                    // we have operand opcode + a literal value
                    *opcode |= (opd->opd_opcode_val << shift);
                    *operand = (uint16_t) opd->opd_literal_val;
                    // we have used up the extra operand passed
                    return 2;
                }
            default:
                LOGERROR("Invalid operand type passed");
                return -1;
        }
    } else {
        LOGERROR("Invalid token type passed");
        return -1;
    }
}

// Just to avoid some confusion..
// opcode string in assembly looks like:  <opcode> <b>,<a>
// opcode binary is [0xOPCODE] [0xOPA] [0xOPB]
int build_opcode(struct assembler *a, struct token *t) {
    struct bcode_node *node;
    uint16_t *opcode, *opd_a, *opd_b;
    int len;
    // sanity check
    if (t->type == tt_basic_opcode) {
        if (!t->right) {
            LOGERROR("First operand is missing");
            return -1;
        } else if (!t->right->right) {
            LOGERROR("Second operand is missing");
            return -1;
        }
    } else if (t->type == tt_special_opcode) {
        if (!t->right) {
            LOGERROR("First operand is missing");
            return -1;
        }
    } else {
        // Internal error
        LOGERROR("Token type passed is not an opcode");
        return -1;
    }
    // get a bcode_node
    node = get_bcode();
    if (!node) {
        LOGERROR("No memory to allocate binary code node");
        return -1;
    }
    // initialize the node
    node->type = bt_code;
    node->next = NULL;
    opcode = &node->btu_code[0];
    opd_a  = &node->btu_code[1];
    opd_b  = &node->btu_code[2];
    // Copy the opcode
    *opcode = (uint16_t) t->ttu_opc;
    node->size = 2; /* 16-bit opcode, hence size is 2 */
    // copy the first operand
    len = build_operand(a, t->right, opcode, (t->type == tt_basic_opcode ? opd_b : opd_a));
    if (len < 0) {
        free(node);
        return -1;
    }
    node->size += len;
    // copy the second operand if present
    if (t->type == tt_basic_opcode) {
        len = build_operand(a, t->right->right, opcode, opd_a);
        if (len < 0) {
            free(node);
            return -1;
        }
        node->size += len;
    }
    // Append the binary code node to the list
    append_to_bcode_list(a, node);
    // Return the node's size in bytes
    return (int) node->size;
}

int build_data(struct assembler *a, struct token *t) {
    struct bcode_node *node;
    if (t->type != tt_data) {
        LOGERROR("Invalid token passed");
        return -1;
    }
    node = get_bcode();
    if (!node) {
        LOGERROR("Could not allocate memory for binary code node");
        return -1;
    }
    node->type = bt_data;
    node->next = NULL;
    node->size = t->tok_len * 2;
    node->btu_data = t->ttu_dat;
    // Append the binary code node to the list
    append_to_bcode_list(a, node);
    // Return the node's size in words (finally when put in code)
    return (int) node->size;
}

// pass #1: Build binary code.
//        * Assume any label that has not been resolved yet to exceed offset of 0x20
//        * If label pointer is found at a place give it an offset.
//        * If label operand is found then find the label pointer
//            * If the label pointer has been resolved create opcode (short or long)
//            * If the label pointer has not been resolved. Allocate 1 word for label offset
//            * If the label pointer is not found, raise an error
int pass1(struct assembler *a) {
    uint64_t tot_off = 0; // current offset in code
    int cur_off;
    struct token *t;
    struct label *l;
    // For the top level it can either be a label, opcode or data.
    // Operands come only after opcode.
    for (t = a->tok_list.head; t; t = t->next) {
        switch (t->type) {
            case tt_label:
                // update the label's offset from the start of file and mark as resolved.
                l = &t->ttu_lab;
                if (l->lbl_state != ls_pt_unresolved) {
                    // Internal error
                    char err_str[256];
                    err_str[0] = 0;
                    label_concat(err_str, l->lbl_name, l->lbl_len);
                    LOGERROR("PASS1: Label %s has been resolved or is not a label pointer.", err_str);
                    return -1;
                }

                l->lbl_off = tot_off;
                l->lbl_state = ls_pt_resolved;
                break;
            case tt_basic_opcode:
            case tt_special_opcode:
                // build the opcode's bcode_node and append it to its list
                cur_off = build_opcode(a, t);
                if (cur_off < 0) {
                    return -1;
                }
                tot_off += cur_off;
                break;
            case tt_data:
                // build the data's bcode_node and append it to its list
                cur_off = build_data(a, t);
                if (cur_off < 0) {
                    return -1;
                }
                tot_off += cur_off;
                break;
            case tt_operand:
                ASMTOKERROR(a, t, "PASS1: Operand at where it should not be");
                return -1;
            case tt_invalid:
                ASMTOKERROR(a, t, "PASS1: Invalid token");
                return -1;
            default:
                ASMTOKERROR(a, t, "PASS1: Unknown token");
                return -1;
        }
    }
    // Make sure all label pointers have been resolved
    for (l = a->label_pts.head; l; l = l->next) {
        if (l->lbl_state != ls_pt_resolved) {
            char err_str[256];
            err_str[0] = 0;
            strcat(err_str, "PASS1: Label pointer '");
            label_concat(err_str, l->lbl_name, l->lbl_len);
            strcat(err_str, "' is unresolved even after first pass");
            ASMTOKERROR(a, t, err_str);
            return -1;
        }
    }
    return 0;
}

// pass #2: Resolve any unresolved label operands.
int pass2(struct assembler *a) {
    struct label *cur, *ptr;
    // for every label operand present in the list
    for (cur = a->label_ops.head; cur; cur = cur->next) {
        switch (cur->lbl_state) {
            case ls_op_unresolved:
                // it is yet to be resolved, resolve it
                ptr = get_label_pointer(a, cur->lbl_name, cur->lbl_len);
                if (!ptr) {
                    // There is no corresponding label pointer for this label operand
                    char err_str[256];
                    err_str[0] = '\0';
                    strcat(err_str, "PASS2: Label '");
                    label_concat(err_str, cur->lbl_name, cur->lbl_len);
                    strcat(err_str, "' does not match any label pointer.");
                    struct token *tok = label_to_token(cur);
                    ASMTOKERROR(a, tok, err_str);
                    return -1;
                }
                // verify sanity of label pointer
                if (ptr->lbl_state != ls_pt_resolved) {
                    LOGERROR("The label pointer is yet to be resolved even after PASS1");
                    return -1;
                }
                // place the label's offset into the pointer in this label's bcode_node
                // after converting offset in bytes into words (dividing by 2)
                *cur->lbl_bcp = (uint16_t ) (ptr->lbl_off / 2);
                break;
            case ls_op_resolved:
                // already resolved, do nothing
                break;
            default:
                LOGERROR("Invalid label placed in label operand list");
                return -1;
        }
    }
    return 0;
}

int bcode_debug(struct assembler *a) {
    struct bcode_node *cur;
    int i, offset = 0;
    for (cur = a->bcd_list.head; cur; cur = cur->next) {
        // Print the current address
        printf("%04x:", offset);
        // Depending on the binary code type, print it
        switch (cur->type) {
            case bt_code:
                // Code
                for (i = 0; i < cur->size / 2; ++i) {
                    printf(" %04x", cur->btu_code[i]);
                    ++offset;
                }
                putchar('\n');
                break;
            case bt_data:
                // Data
                for (i = 0; i < cur->size / 2; ++i) {
                    // NOTE: This force converts ASCII to 16-bit data.
                    printf(" %04x", cur->btu_data[i]);
                    ++offset;
                }
                putchar('\n');
                break;
            default:
                LOGERROR("Unknown binary code type: %p", cur);
                return -1;
        }
    }
    return 0;
}

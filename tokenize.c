//
// Created by Thirumal Venkat on 02/08/16.
//

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "tokenize.h"

#define DELIM_INLINE_WS         " \t"
#define DELIM_ALL_WS            " \t\n"
#define DELIM_OPERAND_END       " \t\n,"
#define DELIM_INDIRECT_MID      " \t\n+"
#define DELIM_INDIRECT_END      " \t\n]"

static struct opcode_tokens basic_opcodes[] = {
        //  {RESRV, resrv, 0x00},
        {"SET", "set", 0x01},
        {"ADD", "add", 0x02},
        {"SUB", "sub", 0x03},
        {"MUL", "mul", 0x04},
        {"MLI", "mli", 0x05},
        {"DIV", "div", 0x06},
        {"DVI", "dvi", 0x07},
        {"MOD", "mod", 0x08},
        {"MDI", "mdi", 0x09},
        {"AND", "and", 0x0a},
        {"BOR", "bor", 0x0b},
        {"XOR", "xor", 0x0c},
        {"SHR", "shr", 0x0d},
        {"ASR", "asr", 0x0e},
        {"SHL", "shl", 0x0f},
        {"IFB", "ifb", 0x10},
        {"IFC", "ifc", 0x11},
        {"IFE", "ife", 0x12},
        {"IFN", "ifn", 0x13},
        {"IFG", "ifg", 0x14},
        {"IFA", "ifa", 0x15},
        {"IFL", "ifl", 0x16},
        {"IFU", "ifu", 0x17},
        //  {RESRV, resrv, 0x18},
        //  {RESRV, resrv, 0x19},
        {"ADX", "adx", 0x1a},
        {"SBX", "sbx", 0x1b},
        //  {RESRV, resrv, 0x1c},
        //  {RESRV, resrv, 0x1d},
        {"STI", "sti", 0x1e},
        {"STD", "std", 0x1f}
};

static struct opcode_tokens special_opcodes[] = {
        //  {RESRV, resrv, 0x00},
        {"JSR", "jsr", 0x01},
        //  {RESRV, resrv, 0x02},
        //  {RESRV, resrv, 0x03},
        //  {RESRV, resrv, 0x04},
        //  {RESRV, resrv, 0x05},
        //  {RESRV, resrv, 0x06},
        //  {RESRV, resrv, 0x07},
        {"INT", "int", 0x08},
        {"IAG", "iag", 0x09},
        {"IAS", "ias", 0x0a},
        {"RFI", "rfi", 0x0b},
        {"IAQ", "iaq", 0x0c},
        //  {RESRV, resrv, 0x0d},
        //  {RESRV, resrv, 0x0e},
        //  {RESRV, resrv, 0x0f},
        {"HWN", "hwn", 0x10},
        {"HWQ", "hwq", 0x11},
        {"HWI", "hwi", 0x12},
        //  {RESRV, resrv, 0x13},
        //  {RESRV, resrv, 0x14},
        //  {RESRV, resrv, 0x15},
        //  {RESRV, resrv, 0x16},
        //  {RESRV, resrv, 0x17},
        //  {RESRV, resrv, 0x18},
        //  {RESRV, resrv, 0x19},
        //  {RESRV, resrv, 0x1a},
        //  {RESRV, resrv, 0x1b},
        //  {RESRV, resrv, 0x1c},
        //  {RESRV, resrv, 0x1d},
        //  {RESRV, resrv, 0x1e},
        //  {RESRV, resrv, 0x1f}
};

static struct operand_tokens operands[] = {
        // Register
        {"A", "a", 0x00, ot_reg},
        {"B", "b", 0x01, ot_reg},
        {"C", "c", 0x02, ot_reg},
        {"X", "x", 0x03, ot_reg},
        {"Y", "y", 0x04, ot_reg},
        {"Z", "z", 0x05, ot_reg},
        {"I", "i", 0x06, ot_reg},
        {"J", "j", 0x07, ot_reg},
        {"PUSH", "push", 0x18, ot_reg},
        {"POP",  "pop",  0x18, ot_reg},
        {"PEEK", "peek", 0x19, ot_reg},
        {"SP", "sp", 0x1b, ot_reg},
        {"PC", "pc", 0x1c, ot_reg},
        {"EX", "ex", 0x1d, ot_reg},
        // Indirect register
        {"A", "a", 0x08, ot_ind_reg},
        {"B", "b", 0x09, ot_ind_reg},
        {"C", "c", 0x0a, ot_ind_reg},
        {"X", "x", 0x0b, ot_ind_reg},
        {"Y", "y", 0x0c, ot_ind_reg},
        {"Z", "z", 0x0d, ot_ind_reg},
        {"I", "i", 0x0e, ot_ind_reg},
        {"J", "j", 0x0f, ot_ind_reg},
        {"SP", "sp", 0x19, ot_ind_reg},
        {"--SP", "--sp", 0x18, ot_ind_reg},
        {"SP++", "sp++", 0x18, ot_ind_reg},
        // Indirect register + literal
        {"A", "a", 0x10, ot_ind_reg_literal},
        {"B", "b", 0x11, ot_ind_reg_literal},
        {"C", "c", 0x12, ot_ind_reg_literal},
        {"X", "x", 0x13, ot_ind_reg_literal},
        {"Y", "y", 0x14, ot_ind_reg_literal},
        {"Z", "z", 0x15, ot_ind_reg_literal},
        {"I", "i", 0x16, ot_ind_reg_literal},
        {"J", "j", 0x17, ot_ind_reg_literal},
        {"SP", "sp", 0x1a, ot_ind_reg_literal},
        {"PICK", "pick", 0x1a, ot_reg_literal},
        // Indirect literal
        {"", "", 0x1e, ot_ind_literal},
        // Literal
        {"", "", 0x1f, ot_literal}
};

// fetches the address of the token enclosing the label
// Should only be used on labels which are part of a token.
inline struct token *label_to_token(struct label *l) {
    static struct token t;
    const long diff = (uint8_t *) &t - (uint8_t *) &t.ttu_lab;
    return (struct token *) ((uint8_t *) l + diff);
}

static inline void save_global_pos_tok(struct assembler *a, struct token *t) {
    t->tok_pos = a->inp_offset;
    t->tok_row = a->inp_row;
    t->tok_col = a->inp_col;
}

static inline void restore_global_pos_tok(struct assembler *a, struct token *t) {
    a->inp_offset = t->tok_pos;
    a->inp_row = t->tok_row;
    a->inp_col = t->tok_col;
}

static inline char *cur_ptr(struct assembler *a) {
    return a->input + a->inp_offset;
}

static inline char cur_char(struct assembler *a) {
    return a->input[a->inp_offset];
}

static inline void inc_char(struct assembler *a) {
    if (a->inp_offset < a->inp_size) {
        if (cur_char(a) == '\n') {
            a->inp_row++;
            a->inp_col = 0;
        } else {
            a->inp_col++;
        }
        a->inp_offset++;
    }
}

static inline int is_end_of_file(struct assembler *a) {
    return a->inp_offset >= a->inp_size;
}

// -1 if strings don't match
// 0 if string ends with null character
// 1 if string ends with one of the characters provided in the delimiter
static int strcmp_token(struct assembler *a,
                        const char *s,
                        const char delims[]) {
    char *p = cur_ptr(a);
    size_t n = strlen(s);
    if (strncmp(s, p, n)) {
        return -1;
    }
    // partial strings match.. let's verify if this is a word
    p += n;
    if (!*p) {
        // perfect match
        return 0;
    }
    while (*delims) {
        if (*p == *delims) {
            // we have a match with one of the suffixes
            return 1;
        }
        ++delims;
    }
    return -1;
}

// go past the current token, we keep going forward till we meet one of
// the delimiters.
static inline uint64_t go_past_cur_token_delim(struct assembler *a, char delims[]) {
    uint64_t counter = 0;
    char c, *s;
    while ((c = cur_char(a)) != '\0') {
        s = delims;
        while (*s) {
            if (c == *s) {
                break;
            }
            ++s;
        }
        if (*s) {
            break;
        }
        inc_char(a);
        ++counter;
    }
    return counter;
}

// Common pattern of going past a token..
static inline uint64_t go_past_cur_token(struct assembler *a) {
    return go_past_cur_token_delim(a, DELIM_ALL_WS);
}

// keep moving forward if you encounter one of the characters in "chars"
static inline int skip_chars(struct assembler *a, char chars[]) {
    int counter = 0, prev_counter;
    char c, *s;
    while ((c = cur_char(a)) != '\0') {
        prev_counter = counter;
        s = chars;
        while (*s) {
            if (c == *s) {
                inc_char(a);
                ++counter;
                break;
            }
            ++s;
        }
        if (prev_counter == counter) {
            // none of the expected characters were encountered
            break;
        }
    }
    return counter;
}

// skip all whitespaces
static inline int skip_all_whitespaces(struct assembler *a) {
    return skip_chars(a, DELIM_ALL_WS);
}

// skip only inline whitespaces (no newline)
static inline int skip_inline_whitespaces(struct assembler *a) {
    return skip_chars(a, DELIM_INLINE_WS);
}

// skip a comment - we move past the newline
static inline int skip_comment(struct assembler *a) {
    int counter = 0;
    // If not a comment return here..
    if (cur_char(a) != ';') {
        return counter;
    }
    // Ignore till the newline
    while (cur_char(a) != '\0' && cur_char(a) != '\n') {
        inc_char(a);
        ++counter;
    }
    // if a newline.. move past it
    if (cur_char(a) == '\n') {
        inc_char(a);
        ++counter;
    }
    return counter;
}

// skip contiguous segments of 0 or 1 whitespace/comment blocks
static inline void skip_whitespaces_and_comments(struct assembler *a) {
    int ws_enc = 0, cm_enc = 0;
    // Till you keep encountering whitespaces or comments
    do {
        // If end of file.. break out of loop
        if (cur_char(a) == '\0') {
            break;
        }
        ws_enc = skip_all_whitespaces(a);
        // If end of file.. break out of loop
        if (cur_char(a) == '\0') {
            break;
        }
        cm_enc = skip_comment(a);
    } while (ws_enc > 0 || cm_enc > 0);
}

// get a label if found. Returns 1 on success, 0 on not found
static inline int getif_label(struct assembler *a, struct token *tok, int has_prefix) {
    struct label *l;
    char *s;
    int i;
    if (has_prefix && cur_char(a) != ':') {
        // If not a label return here itself
        return 0;
    }
    // save the start of token
    save_global_pos_tok(a, tok);
    // move past ':', as it is not needed
    if (has_prefix) {
        inc_char(a);
    }
    // labels should not begin with
    // Finally fill up the token structure and go past the token
    tok->type = tt_label;
    l = &tok->ttu_lab;
    l->lbl_name = cur_ptr(a);
    l->lbl_len = tok->tok_len = go_past_cur_token(a);
    // if prefix then we're a pointer, if no prefix then we're an operand.
    // mark the state as unresolved for now.
    l->lbl_state = has_prefix ? ls_pt_unresolved : ls_op_unresolved;
    l->lbl_off = 0; /* Some default value */
    l->next = NULL;
    // Verify that the label is valid...
    if (!l->lbl_len) {
        // zero length label
        restore_global_pos_tok(a, tok);
        ASMTOKERROR(a, tok, "Label is of zero length");
        return -1;
    }
    s = l->lbl_name;
    // check label start
    if (!isalpha(s[0])) {
        // not starting with an alphabet
        restore_global_pos_tok(a, tok);
        ASMTOKERROR(a, tok, "Label does not start with alphabet");
        return -1;
    }
    // check rest of the label
    for (i = 1; i < l->lbl_len; ++i) {
        if (!isalnum(s[i])) {
            // has characters that are not alphabet or number
            restore_global_pos_tok(a, tok);
            ASMTOKERROR(a, tok, "Label contains non-alphanumeric characters");
            return -1;
        }
    }
    // Valid label
    return 1;
}

// get a string if found
// 1 on success, 0 on not found, -1 on error
static inline int getif_string(struct assembler *a, struct token *t) {
    uint64_t counter = 0;
    char c, *s , *start;

    // check for valid string start delimiter
    if (cur_char(a) != '"') {
        return 0;
    }
    save_global_pos_tok(a, t);
    inc_char(a);
    start = cur_ptr(a);
    // Now find a closing quote
    while ((c = cur_char(a)) != '\0') {
        s = cur_ptr(a); --s;
        if (c == '"' && *s != '\\') {
            break;
        }
        inc_char(a);
        ++counter;
    }

    // Check for end of file loop exit
    if (!cur_char(a)) {
        ASMTOKERROR(a, t, "Unexpected end of file after string start");
        return -1;
    } else {
        // Go past closing " quote
        inc_char(a);
    }

    // Finally fill up the token structure
    t->type = tt_data;
    t->ttu_dat = start;
    t->tok_len = counter;
    return 1;
}

// get data if found
// 1 on success, 0 if not data, -1 on error
static inline int getif_data(struct assembler *a, struct token *t) {
    if (strcmp_token(a, "DAT", DELIM_INLINE_WS) > 0 ||
        strcmp_token(a, "dat", DELIM_INLINE_WS) > 0) {
        // go past the DAT token, skip whitespaces
        go_past_cur_token_delim(a, DELIM_INLINE_WS);
        skip_inline_whitespaces(a);
        // sanity check to see if we've not gone past EOF or newline
        if (!cur_char(a)) {
            ASMTOKERROR(a, t, "Unexpected end of file while searching for data");
            return -1;
        } else if (cur_char(a) == '\n') {
            ASMTOKERROR(a, t, "Unexpected newline while searching for data");
            return -1;
        }
        // See if it is a string?
        return  getif_string(a, t);
    } else {
        // No DAT token, so not data...
        return 0;
    }
}

// extract a number from the current position if found
// 0 if not a number, 1 on success, < 0 on error
static inline int getif_number(struct assembler *a, long *num) {
    long number;
    char *s;
    if (cur_char(a) < '0' || cur_char(a) > '9') {
        /* Not a number */
        return 0;
    }
    // we have a number
    s = cur_ptr(a);
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        number = strtol(s, NULL, 16);
        if (number == LONG_MAX && errno != 0) {
            ASMERROR(a, "Invalid hexadecimal number");
            return -1;
        }
    } else if (s[0] == '0' && isdigit(s[1])) {
        number = strtol(s, NULL, 8);
        if (number == LONG_MAX && errno != 0) {
            ASMERROR(a, "Invalid octal number");
            return -1;
        }
    } else {
        number = strtol(s, NULL, 10);
        if (number == LONG_MAX && errno != 0) {
            ASMERROR(a, "Invalid decimal number");
            return -1;
        }
    }
    // We have a number, and we know what it is..
    *num = number;
    return 1;
}

// read register operands if possible
static inline int getif_reg(struct assembler *a,
                            struct operand_tokens *ot,
                            struct token *t) {
    // If register, then fill up the token structure
    if (strcmp_token(a, ot->caps, DELIM_OPERAND_END) >= 0 ||
        strcmp_token(a, ot->nocaps, DELIM_OPERAND_END) >= 0) {
        save_global_pos_tok(a, t);
        t->type = tt_operand;
        t->ttu_opd.opd_type = ot->type;
        t->ttu_opd.opd_opcode_val = ot->value;
        t->ttu_opd.opd_literal_val = 0;
        t->tok_len = go_past_cur_token_delim(a, DELIM_OPERAND_END);
        return 1;
    }
    return 0;
}

static inline int getif_ind_reg(struct assembler *a,
                                struct operand_tokens *ot,
                                struct token *t) {
    char *start, *end;

    // check for indirection start parenthesis
    if (cur_char(a) != '[') {
        return 0;
    }
    start = cur_ptr(a);
    save_global_pos_tok(a, t);
    inc_char(a);

    // see if we match the register...
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    }
    if (strcmp_token(a, ot->caps, DELIM_INDIRECT_END) < 0 &&
        strcmp_token(a, ot->nocaps, DELIM_INDIRECT_END) < 0) {
        restore_global_pos_tok(a, t);
        return 0;
    }
    go_past_cur_token_delim(a, DELIM_INDIRECT_END);

    // see if we match an indirection end parenthesis
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    } else if (cur_char(a) != ']') {
        ASMERROR(a, "Unexpected character while searching for ']'");
    }
    inc_char(a);

    end = cur_ptr(a);

    // ok we encountered a indirect register, fill up the token
    t->type = tt_operand;
    t->ttu_opd.opd_type = ot->type;
    t->ttu_opd.opd_opcode_val = ot->value;
    t->ttu_opd.opd_literal_val = 0;
    t->tok_len = end - start;
    // done
    return 1;
}

static inline int getif_ind_reg_literal(struct assembler *a,
                                        struct operand_tokens *ot,
                                        struct token *t) {
    char *start, *end;
    int rc;
    long num;

    // check for indirection start parenthesis
    if (cur_char(a) != '[') {
        return 0;
    }
    start = cur_ptr(a);
    save_global_pos_tok(a, t);
    inc_char(a);

    // check to see if we match a register
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    }
    if (strcmp_token(a, ot->caps, DELIM_INDIRECT_MID) < 0 &&
        strcmp_token(a, ot->nocaps, DELIM_INDIRECT_MID) < 0) {
        restore_global_pos_tok(a, t);
        return 0;
    }
    go_past_cur_token_delim(a, DELIM_INDIRECT_MID);

    // skip inline whitespaces if any...
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for '+'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for '+'");
        return -1;
    } else if (cur_char(a) != '+') {
        ASMERROR(a, "Unexpected character while searching for '+'");
    }
    inc_char(a);

    // see if we can get a number..
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    }
    rc = getif_number(a, &num);
    if (rc <= 0) {
        // not a number..
        restore_global_pos_tok(a, t);
        return rc;
    }
    go_past_cur_token_delim(a, DELIM_INDIRECT_END);

    // find indirection close paren...
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    } else if (cur_char(a) != ']') {
        ASMERROR(a, "Unexpected character while searching for ']'");
    }
    inc_char(a);
    end = cur_ptr(a);

    // ok we encountered a indirect register, fill up the token
    t->type = tt_operand;
    t->ttu_opd.opd_type = ot->type;
    t->ttu_opd.opd_opcode_val = ot->value;
    t->ttu_opd.opd_literal_val = num;
    t->tok_len = end - start;
    // done
    return 1;
}

static inline int getif_reg_literal(struct assembler *a,
                                    struct operand_tokens *ot,
                                    struct token *t) {
    char *start, *end;
    int rc;
    long num;

    start = cur_ptr(a);
    save_global_pos_tok(a, t);

    // check to see if we match a register
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    }
    if (strcmp_token(a, ot->caps, DELIM_INLINE_WS) < 0 &&
        strcmp_token(a, ot->nocaps, DELIM_INLINE_WS) < 0) {
        restore_global_pos_tok(a, t);
        return 0;
    }
    go_past_cur_token(a);

    // see if we can get a number..
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    }
    rc = getif_number(a, &num);
    if (rc <= 0) {
        // not a number..
        restore_global_pos_tok(a, t);
        return rc;
    }
    go_past_cur_token_delim(a, DELIM_OPERAND_END);

    end = cur_ptr(a);

    // ok we encountered a indirect register, fill up the token
    t->type = tt_operand;
    t->ttu_opd.opd_type = ot->type;
    t->ttu_opd.opd_opcode_val = ot->value;
    t->ttu_opd.opd_literal_val = num;
    t->tok_len = end - start;
    // done
    return 1;
}

static inline int getif_ind_literal(struct assembler *a,
                                    struct operand_tokens *ot,
                                    struct token *t) {
    long num;
    int rc;
    char *start, *end;

    // check to see if we get an indirection open paren
    if (cur_char(a) != '[') {
        return 0;
    }
    start = cur_ptr(a);
    save_global_pos_tok(a, t);
    inc_char(a);

    // see if we can get a number..
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    }
    rc = getif_number(a, &num);
    if (rc <= 0) {
        // not a number..
        restore_global_pos_tok(a, t);
        return rc;
    }
    go_past_cur_token_delim(a, DELIM_INDIRECT_END);

    // skip whitespaces.. check for correct closing of parenthesis
    skip_inline_whitespaces(a);
    if (!cur_char(a)) {
        ASMERROR(a, "Unexpected end of file while searching for ']'");
        return -1;
    } else if (cur_char(a) == '\n') {
        ASMERROR(a, "Unexpected new line while searching for ']'");
        return -1;
    } else if (cur_char(a) != ']') {
        ASMERROR(a, "Unexpected character while searching for ']'");
    }
    inc_char(a);
    end = cur_ptr(a);

    // ok we encountered a indirect literal, fill up the token
    t->type = tt_operand;
    t->ttu_opd.opd_type = ot->type;
    t->ttu_opd.opd_opcode_val = ot->value;
    t->ttu_opd.opd_literal_val = num;
    t->tok_len = end - start;
    // done
    return 1;
}

static inline int getif_literal(struct assembler *a,
                                struct operand_tokens *ot,
                                struct token *t) {
    long num;
    int rc = getif_number(a, &num);
    if (rc <= 0) {
        return rc;
    } else {
        save_global_pos_tok(a, t);
        t->type = tt_operand;
        t->ttu_opd.opd_type = ot->type;
        t->ttu_opd.opd_opcode_val = ot->value;
        t->ttu_opd.opd_literal_val = num;
        t->tok_len = go_past_cur_token_delim(a, DELIM_OPERAND_END);
        return 1;
    }
}


// 0 on success, -1 on failure
static inline int append_to_unresolved_labels(struct assembler *a, struct token *tok) {
    struct label *l = &tok->ttu_lab, *cur;
    int found;
    switch (l->lbl_state) {
        case ls_op_unresolved:
        case ls_op_resolved:
            if (a->label_ops.tail) {
                a->label_ops.tail->next = l;
                a->label_ops.tail = l;
            } else {
                a->label_ops.head = a->label_ops.tail = l;
            }
            break;
        case ls_pt_unresolved:
        case ls_pt_resolved:
            // First check for duplicate labels
            found = 0;
            for (cur = a->label_pts.head; cur; cur = cur->next) {
                // easy check on label lengths
                if (l->lbl_len != cur->lbl_len) {
                    continue;
                }
                // now compare the labels themselves
                if (!strncmp(l->lbl_name, cur->lbl_name, l->lbl_len)) {
                    // we have a duplicate
                    found = 1;
                    break;
                }
            }
            // error out, if we have found a duplicate label
            if (found) {
                // LOL, reporting this error is a bit bothersome, but has to be done..
                char err_str[256];
                struct token *t2 = label_to_token(cur);
                snprintf(err_str, 256, "Found duplicate label at (%llu:%llu)", t2->tok_row, t2->tok_col);
                ASMTOKERROR(a, tok, err_str);
                return -1;
            }
            // No duplicates, append this
            if (a->label_pts.tail) {
                a->label_pts.tail->next = l;
                a->label_pts.tail = l;
            } else {
                a->label_pts.head = a->label_pts.tail = l;
            }
            break;
        default:
            ASMTOKERROR(a, tok, "Invalid label state");
            return -1;
    }
    return 0;
}

// 1 on success, 0 if not operand, -1 on error
static inline int getif_operand(struct assembler *a, struct token *t) {
    int i, rc = 0;
    int n = sizeof(operands) / sizeof(struct operand_tokens);
    // sweep the operand types from backwards..
    // this prevents us from not recognizing the entire
    // operand.. as the table is ordered from specific to generic.
    for (i = n - 1; i >= 0; --i) {
        switch (operands[i].type) {
            case ot_reg:
                rc = getif_reg(a, operands + i, t);
                if (rc != 0) {
                    return rc;
                }
                break;
            case ot_ind_reg:
                rc = getif_ind_reg(a, operands + i, t);
                if (rc != 0) {
                    return rc;
                }
                break;
            case ot_ind_reg_literal:
                rc = getif_ind_reg_literal(a, operands + i, t);
                if (rc != 0) {
                    return rc;
                }
                break;
            case ot_reg_literal:
                rc = getif_reg_literal(a, operands + i, t);
                if (rc != 0) {
                    return rc;
                }
                break;
            case ot_ind_literal:
                rc = getif_ind_literal(a, operands + i, t);
                if (rc != 0) {
                    return rc;
                }
                break;
            case ot_literal:
                // check if a number
                rc = getif_literal(a, operands + i, t);
                if (rc != 0) {
                    return rc;
                }
                break;
            default:
                LOGERROR("Invalid operand type specified");
                return -1;
        }
    }
    // none of the operands, check if it is a label and return true if so..
    rc = getif_label(a, t, 0);
    if (rc < 0) {
        return rc;
    } else if (rc > 0) {
        // if able to append correctly return 1 as everything succeeded
        // if not return -1 to halt assembling the code.
        return !append_to_unresolved_labels(a, t) ? 1 : -1;
    }
    // not an operand
    return 0;
}

// 1 on success, 0 if not opcode, -1 on failure
static inline int getif_basic_opcode(struct assembler *a, struct token *t) {
    int i, found = 0;
    int n = sizeof(basic_opcodes) / sizeof(struct opcode_tokens);
    for (i = 0; i < n; ++i) {
        if (strcmp_token(a, basic_opcodes[i].opct_caps,
                         DELIM_INLINE_WS) > 0 ||
            strcmp_token(a, basic_opcodes[i].opct_nocaps,
                         DELIM_INLINE_WS) > 0) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return 0;
    }
    /* We have a basic opcode */
    save_global_pos_tok(a, t);
    t->type = tt_basic_opcode;
    t->ttu_opc = basic_opcodes[i].opct_value;
    t->tok_len = go_past_cur_token(a);
    return 1;
}

// 1 on success, 0 if not opcode, -1 on failure
static inline int getif_special_opcode(struct assembler *a, struct token *t) {
    int i, found = 0;
    int n = sizeof(special_opcodes) / sizeof(struct opcode_tokens);
    for (i = 0; i < n; ++i) {
        if (strcmp_token(a, special_opcodes[i].opct_caps,
                         DELIM_INLINE_WS) > 0 ||
            strcmp_token(a, special_opcodes[i].opct_nocaps,
                         DELIM_INLINE_WS) > 0) {
            found = 1;
            break;
        }
    }
    if (!found) {
        return 0;
    }
    /* We have a special opcode */
    save_global_pos_tok(a, t);
    t->type = tt_special_opcode;
    t->ttu_opc = special_opcodes[i].opct_value;
    t->tok_len = go_past_cur_token(a);
    return 1;
}

static inline struct token *get_token() {
    return calloc(1, sizeof(struct token));
}

static inline void append_to_token_list(struct assembler *a, struct token *t) {
    if (a->tok_list.tail) {
        a->tok_list.tail->next = t;
        a->tok_list.tail = t;
    } else {
        a->tok_list.head = a->tok_list.tail = t;
    }
}

int construct_tokens(struct assembler *a) {
    int rc;
    struct token *t, *t1, *t2;
    /* Start parsing */
    while (!is_end_of_file(a)) {
        /* Skip all whitespaces and comments */
        skip_whitespaces_and_comments(a);
        if (is_end_of_file(a)) {
            break;
        }

        t = get_token();
        if (!t) {
            LOGERROR("No more memory to allocate basic token");
            return -1;
        }
        // search for labels with ':' prefixed only...
        if ((rc = getif_label(a, t, 1)) != 0) {
            if (rc < 0) {
                free(t);
                return rc;
            }
            // valid label with : prefix.. add it to global unresolved label pointers
            append_to_unresolved_labels(a, t);
        } else if ((rc = getif_data(a, t)) != 0) {
            if (rc < 0) {
                free(t);
                return rc;
            }
        } else if ((rc = getif_basic_opcode(a, t)) != 0) {
            if (rc < 0) {
                free(t);
                return rc;
            }

            // skip inline whitespace, check for end of file/newline
            skip_inline_whitespaces(a);
            if (!cur_char(a)) {
                ASMERROR(a, "Unexpected end of file while searching for ']'");
                free(t);
                return -1;
            } else if (cur_char(a) == '\n') {
                ASMERROR(a, "Unexpected new line while searching for ']'");
                free(t);
                return -1;
            }

            // First operand
            t1 = get_token();
            if (!t1) {
                LOGERROR("No more memory to allocate op1 token");
                free(t);
                return -1;
            }
            rc = getif_operand(a, t1);
            if (rc < 0) {
                free(t1);
                free(t);
                return -1;
            } else if (rc == 0) {
                ASMERROR(a, "No operand/label found after basic opcode");
                free(t1);
                free(t);
                return -1;
            }

            // skip inline whitespace, check for end of file/newline
            skip_inline_whitespaces(a);
            if (!cur_char(a)) {
                ASMERROR(a, "Unexpected end of file while searching for ']'");
                free(t1);
                free(t);
                return -1;
            } else if (cur_char(a) == '\n') {
                ASMERROR(a, "Unexpected new line while searching for ']'");
                free(t1);
                free(t);
                return -1;
            }

            // check for comma..
            if (cur_char(a) != ',') {
                ASMERROR(a, "No comma after first operand");
                free(t1);
                free(t);
                return -1;
            }
            inc_char(a);

            // skip inline whitespace, check for end of file/newline
            skip_inline_whitespaces(a);
            if (!cur_char(a)) {
                ASMERROR(a, "Unexpected end of file while searching for ']'");
                free(t1);
                free(t);
                return -1;
            } else if (cur_char(a) == '\n') {
                ASMERROR(a, "Unexpected new line while searching for ']'");
                free(t1);
                free(t);
                return -1;
            }

            // Second operand
            t2 = get_token();
            if (!t2) {
                LOGERROR("No more memory to allocate op2 token");
                free(t1);
                free(t);
                return -1;
            }
            rc = getif_operand(a, t2);
            if (rc < 0) {
                free(t2);
                free(t1);
                free(t);
                return rc;
            } else if (rc == 0) {
                ASMERROR(a, "No second operand/label found for basic opcode");
                free(t2);
                free(t1);
                free(t);
                return -1;
            }

            // Make a group out of these 3 tokens
            t->right = t1;
            t1->right = t2;
        } else if ((rc = getif_special_opcode(a, t)) != 0) {
            if (rc < 0) {
                free(t);
                return rc;
            }

            // skip inline whitespace, check for end of file/newline
            skip_inline_whitespaces(a);
            if (!cur_char(a)) {
                ASMERROR(a, "Unexpected end of file while searching for ']'");
                free(t);
                return -1;
            } else if (cur_char(a) == '\n') {
                ASMERROR(a, "Unexpected new line while searching for ']'");
                free(t);
                return -1;
            }

            // First operand
            t1 = get_token();
            if (!t1) {
                LOGERROR("No more memory to allocate op1 token");
                free(t);
                return -1;
            }
            rc = getif_operand(a, t1);
            if (rc < 0) {
                free(t1);
                free(t);
                return -1;
            } else if (rc == 0) {
                ASMERROR(a, "No operand found after basic opcode");
                free(t1);
                free(t);
                return -1;
            }
            // Link the operand to main token and form a group
            t->right = t1;
        } else {
            ASMERROR(a, "Unknown token");
            free(t);
            return -1;
        }
        append_to_token_list(a, t);
    }
    return 0;
}
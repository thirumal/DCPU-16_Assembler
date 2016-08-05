//
// Created by Thirumal Venkat on 02/08/16.
//

#ifndef ASSEMBLER_COMMON_H
#define ASSEMBLER_COMMON_H

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdint.h>

/* Error logging macro */
#define LOGERROR(fmt, ...) \
    do { \
        fprintf(stderr, "[%s:%s:%d,%d] "fmt"\n", \
                basename(__FILE__), __func__, __LINE__, \
                errno, ##__VA_ARGS__); \
    } while (0)

#define ASMERROR(a, message) \
    do { \
        fprintf(stderr, "%s:%llu:%llu %s\n", \
                basename(a->input_file), a->inp_row + 1, \
                a->inp_col + 1, message); \
    } while (0)

#define ASMTOKERROR(a, t, message) \
    do { \
        fprintf(stderr, "%s:%llu:%llu %s\n", \
            basename(a->input_file), t->tok_row + 1, \
            t->tok_col + 1, message); \
    } while (0)

enum operand_type {
    ot_invalid,
    ot_reg,             // A, B, C, ...
    ot_ind_reg,         // [A], [B], ...
    ot_ind_reg_literal, // [A + 20], ...
    ot_reg_literal,     // only PICK 10
    ot_ind_literal,     // [20], ...
    ot_literal,         // 0x12, 017, 23
};

// Type of token
enum token_type {
    tt_invalid,
    tt_label,
    tt_basic_opcode,
    tt_special_opcode,
    tt_operand,
    tt_data,
};

/* Binary code type enumeration */
enum bincode_type {
    bt_invalid,
    bt_code,       /* Opcode type */
    bt_data        /* Data type */
};

/* Label state */
enum label_state {
    ls_invalid,
    ls_op_unresolved,  // Unresolved label operand
    ls_op_resolved,    // Label operand has been resolved
    ls_pt_unresolved,  // Label's offset has not been populated
    ls_pt_resolved     // Label's offset has been populated
};

struct opcode_tokens {
    char *opct_caps;
    char *opct_nocaps;
    int  opct_value;
};

struct operand_tokens {
    char *caps;
    char *nocaps;
    int  value;
    enum operand_type type;
};

struct operand {
    enum  operand_type opd_type;
    int   opd_opcode_val;
    long  opd_literal_val;
};

struct label {
    char     *lbl_name;
    uint64_t  lbl_len;
    enum label_state lbl_state;
    union {
        uint64_t  lbl_pt_off; /* Offset of the label pointer from start of file */
        uint16_t *lbl_op_bcp; /* Pointer inside a bcode_node's data */
    } u;
    /* We'll need this as we put labels in a linked list */
    struct label *next;
};
#define lbl_off u.lbl_pt_off
#define lbl_bcp u.lbl_op_bcp

struct label_list {
    struct label *head;
    struct label *tail;
};

struct token {
    uint64_t tok_pos;
    uint64_t tok_row;
    uint64_t tok_col;
    uint64_t tok_len;
    enum token_type type;
    union {
        char *data;
        int opcode;
        struct label label;
        struct operand operand;
    } u;
    struct token *right;
    struct token *next;
};
#define ttu_lab u.label
#define ttu_opc u.opcode
#define ttu_opd u.operand
#define ttu_dat u.data

struct token *label_to_token(struct label *l);

struct token_list {
    struct token *head;
    struct token *tail;
};

/* Binary code list structure */
struct bcode_node {
    /* Binary code type */
    enum bincode_type type;
    /* Size of the data */
    uint64_t size;
    /* Binary code */
    union {
        struct {
            uint8_t has_a;     /* Are we using register A */
            uint8_t has_b;     /* Are we using register B */
            uint16_t bcode[3]; /* Upto 3 words of binary data */
        } s;
        char     *bdata;    /* Pointer to binary data */
    } u;
    /* Next binary list code entry */
    struct bcode_node *next;
};
#define btu_c_has_a u.s.has_a
#define btu_c_has_b u.s.has_b
#define btu_code u.s.bcode
#define btu_data u.bdata

/* Binary code structure */
struct bcode_list {
    struct bcode_node *head;
    struct bcode_node *tail;
};

/* The main assembler structure */
struct assembler {
    uint64_t inp_row;
    uint64_t inp_col;
    uint64_t inp_offset;
    uint64_t inp_size;
    char *input;
    char *input_file;
    struct label_list label_pts;   // labels whose offsets have been determined
    struct label_list label_ops;   // unresolved labels which are operands
    struct token_list tok_list;
    struct bcode_list bcd_list;
};

#define OPERAND_A_LSHIFT 0xA
#define OPERAND_B_LSHIFT 0x5
#define OPERAND_OP_MAX   0x1F

#endif //ASSEMBLER_COMMON_H

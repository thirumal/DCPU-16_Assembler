# Opcodes
OPCODES = { 'SET': 0x1,
            'ADD': 0x2, 'SUB': 0x3,
            'MUL': 0x4, 'DIV': 0x5, 'MOD': 0x6,
            'SHL': 0x7, 'SHR': 0x8,
            'AND': 0x9, 'BOR': 0xA, 'XOR': 0xB,
            'IFE': 0xC, 'IFN': 0xD, 'IFG' : 0xE, 'IFB': 0xF }

# Special opcodes
SP_OPCODES = { 'JSR': 0x01 }

# Identifiers
IDEFNTIFIERS = { 'A':   0x0,     'B': 0x1,     'C': 0x2,
                 'X':   0x3,     'Y': 0x4,     'Z': 0x5,
                 'I':   0x6,     'J': 0x7,
                 'POP': 0x18, 'PEEK': 0x19, 'PUSH': 0x1A,
                 'SP':  0x1B,   'PC': 0x1C,
                 'O':   0x1D }

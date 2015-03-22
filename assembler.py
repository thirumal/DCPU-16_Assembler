# DCPU assembler, please see the specification
# See here for a copy: http://pastebin.com/raw.php?i=zyBESXdz
#
# Author: Thirumal Venkat

import os
import sys

# Debug flag (set false during production)
Debug = True

# Keep a line counter, for errors
line_no = 0

# Current word offset
byte_offset = 0

# The tuple list containing all the literals and their offsets
labels = []

# The memory in where code is placed (64kB)
memory = [0] * 0x10000

# Opcode dictionary
opcodes = {
    #     : 0x00, # N/A      - special instruction - see below
    'SET' : 0x01, # SET b, a - sets b to a
    'ADD' : 0x02, # ADD b, a - sets b to b+a, sets EX to 0x0001 if there's
                  # an overflow, 0x0 otherwise
    'SUB' : 0x03, # SUB b, a - sets b to b-a, sets EX to 0xffff if there's
                  #            an underflow, 0x0 otherwise
    'MUL' : 0x04, # MUL b, a - sets b to b*a, sets EX to ((b*a)>>16)&0xffff
                  #            (treats b, a as unsigned)
    'MLI' : 0x05, # MLI b, a - like MUL, but treat b, a as signed
    'DIV' : 0x06, # DIV b, a - sets b to b/a, sets EX to ((b<<16)/a)&0xffff.
                  #            if a==0,sets b and EX to 0 instead.
                  #            (treats b, a as unsigned)
    'DVI' : 0x07, # DVI b, a - like DIV, but treat b, a as signed.
                  # Rounds towards 0
    'MOD' : 0x08, # MOD b, a - sets b to b%a. if a==0, sets b to 0 instead.
    'MDI' : 0x09, # MDI b, a - like MOD, but treat b, a as signed.
                  #            (MDI -7, 16 == -7)
    'AND' : 0x0a, # AND b, a - sets b to b&a
    'BOR' : 0x0b, # BOR b, a - sets b to b#a
    'XOR' : 0x0c, # XOR b, a - sets b to b^a
    'SHR' : 0x0d, # SHR b, a - sets b to b>>>a, sets EX to ((b<<16)>>a)&0xffff
                  #            (logical shift)
    'ASR' : 0x0e, # ASR b, a - sets b to b>>a, sets EX to ((b<<16)>>>a)&0xffff
                  #            (arithmetic shift) (treats b as signed)
    'SHL' : 0x0f, # SHL b, a - sets b to b<<a, sets EX to ((b<<a)>>16)&0xffff
    'IFB' : 0x10, # IFB b, a - performs next instruction only if (b&a)!=0
    'IFC' : 0x11, # IFC b, a - performs next instruction only if (b&a)==0
    'IFE' : 0x12, # IFE b, a - performs next instruction only if b==a
    'IFN' : 0x13, # IFN b, a - performs next instruction only if b!=a
    'IFG' : 0x14, # IFG b, a - performs next instruction only if b>a
    'IFA' : 0x15, # IFA b, a - performs next instruction only if b>a (signed)
    'IFL' : 0x16, # IFL b, a - performs next instruction only if b<a
    'IFU' : 0x17, # IFU b, a - performs next instruction only if b<a (signed)
    #     : 0x18, # None
    #     : 0x19, # None
    'ADX' : 0x1a, # ADX b, a - sets b to b+a+EX, sets EX to 0x0001 if there
                  #            is an overflow, 0x0 otherwise
    'SBX' : 0x1b, # SBX b, a - sets b to b-a+EX, sets EX to 0xFFFF if there is
                  #            an underflow, 0x0001 if there's an overflow,
                  #            0x0 otherwise
    #     : 0x1c, # None
    #     : 0x1d, # None
    'STI' : 0x1e, # STI b, a - sets b to a, then increases I and J by 1
    'STD' : 0x1f  # STD b, a - sets b to a, then decreases I and J by 1
}

# Special opcodes dictionary
sopcodes = {
    #     :  0x00, # n/a   - reserved for future expansion
    'JSR' :  0x01, # JSR a - pushes the address of the next instruction
                   #         to the stack, then sets PC to a
    #     :  0x02, # None
    #     :  0x03, # None
    #     :  0x04, # None
    #     :  0x05, # None
    #     :  0x06, # None
    'HCF' :  0x07, # HCF   - Halt and catch fire!
    'INT' :  0x08, # INT a - triggers a software interrupt with message a
    'IAG' :  0x09, # IAG a - sets a to IA
    'IAS' :  0x0a, # IAS a - sets IA to a
    'RFI' :  0x0b, # RFI a - disables interrupt queueing,
                   #         pops A from the stack,
                   #         then pops PC from the stack
    'IAQ' :  0x0c, # IAQ a - if a is non zero, interrupts will be added
                   #         to the queue instead of triggered. if a is zero,
                   #         interrupts will be triggered as normal again
    #     :  0x0d, # None
    #     :  0x0e, # None
    #     :  0x0f, # None
    'HWN' :  0x10, # HWN a - sets a to number of connected hardware devices
    'HWQ' :  0x11, # HWQ a - sets A, B, C, X, Y registers to information
                   #         about hardware a
    #     :        #         A+(B<<16) is a 32 bit word identifying
                   #         the hardware id
    #     :        #         C is the hardware version
    #     :        #         X+(Y<<16) is a 32 bit word identifying
                   #         the manufacturer
    'HWI' :  0x12, # HWI a - sends an interrupt to hardware a
    #     :  0x13, # None
    #     :  0x14, # None
    #     :  0x15, # None
    #     :  0x16, # None
    #     :  0x17, # None
    #     :  0x18, # None
    #     :  0x19, # None
    #     :  0x1a, # None
    #     :  0x1b, # None
    #     :  0x1c, # None
    #     :  0x1d, # None
    #     :  0x1e, # None
    #     :  0x1f  # None
}

# Specials (all the other operations)
specials = {
    'PUSH' : 0X18, # (PUSH / [--SP]) if in b
    'POP'  : 0X18, # (POP / [SP++]) if in a
    'PEEK' : 0X19, # [SP] / PEEK
    'PICK' : 0X1A, # [SP + next word] / PICK n
    'SP'   : 0X1B, # SP
    'PC'   : 0X1C, # PC
    'EX'   : 0X1D  # EX
}

# Registers
registers = {
    'A' : 0x00,
    'B' : 0x01,
    'C' : 0x02,
    'X' : 0x03,
    'Y' : 0x04,
    'Z' : 0x05,
    'I' : 0x06,
    'J' : 0x07
}

# Directives
directives = ['MACRO', 'DEFINE']

# Reserved keywords
res_keywords = [
    # Opcodes
    'SET', 'ADD', 'SUB', 'MUL', 'MLI', 'DIV', 'DVI', 'MOD', 'MDI', 'AND',
    'BOR', 'XOR', 'SHR', 'ASR', 'SHL', 'IFB', 'IFC', 'IFE', 'IFN', 'IFG',
    'IFA', 'IFL', 'IFU', 'ADX', 'SBX', 'STI', 'STD',
    # Special Opcodes
    'JSR', 'HCF', 'INT', 'IAG', 'IAS', 'IAP', 'IAQ', 'HWN', 'HWQ', 'HWI',
    # Specials
    'PUSH', 'POP', 'PEEK', 'PICK', 'SP', 'PC', 'EX',
    # Registers
    'A', 'B', 'C', 'X', 'Y', 'Z', 'I', 'J',
    #Directives
    'MACRO', 'DEFINE',
    # Operations
    'JMP', 'BRK', 'RET', 'BRA', 'DAT', 'ORG'
]

# The routine to tokenize all tokens
def tokenize(line):
    unwanted_tokens = ['', ',']
    return [x.strip(' \t,') for x in line.upper().split() if x not in unwanted_tokens]

# Routine to process labels
def process_label(token):
    token = token[1:]
    # Sanity check on the length of the label
    if len(token) == 0:
        print "%s: Blank label" % line_no
        sys.exit(1)
    # Append the extracted label onto the labels list
    ltuple = (token, line_no)
    labels.append(ltuple)
    if Debug: print "\tLB:", ltuple

# Routine to process data elements
def process_data(line):
    offset_incr = 0
    # See if there's a label associated with the line, if so remove it
    if line[0] == ':':
        line = line.split(None, 1)[1]
        line = line.strip()
    # Split the line into DAT and the rest of the string
    if len(line.split(None, 1)) > 1:
        line = line.split(None, 1)[1].strip()
    else:
        print "%d: Blank data" % line_no
        sys.exit(1)
    # Now play the guessing game of what type of data we're dealing with
    if Debug: print "\tData:", line, " Length: ", len(line)
    try:
        if line.startswith('"'): # String
            if line.endswith('"'):
                data = line[1:-1]
                # TODO: Validate string, esp \"s
            else:
                raise Exception
            offset_incr = len(line)
        elif line.startswith("'"): # Character
            if line.endswith("'") and len(line) == 3:
                data = line[1]
            else:
                raise Exception
            offset_incr = 1
        elif line.startswith('0X') or line.startswith('0x'): # Hex string
            data = int(line, 16)
            offset_incr = 2
        elif line.startswith('0'): # octal string
            data = int(line, 8)
            offset_incr = 2
        else:
            data = int(line)
            offset_incr = 2
    except Exception:
        print "%s: Invalid data: %s" % (line_no, line)
        sys.exit(1)
    else:
        print "\tParsed data:", data
        # TODO: write_to_memory(data)
    return offset_incr

# Routine to process each instruction
def process_instr(fout, line):
    global byte_offset
    # Print debug message
    if Debug: print "Line: ", line_no, ": ", line
    # Remove comments
    line = line.split(';')[0].strip()
    # If there's nothing to process, then return now
    if len(line) == 0: return
    # If debug enabled, print the line without comments
    if Debug: print "\tNC:", line
    # Now tokenize the line
    tokens = tokenize(line)
    # Filter stuff out
    if Debug: print "\tTK:", tokens
    for token in tokens:
        # If a label process it
        if token.startswith(':'):
            process_label(token)
        # If data then store it
        elif token =='DAT':
            byte_offset += process_data(line)
        elif token in opcodes:
            pass
        # TODO: Implement SET by looking at the test cases

# Main assembler routine
# 1. Opens the input file
# 2. Opens the output file
# 3. Reads all lines from the source code
# 4. Formats each line to our needed specification
# 5. Processes each line by calling process_instr()
def assemble():
    global line_no
    # See if the file name is present in the arguments 
    if len(sys.argv) != 2:
        if not Debug:
            print 'Usage: ./assembler.py <file>'
            sys.exit(1)
        else:
            fname = raw_input("Enter the file name: ");
    else:
        # Open the assembly input file
        fname = sys.argv[1]
    with open(fname, 'r') as fin:
        # Open the binary file to be outputted
        with open(os.path.splitext(os.path.basename(fname))[0] + '.out', 'wb') \
            as fout:
            # 1. Read all lines of the source code
            # 2. Strip all whitespaces
            # 3. Make them all upper case
            #    We only have uppercase instructions and variable names
            instructions = fin.readlines()
            # Process each line (instruction)
            for instr in instructions:
                line_no += 1
                process_instr(fout, instr)

# Main routine, star execution here
if __name__ == '__main__':
    if Debug:
        print "Opcodes = ", opcodes
        print "Special Opcodes = ", sopcodes
        print "Specials = ", specials
        print "Registers = ", registers
        print "Directives = ", directives
        print "Reserved Keywords = ", res_keywords
    assemble()
    if Debug:
        print "Labels = ", labels

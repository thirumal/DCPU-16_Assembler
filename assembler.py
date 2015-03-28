# DCPU assembler, please see the specification
# See here for a copy: http://pastebin.com/raw.php?i=NGE63kKV
#
# Author: Thirumal Venkat

import sys

# Import all keywords and the bak
from keywords import *
from grammar.bak import *

# Generate program using the string
def gen_code_from_source(source):
    print type(source)

# Main routine, star execution here
if __name__ == '__main__':
    # See if the file name is present in the arguments 
    if len(sys.argv) != 2:
        print 'Usage: ./assembler.py <file>'
        fname = raw_input("I'm forgiving you for now, enter a file name: ");
    else:
        # Open the assembly input file
        fname = sys.argv[1]
    with open(fname, 'r') as fin:
        # Generate code
        program = gen_code_from_source(fin.read())
        # Write program to OUTPUT
        print program
        # TODO: Write to output

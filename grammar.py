# Try to import pyparsing and fail if not possible
import sys

try:
    import pyparsing as p
except ImportError:
    print 'Please install pyparsing using the following command:'
    print 'easy_install pyparsing'
    sys.exit(1)

# An identifier can alphanumeric and _, but is should start with an alphabet
identifier = p.Word(p.alphas + '_', p.alphanums + '_')

# Label starts with : and is an identifer
label = p.Combine(p.Literal(':').suppress() + identifier)

# Comment starts with ; <anything else>
comment = p.Literal(';').suppress() + p.restOfLine

# Registers
register = p.oneOf('A B C X Y Z I J SP PC O', caseless = True)
register.addParseAction(p.upcaseTokens)

#Stack operations
stack_ops = p.oneOf('PEEK POP PUSH', caseless = True)
stack_ops.addParseAction(p.upcaseTokens)

# Hexadecimal number
hex_prefix = p.Or(p.Literal('0x') | p.Literal('0X'))
hex_literal = p.Combine(hex_prefix + p.Word(p.hexnums))
hex_literal.setParseAction(lambda s, l, t: int(t[0], 16))

# Decimal Number
dec_literal = p.Word(p.nums)
dec_literal.setParseAction(lambda s, l, t: int(t[0]))

# A Numeric literal is either hexa decimal or decimal
numeric_literal = hex_literal | dec_literal

# A literal is an identifier  or a numerical literal
literal = numeric_literal | identifier

# Opcodes
opcode = p.oneOf('SET '
                 'ADD SUB '
                 'MUL DIV MOD '
                 'SHL SHR '
                 'AND BOR XOR '
                 'IFE IFN IFG IFB'
                 'JSR', caseless= True)
opcode.addParseAction(p.upcaseTokens)

# Operand consists of either a register, stack operation or a literal
basic_operand = (register('register') \
                | stack_ops('stack_ops') \
                | literal('literal'))

# An indirect expression will have <LITERAL> + <REGISTER>
indirect_expr = literal('literal') \
                + p.Literal('+') \
                + register('register')

indirection = p.Literal('[').suppress() \
              + (indirect_expr('indirect_expr') \
                 | basic_operand('basic_operand')) \
              + p.Literal(']').suppress()
              
operand = basic_operand('basic_operand') | indirection('indirection')

def make_words(data):
    '''
    Name: make_words
    Purpose: Make 16-bit words out of list of 8-bit integers
    data - string of integers
    data[::2] - integers at index 0,2,4,6,...
    data[1::2]- integers at index 1,3,5,7,...
    Result: [data[0] << 8 | data[1], data[2] << 8 | data[3],...]
    '''
    try:
        from itertools import izip_longest
    except ImportError:
        from itertools import zip_longest as izip_longest
    res = []
    for a,b in izip_longest(data[::2], data[1::2], fillvalue = 0):
        word = a << 8 | b
        res.append(word)
    return res

def make_string_words(string, loc, tokens):
    '''
    Name: make_string_words
    Purpose: Make words out of strings instead of 8-bit integers
    tokens: Tokens to be processed
    string, loc - ignored!
    Returns list of 16-bit words of integers (string converted into ascii ints)
    '''
    return make_words([ord(c) for c in tokens.string])

# Quoted string in data
quoted_string = p.quotedString('string')
quoted_string.addParseAction(p.removeQuotes)
quoted_string.addParseAction(make_string_words)

# One data item can be either a string or a numeric literal
datum = quoted_string | numeric_literal

def parse_data(string, loc, tokens):
    '''
    Name: parse_data
    Purpose: Parses data passed as tokens
    string, loc = ignored
    tokens = tokens to be parsed
    Returns: Parsed data as datum rules
    '''
    res = []
    for token in tokens:
        values = datum.parseString(token).asList()
        for v in values:
            if v > 0xffff:
                raise Exception("Datum exceeds word size")
        res.extend(values)
    return res

# Parser for comma seperated list of data
datalist = p.commaSeparatedList
datalist.setParseAction(parse_data)

# The entire data string with DAT in the front
data = p.CaselessKeyword("DAT") + p.Group(datalist)

instruction = p.Group(opcode("opcode")
               + p.Group(operand("first"))
               + p.Optional(p.Literal(',').suppress() +
                            p.Group(operand('second')))
)

# All statements are either instructions or data
# statement = instruction('instruction') | data('data')
statement = instruction('instruction') | data('data')

# The entire line
line = p.Group(p.Optional(label('label')) \
               + p.Optional(statement('statement'), default = None) \
               + p.Optional(comment('comment')) \
               + p.LineEnd())('line')

# The entire grammar
full_grammar =  p.ZeroOrMore(line)

blue = full_grammar.parseString('\n:bad2 dat 123\n:bad2 dat 234')

print blue
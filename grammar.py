from itertools import izip_longest

import pyparsing as p

# don't mark \n as a delimeter, we need to parse an entire line as one entity
p.ParserElement.setDefaultWhitespaceChars(" \t")

identifier = p.Word(p.alphas + "_", p.alphanums + "_")

label = p.Combine(p.Literal(":").suppress() + identifier)

comment = p.Literal(";").suppress() + p.restOfLine

register = p.oneOf("A B C I J X Y Z O PC SP", caseless=True).addParseAction(p.upcaseTokens)

stack_op = p.oneOf("PEEK POP PUSH", caseless=True).addParseAction(p.upcaseTokens)

hex_literal = p.Combine(p.Literal("0x") + p.Word(p.hexnums)).setParseAction(lambda s, l, t: int(t[0], 16))
dec_literal = p.Word(p.nums).setParseAction(lambda s, l, t: int(t[0]))

numeric_literal = hex_literal | dec_literal

literal = numeric_literal | identifier

opcode = p.oneOf("SET ADD SUB MUL DIV MOD SHL SHR AND BOR XOR IFE IFN IFG IFB JSR", caseless=True).addParseAction(p.upcaseTokens)

basic_operand = p.Group(register("register") | stack_op("stack_op") | literal("literal"))

indirect_expr = p.Group(literal("literal") + p.Literal("+") + register("register"))

indirection_content = indirect_expr("expr") | basic_operand("basic")

indirection = p.Group(p.Literal('[').suppress() + indirection_content + p.Literal(']').suppress())

operand = basic_operand("basic") | indirection("indirect")

def make_words(data):
    return [a << 8 | b for a, b in izip_longest(data[::2], data[1::2], fillvalue=0)]


def make_string_words(s, l, tokens):
    mybytes = [ord(c) for c in tokens.string]
    packed = False
    return make_words(bytes) if packed else mybytes

quoted_string = p.quotedString("string").addParseAction(p.removeQuotes).addParseAction(make_string_words)
datum = quoted_string | numeric_literal

def parse_data(string, loc, tokens):
    result = []
    for token in tokens:
        values = datum.parseString(token).asList()
        assert all(v <= 0xFFFF for v in values), "Datum exceeds word size"
        result.extend(values)
    return result

datalist = p.commaSeparatedList.copy().setParseAction(parse_data)

data = p.CaselessKeyword("DAT")("opcode") + p.Group(datalist)("data")

instruction = (opcode("opcode") + p.Group(operand)("first") + p.Optional(p.Literal(",").suppress() + p.Group(operand)("second")))

statement = p.Group(instruction | data)

line = p.Group(p.Optional(label("label")) + p.Optional(statement("statement"), default=None) + p.Optional(comment("comment")) + p.lineEnd.suppress())("line")

full_grammar = p.ZeroOrMore(line)('program')

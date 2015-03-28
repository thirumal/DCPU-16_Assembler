import pyparsing as p

identifier = p.Word(p.alphas + '_', p.alphanums + '_')

line = p.OneOrMore(identifier) + p.LineEnd().suppress()

print line.parseString('hello\n world\n bye\n world\n _super\n rocket\n')
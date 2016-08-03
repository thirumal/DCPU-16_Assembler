# DCPU-16 Assembler

DCPU-16 Assembler written in C

## Notes

* Labels cannot contain reserved keywords
* Data and instructions need to be in one line (no multi-line support)

## TODO List
* Support literal in front of register in register indirect literal addressing.
For example as of today we support `SET [A + 0x200], 20` but not `SET [0x200 + A], 20`
* Support list of numbers and strings as data. As of today we just support string data.

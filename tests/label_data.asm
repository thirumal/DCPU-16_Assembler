; Test cases for DCPU-16 assembler

; Test for labels and data
:mystring DAT   "hello \" world!"
:myhex  dat 0X15
:myoct dat 017
:myint dat 157
:mychar dat 'z'
dat 1599
; Uncomment the following lines one by one to test negative test cases
;: dat "hello" ; When uncommented should emit blank label error
; dat ; Blank data error
; :mybaddat dat ; Blank data error

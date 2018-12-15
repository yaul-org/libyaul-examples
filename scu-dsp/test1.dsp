; Performs RAM2[0] = RAM0[0] * RAM1[0]

Start:
; ALU           X-bus           Y-bus           D1-bus
  NOP           NOP             NOP             MOV 0, CT0
  NOP           NOP             CLR A           MOV 0, CT1
  NOP           MOV MC0, X      MOV MC1, Y      NOP
  NOP           MOV MUL, P      NOP             NOP
  AD2           NOP             NOP             MOV ALL, MC2
  ENDI

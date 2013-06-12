inplace code generation

register layout:
[0]: always 0
[1]: arithmetic dest r / return value
[2]: arithmetic source s
[3]: arithmetic source t
[4]: tmp
[5]: tmp
[6]: local env base
[7]: program counter

calling convention:
cdecl-like.
arguements are passed sequentially through the stack.
return value in reg[1] if any
callee cleans the stack
register reservations after function invokation:
[0]: reserved
[1]-[3]: not reserved
[4]-[7]: reserved


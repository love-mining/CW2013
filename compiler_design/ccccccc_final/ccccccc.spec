inplace code generation

1. register layout:
[0]: arithmetic dest r / return value
[1]: arithmetic source s
[2]: arithmetic source t
[3]: tmp
[4]: tmp
[5]: stack pointer / initial: sbrk()
[6]: local env base / initial: sbrk()
[7]: program counter / initial: 0

2. calling convention:
cdecl-like.
arguements are passed sequentially through the stack.
return value in reg[0] if any
callee cleans the stack
register reservations after function invokation:
[0]-[3]: not reserved
[4]-[7]: reserved

3. stack layout:

--prev:
...
params
return address
--current:
stored stack pointer
stored frame pointer
locals
temps
...
params
return address
--next:
stored stack pointer
stored frame pointer
locals
temps
...


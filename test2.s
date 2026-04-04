; TinyCPU assembly -- test2.js

_t3abs:
  .entry:
    ; icmp
    br
  .if.then:
    sub
    ret
  .if.merge:
    ret

_t3max:
  .entry:
    ; icmp
    br
  .if.then:
    ret
  .if.else:
    ret

_t6isEven:
  .entry:
    ; srem
    ; icmp
    br
  .if.then:
    ret
  .if.merge:
    ret

_t3gcd:
  .entry:
    br
  .while.cond:
    ; phi
    ; phi
    ; icmp
    br
  .while.body:
    ; srem
    br
  .while.after:
    ret

_t5sumTo:
  .entry:
    br
  .while.cond:
    ; phi
    ; phi
    ; icmp
    br
  .while.body:
    add
    add
    br
  .while.after:
    ret

_t7compute:
  .entry:
    ; call
    ; call
    ; call
    ; call
    ; call
    add
    mul
    ; sdiv
    ret


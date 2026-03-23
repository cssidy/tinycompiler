; ModuleID = 'test2.js'
source_filename = "test2.js"

define i64 @_t3abs(i64 %x) {
entry:
  %0 = icmp slt i64 %x, 0
  br i1 %0, label %if.then, label %if.merge

if.then:                                          ; preds = %entry
  %1 = sub i64 0, %x
  ret i64 %1

if.merge:                                         ; preds = %entry
  ret i64 %x
}

define i64 @_t3max(i64 %a, i64 %b) {
entry:
  %0 = icmp sgt i64 %a, %b
  br i1 %0, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  ret i64 %a

if.merge:                                         ; No predecessors!
  ret i64 0

if.else:                                          ; preds = %entry
  ret i64 %b
}

define i64 @_t6isEven(i64 %n) {
entry:
  %0 = srem i64 %n, 2
  %1 = icmp eq i64 %0, 0
  br i1 %1, label %if.then, label %if.merge

if.then:                                          ; preds = %entry
  ret i64 1

if.merge:                                         ; preds = %entry
  ret i64 0
}

define i64 @_t3gcd(i64 %a, i64 %b) {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %0 = phi i64 [ %1, %while.body ], [ %a, %entry ]
  %1 = phi i64 [ %3, %while.body ], [ %b, %entry ]
  %2 = icmp ne i64 %1, 0
  br i1 %2, label %while.body, label %while.after

while.body:                                       ; preds = %while.cond
  %3 = srem i64 %0, %1
  br label %while.cond

while.after:                                      ; preds = %while.cond
  ret i64 %0
}

define i64 @_t5sumTo(i64 %n) {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %0 = phi i64 [ %3, %while.body ], [ 0, %entry ]
  %1 = phi i64 [ %4, %while.body ], [ 1, %entry ]
  %2 = icmp ne i64 %1, %n
  br i1 %2, label %while.body, label %while.after

while.body:                                       ; preds = %while.cond
  %3 = add i64 %0, %1
  %4 = add i64 %1, 1
  br label %while.cond

while.after:                                      ; preds = %while.cond
  ret i64 %0
}

define i64 @_t7compute(i64 %a, i64 %b) {
entry:
  %0 = call i64 @_t3gcd(i64 %a, i64 %b)
  %1 = call i64 @_t5sumTo(i64 10)
  %2 = call i64 @_t3abs(i64 %a)
  %3 = call i64 @_t3abs(i64 %b)
  %4 = call i64 @_t3max(i64 %2, i64 %3)
  %5 = add i64 %1, %0
  %6 = mul i64 %5, %4
  %7 = sdiv i64 %6, 2
  ret i64 %7
}

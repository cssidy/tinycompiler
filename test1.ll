; ModuleID = 'test1.js'
source_filename = "test1.js"

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

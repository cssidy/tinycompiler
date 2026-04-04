; ModuleID = 'test2.js'
source_filename = "test2.js"

define i64 @_t3abs(i64 %x) !dbg !2 {
entry:
    #dbg_declare(i64 %x, !7, !DIExpression(), !8)
  %0 = icmp slt i64 %x, 0, !dbg !8
  br i1 %0, label %if.then, label %if.merge, !dbg !8

if.then:                                          ; preds = %entry
  %1 = sub i64 0, %x, !dbg !8
  ret i64 %1, !dbg !8

if.merge:                                         ; preds = %entry
  ret i64 %x, !dbg !8
}

define i64 @_t3max(i64 %a, i64 %b) !dbg !9 {
entry:
    #dbg_declare(i64 %a, !12, !DIExpression(), !13)
    #dbg_declare(i64 %b, !14, !DIExpression(), !13)
  %0 = icmp sgt i64 %a, %b, !dbg !13
  br i1 %0, label %if.then, label %if.else, !dbg !13

if.then:                                          ; preds = %entry
  ret i64 %a, !dbg !13

if.else:                                          ; preds = %entry
  ret i64 %b, !dbg !13
}

define i64 @_t6isEven(i64 %n) !dbg !15 {
entry:
    #dbg_declare(i64 %n, !16, !DIExpression(), !17)
  %0 = srem i64 %n, 2, !dbg !17
  %1 = icmp eq i64 %0, 0, !dbg !17
  br i1 %1, label %if.then, label %if.merge, !dbg !17

if.then:                                          ; preds = %entry
  ret i64 1, !dbg !17

if.merge:                                         ; preds = %entry
  ret i64 0, !dbg !17
}

define i64 @_t3gcd(i64 %a, i64 %b) !dbg !18 {
entry:
    #dbg_declare(i64 %a, !19, !DIExpression(), !20)
    #dbg_declare(i64 %b, !21, !DIExpression(), !20)
  br label %while.cond, !dbg !20

while.cond:                                       ; preds = %while.body, %entry
  %0 = phi i64 [ %1, %while.body ], [ %a, %entry ]
  %1 = phi i64 [ %3, %while.body ], [ %b, %entry ]
  %2 = icmp ne i64 %1, 0, !dbg !20
  br i1 %2, label %while.body, label %while.after, !dbg !20

while.body:                                       ; preds = %while.cond
  %3 = srem i64 %0, %1, !dbg !20
  br label %while.cond, !dbg !20

while.after:                                      ; preds = %while.cond
  ret i64 %0, !dbg !20
}

define i64 @_t5sumTo(i64 %n) !dbg !22 {
entry:
    #dbg_declare(i64 %n, !23, !DIExpression(), !24)
  br label %while.cond, !dbg !24

while.cond:                                       ; preds = %while.body, %entry
  %0 = phi i64 [ %3, %while.body ], [ 0, %entry ]
  %1 = phi i64 [ %4, %while.body ], [ 1, %entry ]
  %2 = icmp ne i64 %1, %n, !dbg !24
  br i1 %2, label %while.body, label %while.after, !dbg !24

while.body:                                       ; preds = %while.cond
  %3 = add i64 %0, %1, !dbg !24
  %4 = add i64 %1, 1, !dbg !24
  br label %while.cond, !dbg !24

while.after:                                      ; preds = %while.cond
  ret i64 %0, !dbg !24
}

define i64 @_t7compute(i64 %a, i64 %b) !dbg !25 {
entry:
    #dbg_declare(i64 %a, !26, !DIExpression(), !27)
    #dbg_declare(i64 %b, !28, !DIExpression(), !27)
  %0 = call i64 @_t3gcd(i64 %a, i64 %b), !dbg !27
  %1 = call i64 @_t5sumTo(i64 10), !dbg !27
  %2 = call i64 @_t3abs(i64 %a), !dbg !27
  %3 = call i64 @_t3abs(i64 %b), !dbg !27
  %4 = call i64 @_t3max(i64 %2, i64 %3), !dbg !27
  %5 = add i64 %1, %0, !dbg !27
  %6 = mul i64 %5, %4, !dbg !27
  %7 = sdiv i64 %6, 2, !dbg !27
  ret i64 %7, !dbg !27
}

!llvm.dbg.cu = !{!0}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "tinycompiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "test2.js", directory: ".")
!2 = distinct !DISubprogram(name: "abs", linkageName: "_t3abs", scope: null, file: !1, line: 1, type: !3, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !6)
!3 = !DISubroutineType(types: !4)
!4 = !{!5, !5}
!5 = !DIBasicType(name: "i64", size: 64, encoding: DW_ATE_signed)
!6 = !{}
!7 = !DILocalVariable(name: "x", arg: 1, scope: !2, file: !1, line: 1, type: !5)
!8 = !DILocation(line: 1, column: 1, scope: !2)
!9 = distinct !DISubprogram(name: "max", linkageName: "_t3max", scope: null, file: !1, line: 1, type: !10, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !6)
!10 = !DISubroutineType(types: !11)
!11 = !{!5, !5, !5}
!12 = !DILocalVariable(name: "a", arg: 1, scope: !9, file: !1, line: 1, type: !5)
!13 = !DILocation(line: 1, column: 1, scope: !9)
!14 = !DILocalVariable(name: "b", arg: 2, scope: !9, file: !1, line: 1, type: !5)
!15 = distinct !DISubprogram(name: "isEven", linkageName: "_t6isEven", scope: null, file: !1, line: 1, type: !3, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !6)
!16 = !DILocalVariable(name: "n", arg: 1, scope: !15, file: !1, line: 1, type: !5)
!17 = !DILocation(line: 1, column: 1, scope: !15)
!18 = distinct !DISubprogram(name: "gcd", linkageName: "_t3gcd", scope: null, file: !1, line: 1, type: !10, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !6)
!19 = !DILocalVariable(name: "a", arg: 1, scope: !18, file: !1, line: 1, type: !5)
!20 = !DILocation(line: 1, column: 1, scope: !18)
!21 = !DILocalVariable(name: "b", arg: 2, scope: !18, file: !1, line: 1, type: !5)
!22 = distinct !DISubprogram(name: "sumTo", linkageName: "_t5sumTo", scope: null, file: !1, line: 1, type: !3, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !6)
!23 = !DILocalVariable(name: "n", arg: 1, scope: !22, file: !1, line: 1, type: !5)
!24 = !DILocation(line: 1, column: 1, scope: !22)
!25 = distinct !DISubprogram(name: "compute", linkageName: "_t7compute", scope: null, file: !1, line: 1, type: !10, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !6)
!26 = !DILocalVariable(name: "a", arg: 1, scope: !25, file: !1, line: 1, type: !5)
!27 = !DILocation(line: 1, column: 1, scope: !25)
!28 = !DILocalVariable(name: "b", arg: 2, scope: !25, file: !1, line: 1, type: !5)

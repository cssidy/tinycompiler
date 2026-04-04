; ModuleID = 'test_loop.js'
source_filename = "test_loop.js"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define noundef i64 @_t7sumFour() local_unnamed_addr #0 !dbg !2 {
entry:
  ret i64 6, !dbg !6
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }

!llvm.dbg.cu = !{!0}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "tinycompiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "test_loop.js", directory: ".")
!2 = distinct !DISubprogram(name: "sumFour", linkageName: "_t7sumFour", scope: null, file: !1, line: 1, type: !3, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
!3 = !DISubroutineType(types: !4)
!4 = !{!5}
!5 = !DIBasicType(name: "i64", size: 64, encoding: DW_ATE_signed)
!6 = !DILocation(line: 1, column: 1, scope: !2)

; ModuleID = 'test3.js'
source_filename = "test3.js"

%Vehicle = type { ptr, i64 }
%Mazda = type { ptr, i64, i64, i64, i64 }

@Vehicle_VTable = constant { ptr } { ptr @Vehicle_sound }
@Mazda_VTable = constant { ptr } { ptr @Mazda_sound }
@0 = private unnamed_addr constant [18 x i8] c"vroom vroom vroom\00", align 1
@1 = private unnamed_addr constant [15 x i8] c"zoom zoom zoom\00", align 1
@_ZTIi = external constant ptr

define ptr @Vehicle_sound(ptr %this) !dbg !3 {
entry:
    #dbg_declare(ptr %this, !8, !DIExpression(), !9)
  ret ptr @0, !dbg !9
}

define ptr @Mazda_sound(ptr %this) !dbg !10 {
entry:
    #dbg_declare(ptr %this, !11, !DIExpression(), !12)
  ret ptr @1, !dbg !12
}

define void @Vehicle_init(ptr %this) !dbg !13 {
entry:
  %vptr = getelementptr inbounds nuw %Vehicle, ptr %this, i32 0, i32 0, !dbg !16
  store ptr @Vehicle_VTable, ptr %vptr, align 8, !dbg !16
  %wheels = getelementptr inbounds nuw %Vehicle, ptr %this, i32 0, i32 1, !dbg !16
  store i64 4, ptr %wheels, align 4, !dbg !16
  ret void, !dbg !16
}

define void @Mazda_init(ptr %this) !dbg !17 {
entry:
  %vptr = getelementptr inbounds nuw %Mazda, ptr %this, i32 0, i32 0, !dbg !18
  store ptr @Mazda_VTable, ptr %vptr, align 8, !dbg !18
  %wheels = getelementptr inbounds nuw %Mazda, ptr %this, i32 0, i32 1, !dbg !18
  store i64 4, ptr %wheels, align 4, !dbg !18
  %doors = getelementptr inbounds nuw %Mazda, ptr %this, i32 0, i32 2, !dbg !18
  store i64 4, ptr %doors, align 4, !dbg !18
  %make = getelementptr inbounds nuw %Mazda, ptr %this, i32 0, i32 3, !dbg !18
  store i64 1, ptr %make, align 4, !dbg !18
  %model = getelementptr inbounds nuw %Mazda, ptr %this, i32 0, i32 4, !dbg !18
  store i64 3, ptr %model, align 4, !dbg !18
  ret void, !dbg !18
}

declare i32 @__gxx_personality_v0(...)

declare ptr @__cxa_allocate_exception(i64)

declare void @__cxa_throw(ptr, ptr, ptr)

declare ptr @__cxa_begin_catch(ptr)

declare void @__cxa_end_catch()

declare i32 @puts(ptr)

define i64 @_t4main() personality ptr @__gxx_personality_v0 !dbg !19 {
entry:
  %Mazda.obj = alloca %Mazda, align 8
  call void @Mazda_init(ptr %Mazda.obj), !dbg !23
  %vtable = load ptr, ptr %Mazda.obj, align 8, !dbg !23
  %fn.slot = getelementptr inbounds ptr, ptr %vtable, i64 0, !dbg !23
  %fn = load ptr, ptr %fn.slot, align 8, !dbg !23
  %0 = invoke ptr %fn(ptr %Mazda.obj)
          to label %invoke.cont unwind label %lpad, !dbg !23

invoke.cont:                                      ; preds = %catch, %entry
  ret i64 0, !dbg !23

lpad:                                             ; preds = %entry
  %exc = landingpad { ptr, i32 }
          catch ptr @_ZTIi, !dbg !23
  %exc.ptr = extractvalue { ptr, i32 } %exc, 0, !dbg !23
  %exc.sel = extractvalue { ptr, i32 } %exc, 1, !dbg !23
  %typeid = call i32 @llvm.eh.typeid.for.p0(ptr @_ZTIi), !dbg !23
  %matches = icmp eq i32 %exc.sel, %typeid, !dbg !23
  br i1 %matches, label %catch, label %resume, !dbg !23

catch:                                            ; preds = %lpad
  %1 = call ptr @__cxa_begin_catch(ptr %exc.ptr), !dbg !23
  call void @__cxa_end_catch(), !dbg !23
  br label %invoke.cont, !dbg !23

resume:                                           ; preds = %lpad
  resume { ptr, i32 } %exc, !dbg !23
}

; Function Attrs: nounwind memory(none)
declare i32 @llvm.eh.typeid.for.p0(ptr) #0

attributes #0 = { nounwind memory(none) }

!llvm.dbg.cu = !{!0, !2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "tinycompiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "test3.js", directory: ".")
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "tinycompiler", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
!3 = distinct !DISubprogram(name: "Vehicle_sound", linkageName: "Vehicle_sound", scope: null, file: !1, line: 1, type: !4, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !7)
!4 = !DISubroutineType(types: !5)
!5 = !{!6, !6}
!6 = !DIBasicType(name: "ptr", size: 64, encoding: DW_ATE_address)
!7 = !{}
!8 = !DILocalVariable(name: "this", arg: 1, scope: !3, file: !1, line: 1, type: !6)
!9 = !DILocation(line: 1, column: 1, scope: !3)
!10 = distinct !DISubprogram(name: "Mazda_sound", linkageName: "Mazda_sound", scope: null, file: !1, line: 1, type: !4, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !7)
!11 = !DILocalVariable(name: "this", arg: 1, scope: !10, file: !1, line: 1, type: !6)
!12 = !DILocation(line: 1, column: 1, scope: !10)
!13 = distinct !DISubprogram(name: "Vehicle_init", linkageName: "Vehicle_init", scope: null, file: !1, line: 1, type: !14, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
!14 = !DISubroutineType(types: !15)
!15 = !{null, !6}
!16 = !DILocation(line: 1, column: 1, scope: !13)
!17 = distinct !DISubprogram(name: "Mazda_init", linkageName: "Mazda_init", scope: null, file: !1, line: 1, type: !14, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
!18 = !DILocation(line: 1, column: 1, scope: !17)
!19 = distinct !DISubprogram(name: "main", linkageName: "_t4main", scope: null, file: !1, line: 1, type: !20, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2)
!20 = !DISubroutineType(types: !21)
!21 = !{!22}
!22 = !DIBasicType(name: "i64", size: 64, encoding: DW_ATE_signed)
!23 = !DILocation(line: 1, column: 1, scope: !19)

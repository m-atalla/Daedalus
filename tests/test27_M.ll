; ModuleID = 'tests/test27_F.ll'
source_filename = "tests/test27.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local void @g(i32 noundef %a) #0 {
entry:
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %a)
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @f(i32 noundef %arg) #0 {
entry:
  %rem = srem i32 %arg, 2
  %tobool = icmp ne i32 %rem, 0
  br i1 %tobool, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %return

if.end:                                           ; preds = %entry
  %mul = mul nsw i32 %arg, 2
  br label %return

return:                                           ; preds = %if.end, %if.then
  %retval.0 = phi i32 [ 0, %if.then ], [ %mul, %if.end ]
  ret i32 %retval.0
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 noundef %argc, ptr noundef %argv) #0 {
entry:
  %mul = mul nsw i32 %argc, %argc
  call void @g(i32 noundef 2)
  %call = call i32 @f(i32 noundef %argc)
  ret i32 %call
}

; Function Attrs: nounwind willreturn
define internal i32 @_wyvern_slice_f_retval.0_259447827(i32 %arg) #0 {
sliceclone_entry:
  %0 = srem i32 %arg, 2
  %1 = icmp ne i32 %0, 0
  br i1 %1, label %sliceclone_if.then, label %sliceclone_if.end

sliceclone_if.then:                               ; preds = %sliceclone_entry
  br label %sliceclone_return

sliceclone_if.end:                                ; preds = %sliceclone_entry
  %2 = mul nsw i32 %arg, 2
  br label %sliceclone_return

sliceclone_return:                                ; preds = %sliceclone_if.end, %sliceclone_if.then
  %3 = phi i32 [ 0, %sliceclone_if.then ], [ %2, %sliceclone_if.end ]
  ret i32 %3
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind willreturn "Daedalus_Slice" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 17.0.6 (https://github.com/llvm/llvm-project.git 6009708b4367171ccdbf4b5905cb6a803753fe18)"}

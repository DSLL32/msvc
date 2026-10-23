; ModuleID = '/root/.ai-tmp/msvc2/vc1.0/c1/analysis/parser-return-abi/isolate.c'
source_filename = "/root/.ai-tmp/msvc2/vc1.0/c1/analysis/parser-return-abi/isolate.c"
target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-i128:128-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-pc-linux-gnu"

%union.A = type { [2 x i32] }
%union.B = type { %struct.S }
%struct.S = type { i32, i32 }
%union.C = type { %struct.S }
%union.D = type { %struct.S }
%union.E = type { %union.A }
%union.F = type { [2 x i32] }
%union.G = type { i16, [6 x i8] }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_stdcallcc i64 @a(i32 noundef %0) #0 {
  %2 = alloca %union.A, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = getelementptr inbounds %union.A, ptr %2, i32 0, i32 0
  %5 = load i64, ptr %4, align 4
  ret i64 %5
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_stdcallcc i64 @b(i32 noundef %0) #0 {
  %2 = alloca %union.B, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = getelementptr inbounds %union.B, ptr %2, i32 0, i32 0
  %5 = load i64, ptr %4, align 4
  ret i64 %5
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_stdcallcc i64 @c(i32 noundef %0) #0 {
  %2 = alloca %union.C, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = getelementptr inbounds %union.C, ptr %2, i32 0, i32 0
  %5 = load i64, ptr %4, align 4
  ret i64 %5
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_stdcallcc void @d(ptr dead_on_unwind noalias writable sret(%union.D) align 4 %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 4
  %4 = alloca i32, align 4
  store ptr %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_stdcallcc i64 @e(i32 noundef %0) #0 {
  %2 = alloca %union.E, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = getelementptr inbounds %union.E, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds %union.A, ptr %4, i32 0, i32 0
  %6 = load i64, ptr %5, align 4
  ret i64 %6
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_stdcallcc i64 @f(i32 noundef %0) #0 {
  %2 = alloca %union.F, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = getelementptr inbounds %union.F, ptr %2, i32 0, i32 0
  %5 = load i64, ptr %4, align 4
  ret i64 %5
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_stdcallcc i64 @g(i32 noundef %0) #0 {
  %2 = alloca %union.G, align 2
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = load i64, ptr %2, align 2
  ret i64 %4
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="i686" "target-features"="+cmov,+cx8,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4, !5}
!llvm.ident = !{!6}

!0 = !{i32 1, !"NumRegisterParameters", i32 0}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"PIE Level", i32 2}
!4 = !{i32 7, !"uwtable", i32 2}
!5 = !{i32 7, !"frame-pointer", i32 2}
!6 = !{!"Debian clang version 19.1.7 (3+b1)"}

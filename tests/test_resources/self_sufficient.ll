; ModuleID = 'self_sufficient.cpp'
source_filename = "self_sufficient.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@gpublic = global i32 69, align 4
@gpublic_vol = global i32 0, align 4
@_ZL8gprivate = internal global i32 269, align 4

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define void @add2globals(i32) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %4 = load i32, i32* %2, align 4
  %5 = load i32, i32* @gpublic, align 4
  %6 = add nsw i32 %5, %4
  store i32 %6, i32* @gpublic, align 4
  %7 = load i32, i32* %2, align 4
  %8 = load i32, i32* @_ZL8gprivate, align 4
  %9 = add nsw i32 %8, %7
  store i32 %9, i32* @_ZL8gprivate, align 4
  store i32 0, i32* %3, align 4
  %10 = load i32, i32* @gpublic, align 4
  %11 = add nsw i32 2, %10
  store i32 %11, i32* %3, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define void @use_volatile(i32) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %4 = load volatile i32, i32* @gpublic_vol, align 4
  store i32 %4, i32* %3, align 4
  %5 = load i32, i32* %3, align 4
  %6 = load i32, i32* %2, align 4
  %7 = add nsw i32 %5, %6
  store volatile i32 %7, i32* @gpublic_vol, align 4
  %8 = load i32, i32* %3, align 4
  call void @add2globals(i32 %8)
  ret void
}

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define i32 @use_branch(i1 zeroext) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i8, align 1
  %4 = zext i1 %0 to i8
  store i8 %4, i8* %3, align 1
  %5 = load i8, i8* %3, align 1
  %6 = trunc i8 %5 to i1
  br i1 %6, label %7, label %8

; <label>:7:                                      ; preds = %1
  store i32 69, i32* %2, align 4
  br label %9

; <label>:8:                                      ; preds = %1
  store i32 369, i32* %2, align 4
  br label %9

; <label>:9:                                      ; preds = %8, %7
  %10 = load i32, i32* %2, align 4
  ret i32 %10
}

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define i32 @pow_naive(i32, i32) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32 %1, i32* %4, align 4
  store i32 0, i32* %5, align 4
  br label %6

; <label>:6:                                      ; preds = %14, %2
  %7 = load i32, i32* %5, align 4
  %8 = load i32, i32* %4, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %17

; <label>:10:                                     ; preds = %6
  %11 = load i32, i32* %3, align 4
  %12 = load i32, i32* %3, align 4
  %13 = mul nsw i32 %12, %11
  store i32 %13, i32* %3, align 4
  br label %14

; <label>:14:                                     ; preds = %10
  %15 = load i32, i32* %5, align 4
  %16 = add nsw i32 %15, 1
  store i32 %16, i32* %5, align 4
  br label %6

; <label>:17:                                     ; preds = %6
  %18 = load i32, i32* %3, align 4
  ret i32 %18
}

attributes #0 = { noinline nounwind optnone sspstrong uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 5.0.1 (tags/RELEASE_501/final)"}

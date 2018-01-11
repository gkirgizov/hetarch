; ModuleID = 'part2.cpp'
source_filename = "part2.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@important_result = global i32 0, align 4
@important_data = external global i32, align 4

; Function Attrs: noinline optnone sspstrong uwtable
define i32 @pow_smart(i32, i32) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, i32* %4, align 4
  store i32 %1, i32* %5, align 4
  %6 = load i32, i32* %5, align 4
  %7 = icmp eq i32 %6, 0
  br i1 %7, label %8, label %9

; <label>:8:                                      ; preds = %2
  store i32 1, i32* %3, align 4
  br label %26

; <label>:9:                                      ; preds = %2
  %10 = load i32, i32* %5, align 4
  %11 = urem i32 %10, 2
  %12 = icmp eq i32 %11, 0
  br i1 %12, label %13, label %19

; <label>:13:                                     ; preds = %9
  %14 = load i32, i32* %4, align 4
  %15 = call i32 @square(i32 %14)
  %16 = load i32, i32* %5, align 4
  %17 = udiv i32 %16, 2
  %18 = call i32 @pow_smart(i32 %15, i32 %17)
  store i32 %18, i32* %3, align 4
  br label %26

; <label>:19:                                     ; preds = %9
  %20 = load i32, i32* %4, align 4
  %21 = load i32, i32* %4, align 4
  %22 = load i32, i32* %5, align 4
  %23 = sub i32 %22, 1
  %24 = call i32 @pow_smart(i32 %21, i32 %23)
  %25 = mul nsw i32 %20, %24
  store i32 %25, i32* %3, align 4
  br label %26

; <label>:26:                                     ; preds = %19, %13, %8
  %27 = load i32, i32* %3, align 4
  ret i32 %27
}

declare i32 @square(i32) #1

; Function Attrs: noinline optnone sspstrong uwtable
define void @try_pow() #0 {
  %1 = load i32, i32* @important_data, align 4
  %2 = call i32 @pow_smart(i32 %1, i32 5)
  store i32 %2, i32* @important_result, align 4
  ret void
}

attributes #0 = { noinline optnone sspstrong uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 5.0.1 (tags/RELEASE_501/final)"}

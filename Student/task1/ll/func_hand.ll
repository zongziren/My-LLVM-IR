define dso_local i32 @add(i32 %0, i32 %1) #0 {
2:
    %3 = alloca i32, align 4
    %4 = alloca i32, align 4
    %5 = alloca i32, align 4
    store i32 %0, i32* %4, align 4
    store i32 %1, i32* %5, align 4
    %6 = load i32, i32* %4, align 4
    %7 = load i32, i32* %5, align 4
    %8 = add nsw i32 %6, %7
    %9 = sub nsw i32 %8, 1
    store i32 %9, i32* %3, align 4
    br label %10

10:
    %11 = load i32, i32* %3, align 4
    ret i32 %11
}

define dso_local i32 @main() #0 {
0:
    %1 = alloca i32, align 4
    %2 = alloca i32, align 4
    %3 = alloca i32, align 4
    %4 = alloca i32, align 4
    store i32 0, i32* %1, align 4
    store i32 3, i32* %2, align 4
    store i32 2, i32* %3, align 4
    store i32 5, i32* %4, align 4
    %5 = load i32, i32* %2, align 4
    %6 = load i32, i32* %3, align 4
    %7 = call i32 @add(i32 %5, i32 %6)
    %8 = load i32, i32* %4, align 4
    %9 = add nsw i32 %7, %8
    ret i32 %9
}
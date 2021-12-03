@a = dso_local global i32 0, align 4

define dso_local i32 @main() #0 {
0:
    %1 = alloca i32, align 4
    %2 = alloca i32, align 4
    store i32 10, i32* %2, align 4
    br label %3
3:
    %4 = load i32, i32* %2, align 4
    %5 = icmp sgt i32 %4, 0
    br i1 %5, label %6, label %8
6:
    %7 = load i32, i32* %2, align 4
    ret i32 %7
8:
    ret i32 0
}
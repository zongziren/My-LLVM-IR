; ModuleID = 'func_test.sy'
source_filename = "func_test.sy"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define dso_local i32 @add(i32 %0, i32 %1) #0 {
    %3 = alloca i32, align 4
    %4 = alloca i32, align 4
    ;分配地址存储a and b
    store i32 %0, i32* %3, align 4
    store i32 %1, i32* %4, align 4
    ;传入a and b的值
    %5 = load i32, i32* %3, align 4
    %6 = load i32, i32* %4, align 4
    ;取出a and b的值
    %7 = add nsw i32 %5, %6
    %8 = add nsw i32 %7, 1
    ;计算(a+b-1)
    ret i32 %7
    ;返回(a+b-1)
}


define dso_local i32 @main() #0 {
    %1 = alloca i32, align 4
    %2 = alloca i32, align 4
    %3 = alloca i32, align 4
    ;分配a,b,c的地址
    store i32 3, i32* %1, align 4
    store i32 2, i32* %2, align 4
    store i32 5, i32* %3, align 4
    ;对a,b,c进行初始化
    %4 = load i32, i32* %2, align 4
    %5 = load i32, i32* %3, align 4
    ;取a,b的值
    %6 = call i32 @add(i32 %4,i32 %5)
    ;调用add()
    %7 = load i32, i32* %3, align 4
    ;取c的值
    %8 = add nsw i32 %6, %7
    ;c+add()
    ret i32 %8
    ;返回
}
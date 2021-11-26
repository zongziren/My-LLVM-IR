; ModuleID = 'assign_test.sy'
source_filename = "assign_test.sy"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define dso_local i32 @main() #0 {
    %1 = alloca float, align 4
    ;分配给b的地址
    store float 0x3FFCCCCCC0000000, float* %1, align 4
    ;给b赋值
    %2 = alloca [2 x i32], align 16
    ;分配给a的地址
    %3 = getelementptr inbounds [2 x i32], [2 x i32]* %2, i64 0, i64 0
    store i32 2,i32* %3, align 4
    ;对变量a[0]初始化2
    %4 = load float, float* %1, align 4
    ;取b的值
    %5 = getelementptr inbounds [2 x i32], [2 x i32]* %2, i64 0, i64 1
    %6 = load i32, i32* %5, align 4
    ;取a[0]的值
    %7 = sitofp i32 %6 to float
    ;把a[0]的值提升到float
    %8 = fmul float %4, %7
    ;a[0] * b;
    %9 = fptosi float %8 to i32
    ;把a[0] * b的结果转换为i32
    %10 = getelementptr inbounds [2 x i32], [2 x i32]* %2, i64 0, i64 1
    store i32 %9, i32* %10, align 4
    ;赋结果给a[1]
    %11 = getelementptr inbounds [2 x i32], [2 x i32]* %2, i64 0, i64 1
    %12 = load i32, i32* %11, align 4
    ;取a[1]的值
    ret i32 %12
    ;返回
}

    

define dso_local i32 @main() #0 {
0:
    %1 = alloca i32, align 4                                            ;为返回值分配内存
    %2 = alloca float, align 4                                          ;为float b分配内存
    %3 = alloca [2 x i32], align 4                                      ;为int a[2]分配内存
    store float 0x3FFCCCCCC0000000, float* %2, align 4                  ;b=1.8
    %4 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 0  ;获取a[0]的地址
    store i32 2, i32* %4, align 4                                       ;a[0]=2
    %5 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 1  ;获取a[1]的地址
    store i32 0, i32* %5, align 4                                       ;a[1]=0
    %6 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 0  ;获取a[0]的地址
    %7 = load i32, i32* %6, align 4                                     ;读取a[0]的值
    %8 = sitofp i32 %7 to float                                         ;将a[0]的值转换为浮点数类型
    %9 = load float, float* %2, align 4                                 ;读取b的值
    %10 = fmul float %8, %9                                             ;计算a[0]*b
    %11 = fptosi float %10 to i32                                       ;将计算结果转换为整数类型
    %12 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 1 ;获取a[1]的地址
    store i32 %11, i32* %12, align 4                                    ;a[1]=a[0]*b
    %13 = getelementptr inbounds [2 x i32], [2 x i32]* %3, i64 0, i64 1 ;获取a[1]的地址
    %14 = load i32, i32* %13, align 4                                   ;读取a[1]的值
    ret i32 %14                                                         ;return a[1]
}
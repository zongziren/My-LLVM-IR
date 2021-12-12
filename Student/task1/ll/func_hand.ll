define dso_local i32 @add(i32 %0, i32 %1) #0 {  ;add函数的定义
2:
    %3 = alloca i32, align 4                    ;为返回值分配内存
    %4 = alloca i32, align 4                    ;为形参a分配内存
    %5 = alloca i32, align 4                    ;为形参b分配内存
    store i32 %0, i32* %4, align 4              ;将a的值写入内存
    store i32 %1, i32* %5, align 4              ;将b的值写入内存
    %6 = load i32, i32* %4, align 4             ;读取a的值
    %7 = load i32, i32* %5, align 4             ;读取b的值
    %8 = add nsw i32 %6, %7                     ;计算a+b
    %9 = sub nsw i32 %8, 1                      ;计算a+b-1
    store i32 %9, i32* %3, align 4              ;将计算结果写入返回值内存
    br label %10

10:                                 
    %11 = load i32, i32* %3, align 4            ;读取返回值
    ret i32 %11                                 ;return a+b-1
}

define dso_local i32 @main() #0 {
0:
    %1 = alloca i32, align 4                    ;为返回值分配内存
    %2 = alloca i32, align 4                    ;为a分配内存
    %3 = alloca i32, align 4                    ;为b分配内存
    %4 = alloca i32, align 4                    ;为c分配内存
    store i32 0, i32* %1, align 4
    store i32 3, i32* %2, align 4               ;a=3
    store i32 2, i32* %3, align 4               ;b=2
    store i32 5, i32* %4, align 4               ;c=5
    %5 = load i32, i32* %2, align 4             ;读取a的值
    %6 = load i32, i32* %3, align 4             ;读取b的值
    %7 = call i32 @add(i32 %5, i32 %6)          ;调用函数add(a,b)
    %8 = load i32, i32* %4, align 4             ;读取c的值
    %9 = add nsw i32 %7, %8                     ;计算c+add(a,b)
    ret i32 %9                                  ;return c+add(a,b)
}
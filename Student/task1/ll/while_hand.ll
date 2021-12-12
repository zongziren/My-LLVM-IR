@a = dso_local global i32 0, align 4    ;为a分配内存
@b = dso_local global i32 0, align 4    ;为b分配内存
define dso_local i32 @main() #0 {
0:
    %1 = alloca i32, align 4            ;为返回值分配内存
    store i32 0, i32* @b, align 4       ;b=0
    store i32 3, i32* @a, align 4       ;a=3
    br label %2                         ;跳转到while_cond
2:                                      ;while_cond
    %3 = load i32, i32* @a, align 4     ;读取a的值
    %4 = icmp sgt i32 %3, 0             ;返回a>0的布尔值
    br i1 %4, label %5, label %11       ;根据布尔值分支跳转
5:                                      ;while_true
    %6 = load i32, i32* @b, align 4     ;读取b的值
    %7 = load i32, i32* @a, align 4     ;读取a的值
    %8 = add nsw i32 %6, %7             ;计算b+a
    store i32 %8, i32* @b, align 4      ;b=b+a
    %9 = load i32, i32* @a, align 4     ;读取a的值
    %10 = sub nsw i32 %9, 1             ;计算a-1
    store i32 %10, i32* @a, align 4     ;a=a-1
    br label %2                         ;跳转到while_cond
11:                                     ;while_false
    %12 = load i32, i32* @b, align 4    ;读取b的值
    ret i32 %12                         ;return b
}
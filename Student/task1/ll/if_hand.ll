@a = dso_local global i32 0, align 4    ;为a分配内存

define dso_local i32 @main() #0 {
0:
    %1 = alloca i32, align 4            ;为返回值分配内存
    store i32 10, i32* @a, align 4      ;a=10
    br label %2
2:                                      ;if_cond
    %3 = load i32, i32* @a, align 4     ;读取a的值
    %4 = icmp sgt i32 %3, 0             ;返回a>0的布尔值
    br i1 %4, label %5, label %7        ;根据布尔值分支跳转
5:                                      ;if_true
    %6 = load i32, i32* @a, align 4     ;读取a的值
    ret i32 %6                          ;return a
7:                                      ;if_false
    ret i32 0                           ;return 0
}
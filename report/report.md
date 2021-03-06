# PW6 实验报告

- PB19000362 钟书锐 
- PB19030926 李思逸 
- PB19000165 胡乐翔

## 问题回答

**1-1 请给出while语句对应的LLVM IR的代码布局特点，重点解释其中涉及的几个`br`指令的含义（包含各个参数的含义）**  

**答** 
```assembly
br label %label_while_cond	;进入while语句,跳转到条件表达式的计算
label_while_cond:
	...	;计算条件表达式
	br i1 %x, label %label_while_true, label %label_while_false	;%x为条件表达式的计算结果,为true则跳转到while循环体的执行,为false则跳转到while语句结束位置
label_while_true:
	...	;执行while循环体
	br label %label_while_cond	;while循环体执行结束,跳转到条件表达式的计算
label_while_false:
	...	;while语句结束
```

**1-2 请简述函数调用语句对应的LLVM IR的代码特点**  

**答**
```assembly
%y = call <y_type> @func(<x1_type> %x1, <x2_type> %x2, ...)	;
```
- %y为返回值,<y_type>为返回值类型
- func为函数名
- xi_type为实参类型-
- %xi为实参

**2-1. 请给出`SysYFIR.md`中提到的两种getelementptr用法的区别, 并解释原因:**
  - `%2 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 %0` 
  - `%2 = getelementptr i32, i32* %1, i32 %0`
  - 前者2个参数 i32 0, i32 %0 ，第一个0代表偏移`0*10*4`字节，第二个在此基础上`%0*4`字节
  - 后者1个参数 i32 %0，就直接是偏移`%0*4`字节
  - 例如针对`int a[10];`中访问`a[0]`
    - `[10 x i32]` 指出计算基础类型为指向`[10 x i32]`指针
    - `[10 x i32]* %1` 表示索引开始的指针类型`[10 x i32]`及指针%1
    - 最后计算出的地址为`%1+0*10*4+%0*4`
  - 针对`int *a` 访问 `a[0]`
    - `i32` 指出计算基础类型为指向`i32`指针
    - `i32* %1` 表示索引开始的指针类型`i32*`及指针%1
    - - 最后计算出的地址为`%1+%0*4`
  
**3-1. 在`scope`内单独处理`func`的好处有哪些。**
  - 方便处理行参的作用域
  - 方便方便函数的递归调用
  - 方便处理变量和函数重名

## 实验设计
- 分成几个大的部分
- 1. 声明部分:包括函数声明 局部变量声明 全局变量声明 等等
- 2. stmt:代码块部分，同时需要考虑控制流相关问题
- 3. expr相关，尤其需要注意类型转换，包括各种隐式转换
- 4. 数组相关部分
- 5. 程序调试与修复

## 实验难点及解决方案
- 需要考虑各种情况下的浮点数与整数的隐式转换
- 实参是数组，形参是指针的情况
  - 先`create gep(0,0)`
  - 再放到实参列表里


## 实验总结
- TASK1 和 TASK2 测试没问题
- TASK3 正确pass了所有提供的TEST文件

## 实验反馈
- 无

## 组间交流
- 无

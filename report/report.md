# PW6 实验报告

学号1 姓名1 学号2 姓名2 学号3 姓名3

## 问题回答

**1-1 请给出while语句对应的LLVM IR的代码布局特点，重点解释其中涉及的几个`br`指令的含义（包含各个参数的含义）**  

**答**：  
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

**答**：  
```assembly
%y = call <y_type> @func(<x1_type> %x1, <x2_type> %x2, ...)	;%y为返回值,<y_type>为返回值类型,func为函数名,xi_type为实参类型,%xi为实参
```

## 实验设计

## 实验难点及解决方案

## 实验总结

## 实验反馈

## 组间交流

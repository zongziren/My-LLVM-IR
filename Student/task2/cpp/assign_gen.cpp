#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRStmtBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG                                             // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl; // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFloat::get(num, module) // 得到常数值的表示,方便后面多次用到

int main()
{
    auto module = new Module("SysY code");
    auto builder = new IRStmtBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    Type *FloatType = Type::get_float_type(module);

    // main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    // BasicBlock的名字在生成中无所谓,但是可以方便阅读

    builder->set_insert_point(bb); // 一个BB的开始,将当前插入指令点的位置设在bb

    auto retAlloca = builder->create_alloca(Int32Type); // 在内存中分配返回值的位置
    auto bAlloca = builder->create_alloca(FloatType);   // 在内存中分配参数aa的位置
    auto *arrayType_dp = ArrayType::get(Int32Type, 2);
    auto aAlloca = builder->create_alloca(arrayType_dp); // 在内存中分配参数b的位置

    builder->create_store(CONST_FP(1.8), bAlloca); // b初始化
    auto a0Gep = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(0)});
    builder->create_store(CONST_INT(0), a0Gep); // a[0]初始化

    //a[1] = a[0] * b;
    auto a0Load = builder->create_load(a0Gep);
    auto a0_fp = builder->create_sitofp(a0Load, Int32Type); //a[0]类型转换为float

    auto bload = builder->create_load(bAlloca);
    auto mul = builder->create_fmul(a0_fp, bload); // a[0] * b

    auto mul_int = builder->create_fptosi(mul, FloatType); //结果类型转为int32
    auto a1Gep = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(1)});
    builder->create_store(mul_int, a1Gep); //赋值给a[1]

    auto a1load = builder->create_load(a1Gep);
    builder->create_store(a1load, retAlloca);
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad); //返回

    std::cout << module->print();
    delete module;
    return 0;
}

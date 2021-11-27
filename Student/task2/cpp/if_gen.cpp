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
    auto module = new Module("SysY code"); // module name是什么无关紧要
    auto builder = new IRStmtBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    auto zero_initializer = ConstantZero::get(Int32Type, module);
    auto a = GlobalVariable::create("a", module, Int32Type, false, zero_initializer);

    //
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb); // 一个BB的开始,将当前插入指令点的位置设在bb

    auto retAlloca = builder->create_alloca(Int32Type);
    builder->create_store(CONST_INT(0), retAlloca);

    auto retBB = BasicBlock::create(
        module, "", mainFun); // return分支,提前create,以便true分支可以br

    builder->create_store(CONST_INT(10), a);
    auto aLoad = builder->create_load(a);                     // 将参数a load上来
    auto icmp = builder->create_icmp_gt(aLoad, CONST_INT(0)); // a和0的比较,注意ICMPGT

    auto trueBB = BasicBlock::create(module, "trueBB_if", mainFun);   // true分支
    auto falseBB = BasicBlock::create(module, "falseBB_if", mainFun); // false分支
    builder->create_cond_br(icmp, trueBB, falseBB);                   // 条件BR
    builder->set_insert_point(trueBB);                                // if true; 分支的开始需要SetInsertPoint设置
    aLoad = builder->create_load(a);
    builder->create_store(aLoad, retAlloca);
    builder->create_br(retBB); // br retBB

    builder->set_insert_point(falseBB); // if false
    builder->create_store(CONST_INT(0), retAlloca);
    builder->create_br(retBB);

    builder->set_insert_point(retBB); // ret分支
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);
    std::cout << module->print();
    delete module;
    return 0;
}

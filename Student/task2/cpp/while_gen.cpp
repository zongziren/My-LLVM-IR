#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRStmtBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFloat::get(num, module)

int main(){
    auto module = new Module("SysY code");
    auto builder = new IRStmtBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    Type *FloatType = Type::get_float_type(module);

    //int a; int b;
    auto zero_initializer = ConstantZero::get(Int32Type, module);
    auto a = GlobalVariable::create("a", module, Int32Type, false, zero_initializer);
    auto b = GlobalVariable::create("b", module, Int32Type, false, zero_initializer);

    //main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    auto bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    auto retAlloca = builder->create_alloca(Int32Type);

    //b=0; a=3;
    builder->create_store(CONST_INT(0), b);
    builder->create_store(CONST_INT(3), a);

    //while语句
    auto condBB = BasicBlock::create(module, "condBB_while", mainFun);
    auto trueBB = BasicBlock::create(module, "trueBB_while", mainFun);
    auto falseBB = BasicBlock::create(module, "falseBB_while", mainFun);
    builder->create_br(condBB);

    //cond分支
    builder->set_insert_point(condBB);
    auto aLoad = builder->create_load(a);
    auto icmp = builder->create_icmp_gt(aLoad, CONST_INT(0));
    builder->create_cond_br(icmp, trueBB, falseBB);

    //true分支
    builder->set_insert_point(trueBB);
    auto bLoad = builder->create_load(b);
    aLoad = builder->create_load(a);
    auto add = builder->create_iadd(aLoad, bLoad);
    builder->create_store(add, b);
    aLoad = builder->create_load(a);
    auto sub = builder->create_isub(aLoad, CONST_INT(1));
    builder->create_store(sub, a);
    builder->create_br(condBB);

    //false分支
    builder->set_insert_point(falseBB);
    bLoad = builder->create_load(b);
    builder->create_store(bLoad, retAlloca);
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    std::cout << module->print();
    delete module;
    return 0;
}

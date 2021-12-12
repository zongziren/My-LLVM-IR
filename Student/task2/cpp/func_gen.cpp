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

    //add函数
    std::vector<Type *> Ints(2, Int32Type);
    auto addFunTy = FunctionType::get(Int32Type, Ints);
    auto addFun = Function::create(addFunTy, "add", module);
    auto bb = BasicBlock::create(module, "entry", addFun);
    builder->set_insert_point(bb);
    auto retAlloca = builder->create_alloca(Int32Type);
    auto aAlloca = builder->create_alloca(Int32Type);
    auto bAlloca = builder->create_alloca(Int32Type);
    std::vector<Value *> args;
    for (auto arg = addFun->arg_begin(); arg != addFun->arg_end(); arg++) {
        args.push_back(*arg);
    }
    builder->create_store(args[0], aAlloca);
    builder->create_store(args[1], bAlloca);

    //return (a+b-1);
    auto aLoad = builder->create_load(aAlloca);
    auto bLoad = builder->create_load(bAlloca);
    auto add = builder->create_iadd(aLoad, bLoad);
    auto sub = builder->create_isub(add, CONST_INT(1));
    builder->create_store(sub, retAlloca);
    auto retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    //main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}), "main", module);
    bb = BasicBlock::create(module, "entry", mainFun);
    builder->set_insert_point(bb);
    retAlloca = builder->create_alloca(Int32Type);

    //int a; int b; int c;
    aAlloca = builder->create_alloca(Int32Type);
    bAlloca = builder->create_alloca(Int32Type);
    auto cAlloca = builder->create_alloca(Int32Type);

    //a=3; b=2; c=5;
    builder->create_store(CONST_INT(3), aAlloca);
    builder->create_store(CONST_INT(2), bAlloca);
    builder->create_store(CONST_INT(5), cAlloca);

    //return c + add(a,b);
    aLoad = builder->create_load(aAlloca);
    bLoad = builder->create_load(bAlloca);
    auto call = builder->create_call(addFun, {aLoad, bLoad});
    auto cLoad = builder->create_load(cAlloca);
    add = builder->create_iadd(cLoad, call);
    builder->create_store(add, retAlloca);
    retLoad = builder->create_load(retAlloca);
    builder->create_ret(retLoad);

    std::cout << module->print();
    delete module;
    return 0;
}

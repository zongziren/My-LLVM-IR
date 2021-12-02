#include "IRBuilder.h"

#define CONST_INT(num) ConstantInt::get(num, module.get())
#define CONST_FLOAT(num) ConstantFloat::get(num, module.get())

// You can define global variables and functions here
// to store state

// store temporary value
Value *tmp_val = nullptr;

//数组与多维数组相关
int position;    //存储存放的位置
int array_sizes; //存放数组的长度

//变量相关
std::map<int, Value *> initval;           //用来给非常量赋值
std::vector<Constant *> constant_initval; //用来存储const,CONST在编译时计算

//函数相关
SyntaxTree::FuncFParamList func_param_list; //存储函数参数列表
Function *cur_function = nullptr;
BasicBlock *ret_bb;
Value *ret_addr;

// BB基本块相关
std::vector<BasicBlock *> cur_bb; //用栈存储目前所在的代码块

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *FLOAT_T;
Type *INT32PTR_T;
Type *FLOATPTR_T;

void IRBuilder::visit(SyntaxTree::Assembly &node)
{
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());
    for (const auto &def : node.global_defs)
    {
        def->accept(*this);
    }
}

// You need to fill them

void IRBuilder::visit(SyntaxTree::InitVal &node)
{
    if (node.isExp)
    //非数组的初始值
    //或者作为数组初值的最底层,目前只考虑一维数组
    {
        node.expr->accept(*this);
        initval[position] = tmp_val;
        constant_initval.push_back(dynamic_cast<Constant *>(tmp_val));
    }
    else
    //数组初始化,目前只考虑一维数组
    {
        for (auto arry_initial : node.elementList)
        {
            arry_initial->accept(*this);
            position++;
        }
        //赋值长度与数组长度不相等时,例如int a[3]={1,2}
        //在后面的赋值补0
        while (position < array_sizes)
        {
            constant_initval.push_back(CONST_INT(0));
            position++;
        }
    }
}

void IRBuilder::visit(SyntaxTree::FuncDef &node)
{
    //返回类型
    Type *ret_type;
    if (node.ret_type == SyntaxTree::Type::INT)
        ret_type = INT32_T;
    else if (node.ret_type == SyntaxTree::Type::FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    //函数参数类型
    std::vector<Type *> param_types;
    node.param_list->accept(*this);
    for (auto param : func_param_list.params)
    {
        if (param->param_type == SyntaxTree::Type::INT)
        {
            if (param->array_index.empty())
            {
                param_types.push_back(INT32_T);
            }
            else
            {
                param_types.push_back(INT32PTR_T);
            }
        }
        else if (param->param_type == SyntaxTree::Type::FLOAT)
        {
            if (param->array_index.empty())
            {
                param_types.push_back(FLOAT_T);
            }
            else
            {
                param_types.push_back(FLOATPTR_T);
            }
        }
    }

    //创造函数
    auto ty = FunctionType::get(ret_type, param_types);
    auto function = Function::create(ty, node.name, module.get());
    scope.push(node.name, function);
    cur_function = function;

    //函数体处理
    auto funBB = BasicBlock::create(module.get(), node.name, function);
    builder->set_insert_point(funBB); //创造函数基本块
    cur_bb.push_back(funBB);          //代码块入栈
    scope.enter();                    //进入新的作用域

    //函数形参在作用域分配空间
    auto argnum = function->get_num_of_args();
    std::vector<Value *> args; //存储形参初值
    for (auto i : function->get_args())
        args.push_back(i);
    int i = 0;
    for (auto parm : func_param_list.params)
    {
        if (parm->array_index.empty())
        //非数组
        {
            Value *alloc;
            if (parm->param_type == SyntaxTree::Type::INT)
            // int
            {
                alloc = builder->create_alloca(INT32_T);
                builder->create_store(args[i], alloc);
                scope.push(parm->name, alloc);
            }
            else if (parm->param_type == SyntaxTree::Type::FLOAT)
            // float
            {
                alloc = builder->create_alloca(FLOAT_T);
                builder->create_store(args[i], alloc);
                scope.push(parm->name, alloc);
            }
        }
        else
        //数组
        //目前只考虑一维数组
        {
            Value *alloc2;
            if (parm->param_type == SyntaxTree::Type::INT)
            // int arry
            {
                alloc2 = builder->create_alloca(INT32PTR_T);
                builder->create_store(args[i], alloc2);
                scope.push(parm->name, alloc2);
            }
            if (parm->param_type == SyntaxTree::Type::FLOAT)
            // float arry
            {
                alloc2 = builder->create_alloca(FLOATPTR_T);
                builder->create_store(args[i], alloc2);
                scope.push(parm->name, alloc2);
            }
        }
        i++;
    }

    //为返回值分配空间
    if (ret_type == INT32_T)
        ret_addr = builder->create_alloca(INT32_T);
    else if (ret_type == FLOAT_T)
        ret_addr = builder->create_alloca(FLOAT_T);
    ret_bb = BasicBlock::create(module.get(), "return", function);

    // visit函数体
    node.body->accept(*this);

    //缺少return语句时，添加跳转到return的指令，并且默认返回0
    //最后一条指令不是终止指令
    if (builder->get_insert_block()->get_terminator() == nullptr)
    {
        if (function->get_return_type()->is_void_type())
        {
            builder->create_br(ret_bb);
        }
        else if (function->get_return_type()->is_float_type())
        {
            builder->create_store(CONST_FLOAT(0), ret_addr);
            builder->create_br(ret_bb);
        }
        else
        {
            builder->create_store(CONST_INT(0), ret_addr);
            builder->create_br(ret_bb);
        }
    }

    //退出基本快
    scope.exit();
    cur_bb.pop_back();

    //创建return语句
    builder->set_insert_point(ret_bb);
    if (function->get_return_type() == VOID_T)
    {
        builder->create_void_ret();
    }
    else
    {
        auto ret_val = builder->create_load(ret_addr);
        builder->create_ret(ret_val);
    }
}

void IRBuilder::visit(SyntaxTree::FuncFParamList &node)
{
    func_param_list.params.clear();
    for (auto param : node.params)
    {
        func_param_list.params.push_back(param);
    }
}

void IRBuilder::visit(SyntaxTree::FuncParam &node)
{
}

void IRBuilder::visit(SyntaxTree::VarDef &node)
{
    if (node.is_constant)
    //对constant的声明
    // const需要在编译时直接计算
    {
        Value *var;
        if (node.array_length.empty())
        //非数组情况
        {
            //在对应的scope建立值和变量的对应关系
            if (node.btype == SyntaxTree::Type::INT)
            // int
            {
                node.initializers->accept(*this);
                auto initi_value = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
                var = ConstantInt::get(initi_value, module.get());
                scope.push(node.name, var);
            }
            else if (node.btype == SyntaxTree::Type::FLOAT)
            // float
            {
                node.initializers->accept(*this);
                auto initi_value = dynamic_cast<ConstantFloat *>(tmp_val)->get_value();
                var = ConstantFloat::get(initi_value, module.get());
                scope.push(node.name, var);
            }
        }
        else
        //数组情况
        {
        }
    }
    else
    //对非constant的声明
    {
        Value *var;
        if (node.array_length.empty())
        //非数组
        {
            if (node.btype == SyntaxTree::Type::INT)
            // int
            {
                if (scope.in_global())
                //全局变量声明
                {
                    if (node.is_inited)
                    //已经初始化
                    {
                        node.initializers->accept(*this);
                        auto initi_value = dynamic_cast<ConstantInt *>(tmp_val);
                        var = GlobalVariable::create(node.name, module.get(), INT32_T, false, initi_value);
                        scope.push(node.name, var);
                    }
                    else
                    //默认初始化为0
                    {
                        auto initi_value = ConstantZero::get(INT32_T, module.get());
                        var = GlobalVariable::create(node.name, module.get(), INT32_T, false, initi_value);
                        scope.push(node.name, var);
                    }
                }
                else
                //非全局变量
                {
                    var = builder->create_alloca(INT32_T);
                    if (node.is_inited)
                    {
                        node.initializers->accept(*this);
                        builder->create_store(tmp_val, var);
                    }
                    scope.push(node.name, var);
                }
            }
            else if (node.btype == SyntaxTree::Type::FLOAT)
            // float
            {
                if (scope.in_global())
                //全局变量声明
                {
                    if (node.is_inited)
                    //已经初始化
                    {
                        node.initializers->accept(*this);
                        auto initi_value = dynamic_cast<ConstantFloat *>(tmp_val);
                        var = GlobalVariable::create(node.name, module.get(), FLOAT_T, false, initi_value);
                        scope.push(node.name, var);
                    }
                    else
                    //默认初始化为0
                    {
                        auto initi_value = ConstantZero::get(FLOAT_T, module.get());
                        var = GlobalVariable::create(node.name, module.get(), FLOAT_T, false, initi_value);
                        scope.push(node.name, var);
                    }
                }
                else
                //非全局变量
                {
                    var = builder->create_alloca(FLOAT_T);
                    if (node.is_inited)
                    {
                        node.initializers->accept(*this);
                        builder->create_store(tmp_val, var);
                    }
                    scope.push(node.name, var);
                }
            }
        }
        else
        {
        }
    }
}

void IRBuilder::visit(SyntaxTree::LVal &node) {}

void IRBuilder::visit(SyntaxTree::AssignStmt &node) {}

void IRBuilder::visit(SyntaxTree::Literal &node) {}

void IRBuilder::visit(SyntaxTree::ReturnStmt &node) {}

void IRBuilder::visit(SyntaxTree::BlockStmt &node) {}

void IRBuilder::visit(SyntaxTree::EmptyStmt &node) {}

void IRBuilder::visit(SyntaxTree::ExprStmt &node) {}

void IRBuilder::visit(SyntaxTree::UnaryCondExpr &node) {}

void IRBuilder::visit(SyntaxTree::BinaryCondExpr &node) {}

void IRBuilder::visit(SyntaxTree::BinaryExpr &node) {}

void IRBuilder::visit(SyntaxTree::UnaryExpr &node) {}

void IRBuilder::visit(SyntaxTree::FuncCallStmt &node) {}

void IRBuilder::visit(SyntaxTree::IfStmt &node) {}

void IRBuilder::visit(SyntaxTree::WhileStmt &node) {}

void IRBuilder::visit(SyntaxTree::BreakStmt &node) {}

void IRBuilder::visit(SyntaxTree::ContinueStmt &node) {}

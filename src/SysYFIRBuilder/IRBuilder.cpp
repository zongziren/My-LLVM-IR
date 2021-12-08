#include "IRBuilder.h"

#define CONST_INT(num) ConstantInt::get(num, module.get())
#define CONST_FLOAT(num) ConstantFloat::get(num, module.get())

// You can define global variables and functions here
// to store state
struct true_false_BB
{
  BasicBlock *trueBB = nullptr;
  BasicBlock *falseBB = nullptr;
};
std::list<true_false_BB> IF_While_And_Cond_Stack; // used for Cond
std::list<true_false_BB> IF_While_Or_Cond_Stack;  // used for Cond
std::list<true_false_BB> While_Stack;             // used for break and continue

//用来处理变量定义时，浮点数和整数转化的问题
// bool is_in_def = false;
// SyntaxTree::Type DefType;

// whether require lvalue
bool require_lvalue = false;
// detect scope pre-enter (for elegance only)
// bool pre_enter_scope = false;

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
      initval[position] = CONST_INT(0);
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
  // is_in_def = true;
  // DefType = node.btype;
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
        if (dynamic_cast<ConstantInt *>(tmp_val))
        {
          var = ConstantInt::get(dynamic_cast<ConstantInt *>(tmp_val)->get_value(), module.get());
        }
        else
        {
          var = ConstantInt::get((int)(dynamic_cast<ConstantFloat *>(tmp_val)->get_value()), module.get());
        }
        scope.push(node.name, var);
      }
      else if (node.btype == SyntaxTree::Type::FLOAT)
      // float
      {
        node.initializers->accept(*this);
        if (dynamic_cast<ConstantFloat *>(tmp_val))
        {
          var = ConstantFloat::get(dynamic_cast<ConstantFloat *>(tmp_val)->get_value(), module.get());
        }
        else
        {
          var = ConstantFloat::get((float)(dynamic_cast<ConstantInt *>(tmp_val)->get_value()), module.get());
        }
        scope.push(node.name, var);
      }
    }
    else
    //数组情况
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
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            position = 0;
            node.initializers->accept(*this);
            auto *arrayType_t = ArrayType::get(INT32_T, array_sizes);
            std::vector<Constant *> initi_value_t;
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              if (dynamic_cast<ConstantFloat *>(tmp_val))
                initi_value_t.push_back(CONST_INT(static_cast<int>(dynamic_cast<ConstantFloat *>(tmp_val)->get_value())));
              else
                initi_value_t.push_back(dynamic_cast<ConstantInt *>(tmp_val));
            }
            auto initi_value = ConstantArray::get(arrayType_t, initi_value_t);
            auto var = GlobalVariable::create(node.name, module.get(), arrayType_t, false, initi_value);
            scope.push(node.name, var);
          }
          else
          //默认初始化为0
          {
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            auto *arrayType = ArrayType::get(INT32_T, array_sizes);
            auto initi_value = ConstantZero::get(INT32_T, module.get());
            var = GlobalVariable::create(node.name, module.get(), arrayType, false, initi_value);
            scope.push(node.name, var);
          }
        }
        else
        //非全局变量
        {
          for (auto length : node.array_length)
          {
            position = 0;
            length->accept(*this);
          }
          array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
          auto *arrayType = ArrayType::get(INT32_T, array_sizes);
          var = builder->create_alloca(arrayType);
          if (node.is_inited)
          {
            position = 0;
            node.initializers->accept(*this);
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              auto Gep = builder->create_gep(var, {CONST_INT(0), CONST_INT(i)});
              if (dynamic_cast<ConstantFloat *>(tmp_val))
                tmp_val = CONST_INT(static_cast<int>(dynamic_cast<ConstantFloat *>(tmp_val)->get_value()));
              builder->create_store(tmp_val, Gep);
            }
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
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            node.initializers->accept(*this);
            auto *arrayType = ArrayType::get(FLOAT_T, array_sizes);
            std::vector<Constant *> initi_value_t;
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              if (dynamic_cast<ConstantInt *>(tmp_val))
                tmp_val = CONST_FLOAT(static_cast<float>(dynamic_cast<ConstantInt *>(tmp_val)->get_value()));
              initi_value_t.push_back(dynamic_cast<ConstantFloat *>(tmp_val));
            }
            auto initi_value = ConstantArray::get(arrayType, initi_value_t);
            var = GlobalVariable::create(node.name, module.get(), arrayType, false, initi_value);
            scope.push(node.name, var);
          }
          else
          //默认初始化为0
          {
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            auto *arrayType = ArrayType::get(FLOAT_T, array_sizes);
            auto initi_value = ConstantZero::get(FLOAT_T, module.get());
            var = GlobalVariable::create(node.name, module.get(), arrayType, false, initi_value);
            scope.push(node.name, var);
          }
        }
        else
        //非全局变量
        {
          for (auto length : node.array_length)
          {
            position = 0;
            length->accept(*this);
          }
          array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
          auto *arrayType = ArrayType::get(FLOAT_T, array_sizes);
          var = builder->create_alloca(arrayType);
          if (node.is_inited)
          {
            position = 0;
            node.initializers->accept(*this);
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              auto Gep = builder->create_gep(var, {CONST_INT(0), CONST_INT(i)});
              if (dynamic_cast<ConstantInt *>(tmp_val))
                tmp_val = CONST_FLOAT(static_cast<float>(dynamic_cast<ConstantInt *>(tmp_val)->get_value()));
              builder->create_store(tmp_val, Gep);
            }
          }
          scope.push(node.name, var);
        }
      }
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
            if (dynamic_cast<ConstantInt *>(tmp_val))
              var = GlobalVariable::create(node.name, module.get(), INT32_T, false, dynamic_cast<ConstantInt *>(tmp_val));
            else
              var = GlobalVariable::create(node.name, module.get(), INT32_T, false, CONST_INT(static_cast<int>(dynamic_cast<ConstantFloat *>(tmp_val)->get_value())));
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
            if (tmp_val->get_type()->is_float_type())
              tmp_val = builder->create_fptosi(tmp_val, INT32_T);

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

            if (!dynamic_cast<ConstantInt *>(tmp_val))
              std::cout << node.name << " "
                        << "true\n";
            if (dynamic_cast<ConstantFloat *>(tmp_val)) // tmp_val是浮点数
              var = GlobalVariable::create(node.name, module.get(), FLOAT_T, false, dynamic_cast<ConstantFloat *>(tmp_val));

            else
              var = GlobalVariable::create(node.name, module.get(), FLOAT_T, false, CONST_FLOAT(static_cast<float>(dynamic_cast<ConstantInt *>(tmp_val)->get_value())));

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
            if (tmp_val->get_type()->is_integer_type())
              tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            builder->create_store(tmp_val, var);
          }
          scope.push(node.name, var);
        }
      }
    }
    else
    //数组
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
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            position = 0;
            node.initializers->accept(*this);
            auto *arrayType = ArrayType::get(INT32_T, array_sizes);
            std::vector<Constant *> initi_value_t;
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              if (dynamic_cast<ConstantFloat *>(tmp_val))
                tmp_val = CONST_INT(static_cast<int>(dynamic_cast<ConstantFloat *>(tmp_val)->get_value()));
              initi_value_t.push_back(dynamic_cast<ConstantInt *>(tmp_val));
            }
            auto initi_value = ConstantArray::get(arrayType, initi_value_t);
            var = GlobalVariable::create(node.name, module.get(), arrayType, false, initi_value);
            scope.push(node.name, var);
          }
          else
          //默认初始化为0
          {
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            auto *arrayType = ArrayType::get(INT32_T, array_sizes);
            auto initi_value = ConstantZero::get(INT32_T, module.get());
            var = GlobalVariable::create(node.name, module.get(), arrayType, false, initi_value);
            scope.push(node.name, var);
          }
        }
        else
        //非全局变量
        {
          for (auto length : node.array_length)
          {
            position = 0;
            length->accept(*this);
          }
          array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
          auto *arrayType = ArrayType::get(INT32_T, array_sizes);
          var = builder->create_alloca(arrayType);
          if (node.is_inited)
          {
            position = 0;
            node.initializers->accept(*this);
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              auto Gep = builder->create_gep(var, {CONST_INT(0), CONST_INT(i)});
              if (dynamic_cast<ConstantFloat *>(tmp_val))
              {
                tmp_val = CONST_INT(static_cast<int>(dynamic_cast<ConstantFloat *>(tmp_val)->get_value()));
              }
              else if (tmp_val->get_type()->is_float_type())
              {
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
              }
              builder->create_store(tmp_val, Gep);
            }
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
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            node.initializers->accept(*this);
            auto *arrayType = ArrayType::get(FLOAT_T, array_sizes);
            std::vector<Constant *> initi_value_t;
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              if (dynamic_cast<ConstantInt *>(tmp_val))
                tmp_val = CONST_FLOAT(static_cast<float>(dynamic_cast<ConstantInt *>(tmp_val)->get_value()));
              initi_value_t.push_back(dynamic_cast<ConstantFloat *>(tmp_val));
            }
            auto initi_value = ConstantArray::get(arrayType, initi_value_t);
            var = GlobalVariable::create(node.name, module.get(), arrayType, false, initi_value);
            scope.push(node.name, var);
          }
          else
          //默认初始化为0
          {
            for (auto length : node.array_length)
            {
              position = 0;
              length->accept(*this);
            }
            array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
            auto *arrayType = ArrayType::get(FLOAT_T, array_sizes);
            auto initi_value = ConstantZero::get(FLOAT_T, module.get());
            var = GlobalVariable::create(node.name, module.get(), arrayType, false, initi_value);
            scope.push(node.name, var);
          }
        }
        else
        //非全局变量
        {
          for (auto length : node.array_length)
          {
            position = 0;
            length->accept(*this);
          }
          array_sizes = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
          auto *arrayType = ArrayType::get(FLOAT_T, array_sizes);
          var = builder->create_alloca(arrayType);
          if (node.is_inited)
          {
            position = 0;
            node.initializers->accept(*this);
            for (int i = 0; i < array_sizes; i++)
            {
              tmp_val = initval.find(i)->second;
              auto Gep = builder->create_gep(var, {CONST_INT(0), CONST_INT(i)});
              if (dynamic_cast<ConstantInt *>(tmp_val))
              {
                tmp_val = CONST_FLOAT(static_cast<float>(dynamic_cast<ConstantInt *>(tmp_val)->get_value()));
              }
              else if (tmp_val->get_type()->is_integer_type())
              {
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
              }
              builder->create_store(tmp_val, Gep);
            }
          }
          scope.push(node.name, var);
        }
      }
    }
  }
  // is_in_def = false;
}

// void IRBuilder::visit(SyntaxTree::LVal &node)
// {
//     auto var = scope.find(node.name, 0);
//     if (node.array_index.empty())
//     //非数组
//     {
//         if (var->get_type()->get_pointer_element_type()->is_integer_type())
//         {
//             auto val_const = dynamic_cast<ConstantInt *>(var);
//             if (val_const != nullptr)
//             {
//                 tmp_val = val_const;
//             }
//             else
//             {
//                 tmp_val = var;
//             }
//         }
//         else if (var->get_type()->get_pointer_element_type()->is_float_type())
//         {
//             auto val_const = dynamic_cast<ConstantFloat *>(var);
//             if (val_const != nullptr)
//             {
//                 tmp_val = val_const;
//             }
//             else
//             {
//                 tmp_val = var;
//             }
//         }
//     }
//     else
//     //数组
//     {
//         for (auto index : node.array_index)
//         {
//             index->accept(*this);
//         }
//         auto Gep = builder->create_gep(var, {CONST_INT(0), dynamic_cast<ConstantInt *>(tmp_val)});
//         var = Gep;
//         if (var->get_type()->get_pointer_element_type()->is_integer_type())
//         {
//             auto val_const = dynamic_cast<ConstantInt *>(var);
//             if (val_const != nullptr)
//             {
//                 tmp_val = val_const;
//             }
//             else
//             {
//                 tmp_val = var;
//             }
//         }
//         else if (var->get_type()->get_array_element_type()->is_float_type())
//         {
//             auto val_const = dynamic_cast<ConstantFloat *>(var);
//             if (val_const != nullptr)
//             {
//                 tmp_val = val_const;
//             }
//             else
//             {
//                 tmp_val = var;
//             }
//         }
//     }
// }

void IRBuilder::visit(SyntaxTree::LVal &node)
{
  auto var = scope.find(node.name, false);

  bool should_return_lvalue = require_lvalue;
  require_lvalue = false;
  if (node.array_index.empty())
  {
    if (dynamic_cast<ConstantInt *>(var))
    {
      tmp_val = dynamic_cast<ConstantInt *>(var);
    }
    else if (dynamic_cast<ConstantFloat *>(var))
    {
      tmp_val = dynamic_cast<ConstantFloat *>(var);
    }
    else
    {
      if (should_return_lvalue)
      {
        if (var->get_type()->get_pointer_element_type()->is_pointer_type())
        {
          tmp_val = builder->create_load(var);
        }
        else if (var->get_type()->get_pointer_element_type()->is_array_type())
        {
          tmp_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)});
        }
        else
        {
          tmp_val = var;
        }
        require_lvalue = false;
      }
      else
      {
        if (var->get_type()->get_pointer_element_type()->is_integer_type())
        {
          auto val_const = dynamic_cast<ConstantInt *>(var);
          if (val_const != nullptr)
          {
            tmp_val = val_const;
          }
          else
          {
            tmp_val = builder->create_load(var);
          }
        }
        else
        {
          auto val_const = dynamic_cast<ConstantFloat *>(var);
          if (val_const != nullptr)
          {
            tmp_val = val_const;
          }
          else
          {
            tmp_val = builder->create_load(var);
          }
        }
      }
    }
  }
  else
  {
    for (auto index : node.array_index)
    { //只有一维
      index->accept(*this);
      bool is_const_array = dynamic_cast<GlobalVariable *>(var) &&
                            dynamic_cast<ConstantArray *>(dynamic_cast<GlobalVariable *>(var)->get_init());
      if (is_const_array && dynamic_cast<ConstantInt *>(tmp_val))
      { //是常数数组，并且下标可以直接算出来
        int indexID = dynamic_cast<ConstantInt *>(tmp_val)->get_value();
        tmp_val = dynamic_cast<ConstantArray *>(dynamic_cast<GlobalVariable *>(var)->get_init())->get_element_value(indexID);
      }
      else
      {
        if (dynamic_cast<ConstantInt *>(tmp_val))
        { //下标直接算出来的
          if (var->get_type()->get_pointer_element_type()->is_array_type()){
            var = builder->create_gep(var, {CONST_INT(0), dynamic_cast<ConstantInt *>(tmp_val)});
          }
          else {
            auto var_t = builder->create_load(var);
            var = builder->create_gep(var_t, {dynamic_cast<ConstantInt *>(tmp_val)});
          }
        }
        else
        {
          if (var->get_type()->get_pointer_element_type()->is_array_type())
            var = builder->create_gep(var, {CONST_INT(0), tmp_val});
          else{
            auto var_t = builder->create_load(var);
            var = builder->create_gep(var_t, {tmp_val});
          }
        }
        if (should_return_lvalue)
        {
          if (var->get_type()->get_pointer_element_type()->is_pointer_type())
          {
            tmp_val = builder->create_load(var);
          }
          else
          {
            tmp_val = var;
          }
          require_lvalue = false;
        }
        else
        {
          if (var->get_type()->get_pointer_element_type()->is_integer_type())
          {
            auto val_const = dynamic_cast<ConstantInt *>(var);
            if (val_const != nullptr)
            {
              tmp_val = val_const;
            }
            else
            {
              tmp_val = builder->create_load(var);
            }
          }
          else
          {
            auto val_const = dynamic_cast<ConstantFloat *>(var);
            if (val_const != nullptr)
            {
              tmp_val = val_const;
            }
            else
            {
              tmp_val = builder->create_load(var);
            }
          }
        }
      } // end else (not const array)
    }   // end for
  }
}

void IRBuilder::visit(SyntaxTree::AssignStmt &node)
{
  node.value->accept(*this);
  auto result = tmp_val;
  require_lvalue = true;
  node.target->accept(*this);
  auto addr = tmp_val;
  // if (result->get_type()->is_float_type())
  // {
  //     std::cout << addr->get_name() << ": float\n";
  //     // std::cout << ((addr->get_type()         ) ? "true" : "false") << "\n";
  //     if (addr->get_type()->get_pointer_element_type()->is_integer_type())
  //         std::cout << "true\n";
  //     else
  //         std::cout << "false\n";
  // }
  if (result->get_type()->is_float_type() && addr->get_type()->get_pointer_element_type()->is_integer_type())
  {
    result = builder->create_fptosi(result, INT32_T);
  }
  else if (result->get_type()->is_integer_type() && addr->get_type()->get_pointer_element_type()->is_float_type())
  {
    result = builder->create_sitofp(result, FLOAT_T);
  }
  builder->create_store(result, addr);
  tmp_val = result;
}

void IRBuilder::visit(SyntaxTree::Literal &node)
{
  if (node.literal_type == SyntaxTree::Type::INT)
    tmp_val = CONST_INT(node.int_const);
  else
    tmp_val = CONST_FLOAT(node.float_const);
}

void IRBuilder::visit(SyntaxTree::ReturnStmt &node)
{
  if (node.ret == nullptr)
  {
    builder->create_br(ret_bb);
  }
  else
  {
    node.ret->accept(*this);
    if (ret_addr->get_type()->get_pointer_element_type()->is_integer_type() && tmp_val->get_type()->is_float_type())
      tmp_val = builder->create_fptosi(tmp_val, INT32_T);
    else if (ret_addr->get_type()->get_pointer_element_type()->is_float_type() && tmp_val->get_type()->is_integer_type())
      tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
    builder->create_store(tmp_val, ret_addr);
    builder->create_br(ret_bb);
  }
}

void IRBuilder::visit(SyntaxTree::BlockStmt &node)
{
  // bool need_exit_scope = !pre_enter_scope;
  // if (pre_enter_scope) //函数定义要用的FuncDef
  // {
  //     pre_enter_scope = false;
  // }
  // else
  {
    scope.enter();
  }
  for (auto &decl : node.body)
  {
    decl->accept(*this);
    if (builder->get_insert_block()->get_terminator() != nullptr)
      break; //不等于空，存在br指令，还是有一点疑惑
  }
  // if (need_exit_scope)
  {
    scope.exit();
  }
}

void IRBuilder::visit(SyntaxTree::EmptyStmt &node) { tmp_val = nullptr; }

void IRBuilder::visit(SyntaxTree::ExprStmt &node) { node.exp->accept(*this); }

void IRBuilder::visit(SyntaxTree::UnaryCondExpr &node)
{
  if (node.op == SyntaxTree::UnaryCondOp::NOT)
  {
    node.rhs->accept(*this);
    auto r_val = tmp_val;
    if (dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 32) {
      tmp_val = builder->create_icmp_eq(r_val, CONST_INT(0));
    } 
    else if (dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 1) {
      tmp_val = builder->create_icmp_eq(r_val, CONST_INT(false));      
    }
  }
}

void IRBuilder::visit(SyntaxTree::BinaryCondExpr &node)
{
  CmpInst *cond_val;
  FCmpInst *fcond_val;
  bool is_float;
  bool is_int;
  bool is_literal;
  if (node.op == SyntaxTree::BinaryCondOp::LAND)
  {
    auto trueBB = BasicBlock::create(module.get(), "", cur_function);
    IF_While_And_Cond_Stack.push_back({trueBB, IF_While_Or_Cond_Stack.back().falseBB});
    node.lhs->accept(*this);
    IF_While_And_Cond_Stack.pop_back();
    auto ret_val = tmp_val;
    is_float = ret_val->get_type()->is_float_type();
    is_int = ret_val->get_type()->is_integer_type();

    cond_val = dynamic_cast<CmpInst *>(ret_val);
    fcond_val = dynamic_cast<FCmpInst *>(ret_val);
    is_literal = (cond_val == nullptr && fcond_val == nullptr);
    if (is_literal) //是一个literal 类型
    {
      if (is_float) //浮点数类型
      {
        fcond_val = builder->create_fcmp_ne(tmp_val, CONST_FLOAT(0));
        // builder->create_cond_br(fcond_val, trueBB, IF_While_Or_Cond_Stack.back().falseBB);
      }
      else //整数类型
      {
        cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
        // builder->create_cond_br(cond_val, trueBB, IF_While_Or_Cond_Stack.back().falseBB);
      }
    }
    if ((is_literal && is_float) || fcond_val != nullptr)
      builder->create_cond_br(fcond_val, trueBB, IF_While_Or_Cond_Stack.back().falseBB);
    else //可能有bug
      builder->create_cond_br(cond_val, trueBB, IF_While_Or_Cond_Stack.back().falseBB);
    builder->set_insert_point(trueBB);
    node.rhs->accept(*this);
  }
  else if (node.op == SyntaxTree::BinaryCondOp::LOR)
  {
    auto falseBB = BasicBlock::create(module.get(), "", cur_function);
    IF_While_Or_Cond_Stack.push_back({IF_While_Or_Cond_Stack.back().trueBB, falseBB});
    node.lhs->accept(*this);
    IF_While_Or_Cond_Stack.pop_back();
    auto ret_val = tmp_val;
    is_float = ret_val->get_type()->is_float_type();
    is_int = ret_val->get_type()->is_integer_type();

    cond_val = dynamic_cast<CmpInst *>(ret_val);
    fcond_val = dynamic_cast<FCmpInst *>(ret_val);
    is_literal = (cond_val == nullptr && fcond_val == nullptr);
    if (is_literal) //是一个literal 类型
    {
      if (is_float) //浮点数类型
      {
        fcond_val = builder->create_fcmp_ne(tmp_val, CONST_FLOAT(0));
        // builder->create_cond_br(fcond_val, trueBB, IF_While_Or_Cond_Stack.back().falseBB);
      }
      else //整数类型
      {
        cond_val = builder->create_icmp_ne(tmp_val, CONST_INT(0));
        // builder->create_cond_br(cond_val, trueBB, IF_While_Or_Cond_Stack.back().falseBB);
      }
    }

    if ((is_literal && is_float) || fcond_val != nullptr)
      builder->create_cond_br(fcond_val, IF_While_Or_Cond_Stack.back().trueBB, falseBB);
    else
      builder->create_cond_br(cond_val, IF_While_Or_Cond_Stack.back().trueBB, falseBB);
    builder->set_insert_point(falseBB);
    node.rhs->accept(*this);
  }
  else //小于大于关系，这里只考虑lval和rval为literal或者expr，不可能为cond
  {
    node.lhs->accept(*this);
    auto l_val = tmp_val;
    node.rhs->accept(*this);
    auto r_val = tmp_val;
    Value *cmp;
    switch (node.op)
    {
    case SyntaxTree::BinaryCondOp::LT:
      if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 32 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 1)
      {
        r_val = builder->create_zext(r_val, INT32_T);
      }
      else if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 1 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 32)
      {
        l_val = builder->create_zext(l_val, INT32_T);
      }
      cmp = builder->create_icmp_lt(l_val, r_val);
      break;
    case SyntaxTree::BinaryCondOp::LTE:
      if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 32 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 1)
      {
        r_val = builder->create_zext(r_val, INT32_T);
      }
      else if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 1 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 32)
      {
        l_val = builder->create_zext(l_val, INT32_T);
      }
      cmp = builder->create_icmp_le(l_val, r_val);
      break;
    case SyntaxTree::BinaryCondOp::GTE:
      if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 32 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 1)
      {
        r_val = builder->create_zext(r_val, INT32_T);
      }
      else if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 1 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 32)
      {
        l_val = builder->create_zext(l_val, INT32_T);
      }
      cmp = builder->create_icmp_ge(l_val, r_val);
      break;
    case SyntaxTree::BinaryCondOp::GT:
      if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 32 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 1)
      {
        r_val = builder->create_zext(r_val, INT32_T);
      }
      else if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 1 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 32)
      {
        l_val = builder->create_zext(l_val, INT32_T);
      }
      cmp = builder->create_icmp_gt(l_val, r_val);
      break;
    case SyntaxTree::BinaryCondOp::EQ:
      if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 32 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 1)
      {
        r_val = builder->create_zext(r_val, INT32_T);
      }
      else if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 1 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 32)
      {
        l_val = builder->create_zext(l_val, INT32_T);
      }
      cmp = builder->create_icmp_eq(l_val, r_val);
      break;
    case SyntaxTree::BinaryCondOp::NEQ:
      if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 32 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 1)
      {
        r_val = builder->create_zext(r_val, INT32_T);
      }
      else if (dynamic_cast<IntegerType *>(l_val->get_type())->get_num_bits() == 1 &&
       dynamic_cast<IntegerType *>(r_val->get_type())->get_num_bits() == 32)
      {
        l_val = builder->create_zext(l_val, INT32_T);
      }
      cmp = builder->create_icmp_ne(l_val, r_val);
      break;
    default:
      break;
    }
    tmp_val = cmp;
  }
}

void IRBuilder::visit(SyntaxTree::BinaryExpr &node)
{
  if (node.rhs == nullptr)
  {
    node.lhs->accept(*this);
  } //出错的情况（猜测）
  else
  {
    bool is_lint, is_rint;
    node.lhs->accept(*this);
    auto l_val_const_int = dynamic_cast<ConstantInt *>(tmp_val);
    auto l_val_const_float = dynamic_cast<ConstantFloat *>(tmp_val);
    auto l_val = tmp_val;

    node.rhs->accept(*this);
    auto r_val_const_int = dynamic_cast<ConstantInt *>(tmp_val);
    auto r_val_const_float = dynamic_cast<ConstantFloat *>(tmp_val);
    auto r_val = tmp_val;

    is_lint = l_val->get_type()->is_integer_type();
    is_rint = r_val->get_type()->is_integer_type();
    switch (node.op)
    {
    case SyntaxTree::BinOp::PLUS:
      if (r_val_const_int != nullptr && l_val_const_int != nullptr)
      {
        tmp_val = CONST_INT(l_val_const_int->get_value() + r_val_const_int->get_value());
      } //整数常数先计算
      else if (r_val_const_float != nullptr && l_val_const_float != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() +
                              r_val_const_float->get_value());
      } //浮点数常数先计算
      else if (l_val_const_int != nullptr && r_val_const_float != nullptr)
      { //左int常，右float常
        tmp_val = CONST_FLOAT(l_val_const_int->get_value() + r_val_const_float->get_value());
      }
      else if (l_val_const_float != nullptr && r_val_const_int != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() + r_val_const_int->get_value());
      }
      else
      {
        if (is_lint && is_rint) //全是整数
        {
          tmp_val = builder->create_iadd(l_val, r_val);
        }
        else
        {
          if (!is_lint && !is_rint) //全是浮点数
          {
            tmp_val = builder->create_fadd(l_val, r_val);
          }
          else
          {
            if (!is_lint) // lhs是浮点数，转换rhs
              r_val = builder->create_sitofp(r_val, l_val->get_type());
            else //反过来
            {
              l_val = builder->create_sitofp(l_val, r_val->get_type());
            }
            tmp_val = builder->create_fadd(l_val, r_val);
          }
        }
      }
      break;
    case SyntaxTree::BinOp::MINUS:
      if (r_val_const_int != nullptr && l_val_const_int != nullptr)
      {
        tmp_val = CONST_INT(l_val_const_int->get_value() - r_val_const_int->get_value());
      }
      else if (r_val_const_float != nullptr && l_val_const_float != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() -
                              r_val_const_float->get_value());
      } //浮点数常数先计算
      else if (l_val_const_int != nullptr && r_val_const_float != nullptr)
      { //左int常，右float常
        tmp_val = CONST_FLOAT(l_val_const_int->get_value() - r_val_const_float->get_value());
      }
      else if (l_val_const_float != nullptr && r_val_const_int != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() - r_val_const_int->get_value());
      }

      else
      {
        if (is_lint && is_rint) //全是整数
        {
          tmp_val = builder->create_isub(l_val, r_val);
        }
        else
        {
          if (!is_lint && !is_rint) //全是浮点数
          {
            tmp_val = builder->create_fsub(l_val, r_val);
          }
          else
          {
            if (!is_lint) // lhs是浮点数，转换rhs
              r_val = builder->create_sitofp(r_val, l_val->get_type());
            else //反过来
            {
              l_val = builder->create_sitofp(l_val, r_val->get_type());
            }
            tmp_val = builder->create_fsub(l_val, r_val);
          }
        }
      }
      break;
    case SyntaxTree::BinOp::MULTIPLY:
      if (r_val_const_int != nullptr && l_val_const_int != nullptr)
      {
        tmp_val = CONST_INT(l_val_const_int->get_value() * r_val_const_int->get_value());
      }
      else if (r_val_const_float != nullptr && l_val_const_float != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() -
                              r_val_const_float->get_value());
      } //浮点数常数先计算
      else if (l_val_const_int != nullptr && r_val_const_float != nullptr)
      { //左int常，右float常
        tmp_val = CONST_FLOAT(l_val_const_int->get_value() * r_val_const_float->get_value());
      }
      else if (l_val_const_float != nullptr && r_val_const_int != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() * r_val_const_int->get_value());
      }
      else
      {
        if (is_lint && is_rint) //全是整数
        {
          tmp_val = builder->create_imul(l_val, r_val);
        }
        else
        {
          if (!is_lint && !is_rint) //全是浮点数
          {
            tmp_val = builder->create_fmul(l_val, r_val);
          }
          else
          {
            if (!is_lint) // lhs是浮点数，转换rhs
              r_val = builder->create_sitofp(r_val, l_val->get_type());
            else //反过来
            {
              l_val = builder->create_sitofp(l_val, r_val->get_type());
            }
            tmp_val = builder->create_fmul(l_val, r_val);
          }
        }
      }
      break;
    case SyntaxTree::BinOp::DIVIDE:
      if (r_val_const_int != nullptr && l_val_const_int != nullptr)
      {
        tmp_val = CONST_INT(l_val_const_int->get_value() / r_val_const_int->get_value());
      }
      else if (r_val_const_float != nullptr && l_val_const_float != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() -
                              r_val_const_float->get_value());
      } //浮点数常数先计算
      else if (l_val_const_int != nullptr && r_val_const_float != nullptr)
      { //左int常，右float常
        tmp_val = CONST_FLOAT(l_val_const_int->get_value() / r_val_const_float->get_value());
      }
      else if (l_val_const_float != nullptr && r_val_const_int != nullptr)
      {
        tmp_val = CONST_FLOAT(l_val_const_float->get_value() / r_val_const_int->get_value());
      }

      else
      {
        if (is_lint && is_rint) //全是整数
        {
          tmp_val = builder->create_isdiv(l_val, r_val);
        }
        else
        {
          if (!is_lint && !is_rint) //全是浮点数
          {
            tmp_val = builder->create_fdiv(l_val, r_val);
          }
          else
          {
            if (!is_lint) // lhs是浮点数，转换rhs
              r_val = builder->create_sitofp(r_val, l_val->get_type());
            else //反过来
            {
              l_val = builder->create_sitofp(l_val, r_val->get_type());
            }
            tmp_val = builder->create_fdiv(l_val, r_val);
          }
        }
      }
      break;
    case SyntaxTree::BinOp::MODULO:
      if (r_val_const_int != nullptr && l_val_const_int != nullptr)
      {
        tmp_val = CONST_INT(l_val_const_int->get_value() % r_val_const_int->get_value());
      }
      else
      {
        tmp_val = builder->create_isrem(l_val, r_val);
      }
    }
  }
}

void IRBuilder::visit(SyntaxTree::UnaryExpr &node)
{
  node.rhs->accept(*this);
  bool is_int, is_float;
  if (node.op == SyntaxTree::UnaryOp::MINUS)
  {
    auto val_const_int = dynamic_cast<ConstantInt *>(tmp_val);     // ConstInt
    auto val_const_float = dynamic_cast<ConstantFloat *>(tmp_val); // ConstFloat;
    if (tmp_val->get_type()->is_integer_type()
     && dynamic_cast<IntegerType *>(tmp_val->get_type())->get_num_bits() == 32)
    //是32位整数
    {
      auto r_val = tmp_val;
      if (val_const_int != nullptr)
      {
        tmp_val = CONST_INT(0 - val_const_int->get_value());
      } //是常数
      else
      {
        tmp_val = builder->create_isub(CONST_INT(0), r_val);
      } //非常数
    }
    else if (tmp_val->get_type()->is_integer_type()
     && dynamic_cast<IntegerType *>(tmp_val->get_type())->get_num_bits() == 1)
    //是1位整数
    {
      tmp_val = builder->create_zext(tmp_val, INT32_T);
      auto r_val = tmp_val;
      if (val_const_int != nullptr)
      {
        tmp_val = CONST_INT(0 - val_const_int->get_value());
      } //是常数
      else
      {
        tmp_val = builder->create_isub(CONST_INT(0), r_val);
      } //非常数
    }
    else if (tmp_val->get_type()->is_float_type())
    {
      auto r_val = tmp_val;
      if (val_const_float != nullptr)
      {
        tmp_val = CONST_FLOAT(0 - val_const_float->get_value());
      } //是常数
      else
      {
        tmp_val = builder->create_fsub(CONST_FLOAT(0), r_val);
      } //非常数
    }
  } //
}

void IRBuilder::visit(SyntaxTree::FuncCallStmt &node)
{
  auto fun = static_cast<Function *>(scope.find(node.name, true));

  std::vector<Value *> params;
  int i = 0;
  if (node.name == "starttime" || node.name == "stoptime")
  {
    params.push_back(ConstantInt::get(node.loc.begin.line, module.get()));
  } // what?
  else
  {
    for (auto &param : node.params)
    {
      auto param_type = fun->get_function_type()->get_param_type(i++);
      if (param_type->is_integer_type() || param_type->is_float_type())
      {
        require_lvalue = false;
      } // TBC
      else
      {
        require_lvalue = true;
      } // TBC
      param->accept(*this);
      require_lvalue = false;

      if (tmp_val->get_type()->is_float_type() && param_type->is_integer_type())
      {
        tmp_val = builder->create_fptosi(tmp_val, INT32_T);
      }
      if (tmp_val->get_type()->is_integer_type() && param_type->is_float_type())
      {
        tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
      }
      params.push_back(tmp_val);
    }
  }
  tmp_val = builder->create_call(static_cast<Function *>(fun), params);
}

void IRBuilder::visit(SyntaxTree::IfStmt &node)
{
  auto trueBB = BasicBlock::create(module.get(), "", cur_function);
  auto falseBB = BasicBlock::create(module.get(), "", cur_function);
  auto nextBB = BasicBlock::create(module.get(), "", cur_function);
  IF_While_Or_Cond_Stack.push_back({nullptr, nullptr});
  IF_While_Or_Cond_Stack.back().trueBB = trueBB;
  if (node.else_statement == nullptr)
  {
    IF_While_Or_Cond_Stack.back().falseBB = nextBB;
  } //无else，
  else
  {
    IF_While_Or_Cond_Stack.back().falseBB = falseBB;
  } //有else
  node.cond_exp->accept(*this);
  IF_While_Or_Cond_Stack.pop_back();
  auto ret_val = tmp_val;
  auto *cond_val_int = dynamic_cast<CmpInst *>(ret_val);
  auto *cond_val_float = dynamic_cast<FCmpInst *>(ret_val);
  if (cond_val_int == nullptr && cond_val_float == nullptr)
  {
    if (ret_val->get_type()->is_integer_type()
     && dynamic_cast<IntegerType *>(tmp_val->get_type())->get_num_bits() == 32)
      cond_val_int = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    else if (ret_val->get_type()->is_integer_type()
     && dynamic_cast<IntegerType *>(tmp_val->get_type())->get_num_bits() == 1)
      cond_val_int = builder->create_icmp_ne(tmp_val, CONST_INT(false));
    else
      cond_val_float = builder->create_fcmp_ne(tmp_val, CONST_FLOAT(0));
  } //同下while
  if (node.else_statement == nullptr)
  {
    if (cond_val_float == nullptr)
      builder->create_cond_br(cond_val_int, trueBB, nextBB);
    else
      builder->create_cond_br(cond_val_float, trueBB, nextBB);
  }
  else
  {
    if (cond_val_float == nullptr)
      builder->create_cond_br(cond_val_int, trueBB, falseBB);
    else
      builder->create_cond_br(cond_val_float, trueBB, falseBB);
  }
  cur_bb.pop_back(); //同样，为什么pop
  builder->set_insert_point(trueBB);
  cur_bb.push_back(trueBB);
  if (dynamic_cast<SyntaxTree::BlockStmt *>(node.if_statement.get()))
  {
    node.if_statement->accept(*this);
  }
  else
  {
    scope.enter();
    node.if_statement->accept(*this);
    scope.exit();
  }

  if (builder->get_insert_block()->get_terminator() == nullptr)
  {
    builder->create_br(nextBB);
  }
  cur_bb.pop_back();

  if (node.else_statement == nullptr)
  {
    falseBB->erase_from_parent();
  }
  else
  {
    builder->set_insert_point(falseBB);
    cur_bb.push_back(falseBB);
    if (dynamic_cast<SyntaxTree::BlockStmt *>(node.else_statement.get()))
    {
      node.else_statement->accept(*this);
    }
    else
    {
      scope.enter();
      node.else_statement->accept(*this);
      scope.exit();
    }
    if (builder->get_insert_block()->get_terminator() == nullptr)
    {
      builder->create_br(nextBB);
    } // else 语句结束，进入nextBB
    cur_bb.pop_back();
  }
  builder->set_insert_point(nextBB);
  cur_bb.push_back(nextBB);
  if (nextBB->get_pre_basic_blocks().size() == 0)
  {
    builder->set_insert_point(trueBB);
    nextBB->erase_from_parent();
  }
}

void IRBuilder::visit(SyntaxTree::WhileStmt &node)
{
  auto whileBB = BasicBlock::create(module.get(), "", cur_function);
  auto trueBB = BasicBlock::create(module.get(), "", cur_function);
  auto nextBB = BasicBlock::create(module.get(), "", cur_function);
  //分别指向条件判断，真进入循环，假跳出循环
  // continue和break要用到While_Stack
  While_Stack.push_back({whileBB, nextBB});
  if (builder->get_insert_block()->get_terminator() == nullptr)
  { //上一条语句不是分支，为什么呢
    builder->create_br(whileBB);
  }
  cur_bb.pop_back();
  //为什么要pop掉cur_bb, 以及这个list是干嘛的？
  builder->set_insert_point(whileBB);
  IF_While_Or_Cond_Stack.push_back({trueBB, nextBB});
  node.cond_exp->accept(*this);
  IF_While_Or_Cond_Stack.pop_back();
  auto ret_val = tmp_val;
  auto *cond_val_int = dynamic_cast<CmpInst *>(ret_val);
  auto *cond_val_float = dynamic_cast<FCmpInst *>(ret_val);
  if (cond_val_int == nullptr && cond_val_float == nullptr)
  {
    if (ret_val->get_type()->is_integer_type())
      cond_val_int = builder->create_icmp_ne(tmp_val, CONST_INT(0));
    else
      cond_val_float = builder->create_fcmp_ne(tmp_val, CONST_FLOAT(0));
  } // cond_val为空表示非逻辑值，此时判断表达式的值是否不等于0，不等于0为真
  if (cond_val_float == nullptr)
    builder->create_cond_br(cond_val_int, trueBB, nextBB); //其余情况就按cond_val来处理
  else
    builder->create_cond_br(cond_val_float, trueBB, nextBB);
  builder->set_insert_point(trueBB);
  cur_bb.push_back(trueBB);
  if (dynamic_cast<SyntaxTree::BlockStmt *>(node.statement.get()))
  {
    node.statement->accept(*this);
  }    //有blockstmt（大括号），由blockstmt处理作用域
  else //无大括号，直接处理作用域
  {
    scope.enter();
    node.statement->accept(*this);
    scope.exit();
  }
  if (builder->get_insert_block()->get_terminator() == nullptr)
  {
    builder->create_br(whileBB);
  } // true 语句结束，返回whileBB进行条件判断
  cur_bb.pop_back();
  builder->set_insert_point(nextBB);
  cur_bb.push_back(nextBB);
  While_Stack.pop_back();
}

void IRBuilder::visit(SyntaxTree::BreakStmt &node)
{
  builder->create_br(While_Stack.back().falseBB);
}

void IRBuilder::visit(SyntaxTree::ContinueStmt &node)
{
  builder->create_br(While_Stack.back().trueBB);
}
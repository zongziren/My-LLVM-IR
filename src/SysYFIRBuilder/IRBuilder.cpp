#include "IRBuilder.h"

#define CONST_INT(num) ConstantInt::get(num, module.get())
#define CONST_FLOAT(num) ConstantFloat::get(num, module.get())

// You can define global variables and functions here
auto t_initializer;
int t_num_elements;
int init_val;
std::vector<Constant *> init_val_array;
std::vector<Type *> funcparam_type;
// to store state
bool is_array;
bool is_initval;

// store temporary value
Value *tmp_val = nullptr;

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *FLOAT_T;
Type *INT32PTR_T;
Type *FLOATPTR_T;

void IRBuilder::visit(SyntaxTree::Assembly &node) {
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());
    for (const auto &def : node.global_defs) {
        def->accept(*this);
    }
}

// You need to fill them

void IRBuilder::visit(SyntaxTree::InitVal &node) {
    if (node.isExp) {
        is_initval = true;
        node.expr->accept(*this);
        is_initval = false;
        t_initializer = CONST_INT(init_val);
    }
    else {
        init_val_array.clear();
        for (auto element : node.elementList) {
            is_initval = true;
            element->accept(*this);
            is_initval = false;
            init_val_array.push_back(CONST_INT(init_val));
        }
    }
}

void IRBuilder::visit(SyntaxTree::FuncDef &node) {
    
}

void IRBuilder::visit(SyntaxTree::FuncFParamList &node) {}

void IRBuilder::visit(SyntaxTree::FuncParam &node) {}

void IRBuilder::visit(SyntaxTree::VarDef &node) {
    if (this->scope.in_global()) {
        //全局变量
        if (node.array_length.empty()) {
            //不是数组
            if (node.is_inited) {
                //已初始化
                is_initval = true;
                node.initializers->accept(*this);
                is_initval = false;
                if (node.btype == SyntaxTree::Type::INT) {
                    auto t = GlobalVariable::create(node.name, this->module.get(), Int32Type, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::FLOAT) {
                    auto t = GlobalVariable::create(node.name, this->module, FloatType, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::BOOL) {
                    auto t = GlobalVariable::create(node.name, this->module, Int1Type, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
            }
            else {
                //未初始化
                if (node.btype == SyntaxTree::Type::INT) {
                    auto zero_initializer = ConstantZero::get(Int32Type, this->module);
                    auto t = GlobalVariable::create(node.name, this->module, Int32Type, false, zero_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::FLOAT) {
                    auto zero_initializer = ConstantZero::get(FloatType, this->module);
                    auto t = GlobalVariable::create(node.name, this->module, FloatType, false, zero_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::BOOL) {
                    auto zero_initializer = ConstantZero::get(Int1Type, this->module);
                    auto t = GlobalVariable::create(node.name, this->module, Int1Type, false, zero_initializer);
                    this->scope.push(node.name,&t);
                }
            }
        }
        else {
            //是数组
            if (node.is_inited) {
                //已初始化
                for (auto length : node.array_length) {
                    is_array = true;
                    length->accept(*this);
                    is_array = false;
                }
                is_initval = true;
                node.initializers->accept(*this);
                is_initval = false;
                if (node.btype == SyntaxTree::Type::INT) {
                    auto *arrayType_t = ArrayType::get(Int32Type, t_num_elements);
                    t_initializer = ConstantArray::get(arrayType_t, init_val_array);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::FLOAT) {
                    auto *arrayType_t = ArrayType::get(FloatType, t_num_elements);
                    t_initializer = ConstantArray::get(arrayType_t, init_val_array);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::BOOL) {
                    auto *arrayType_t = ArrayType::get(Int1Type, t_num_elements);
                    t_initializer = ConstantArray::get(arrayType_t, init_val_array);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
            }
            else {
                //未初始化
                for (auto length : node.array_length) {
                    is_array = true;
                    length->accept(*this);
                    is_array = false;
                }
                if (node.btype == SyntaxTree::Type::INT) {
                    zero_initializer = ConstantZero::get(Int32Type, this->module);
                    auto *arrayType_t = ArrayType::get(Int32Type, t_num_elements);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, zero_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::FLOAT) {
                    zero_initializer = ConstantZero::get(FloatType, this->module);
                    auto *arrayType_t = ArrayType::get(FloatType, t_num_elements);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, zero_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::BOOL) {
                    zero_initializer = ConstantZero::get(Int1Type, this->module);
                    auto *arrayType_t = ArrayType::get(Int1Type, t_num_elements);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, zero_initializer);
                    this->scope.push(node.name,&t);
                }
            }
        }
    }
    else {
        //局部变量
        if (node.array_length.empty()) {
            //不是数组
            if (node.is_inited) {
                //已初始化
            }
            else {
                //未初始化
                if (node.btype == SyntaxTree::Type::INT) {
                    auto t = this->builder->create_alloca(Int32Type);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::FLOAT) {
                    auto t = this->builder->create_alloca(FloatType);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::BOOL) {
                    auto t = this->builder->create_alloca(Int1Type);
                    this->scope.push(node.name,&t);
                }
            }
        }
        else {
            //是数组
            if (node.is_inited) {
                //已初始化
            }
            else {
                //未初始化
                for (auto length : node.array_length) {
                    is_array = true;
                    length->accept(*this);
                    is_array = false;
                }
                if (node.btype == SyntaxTree::Type::INT) {
                    auto *arrayType_t = ArrayType::get(Int32Type, t_num_elements);
                    auto t = this->builder->create_alloca(arrayType_t);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::FLOAT) {
                    auto *arrayType_t = ArrayType::get(FloatType, t_num_elements);
                    auto t = this->builder->create_alloca(arrayType_t);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::BOOL) {
                    auto *arrayType_t = ArrayType::get(Int1Type, t_num_elements);
                    auto t = this->builder->create_alloca(arrayType_t);
                    this->scope.push(node.name,&t);
            }
        }
    }
}

void IRBuilder::visit(SyntaxTree::LVal &node) {

}

void IRBuilder::visit(SyntaxTree::AssignStmt &node) {}

void IRBuilder::visit(SyntaxTree::Literal &node) {
    if (is_array) {
        t_num_elements = node.int_const;
    }
}

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

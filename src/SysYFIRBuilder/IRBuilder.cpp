#include "IRBuilder.h"

#define CONST_INT(num) ConstantInt::get(num, module.get())
#define CONST_FLOAT(num) ConstantFloat::get(num, module.get())

// You can define global variables and functions here
auto t_initializer;
int t_num_elements;
// to store state

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

void IRBuilder::visit(SyntaxTree::InitVal &node) {}

void IRBuilder::visit(SyntaxTree::FuncDef &node) {}

void IRBuilder::visit(SyntaxTree::FuncFParamList &node) {}

void IRBuilder::visit(SyntaxTree::FuncParam &node) {}

void IRBuilder::visit(SyntaxTree::VarDef &node) {
    if (this->scope.in_global()) {
        //全局变量
        if (node.array_length.empty()) {
            //不是数组
            if (node.is_inited) {
                //已初始化
                node.initializers->accept(*this);
                if (node.btype == SyntaxTree::Type::INT) {
                    auto t = GlobalVariable::create(node.name, this->module, Int32Type, false, t_initializer);
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
                    length->accept(*this);
                }
                node.initializers->accept(*this);
                if (node.btype == SyntaxTree::Type::INT) {
                    auto *arrayType_t = ArrayType::get(Int32Type, t_num_elements);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::FLOAT) {
                    auto *arrayType_t = ArrayType::get(FloatType, t_num_elements);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
                else if (node.btype == SyntaxTree::Type::BOOL) {
                    auto *arrayType_t = ArrayType::get(Int1Type, t_num_elements);
                    auto t = GlobalVariable::create(node.name, this->module, arrayType_t, false, t_initializer);
                    this->scope.push(node.name,&t);
                }
            }
            else {
                //未初始化
                for (auto length : node.array_length) {
                    length->accept(*this);
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

    }
}

void IRBuilder::visit(SyntaxTree::LVal &node) {

}

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

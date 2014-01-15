/*
 * descripten - ECMAScript to native compiler
 * Copyright (C) 2011-2013 Christian Kindahl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmath>
#include <iomanip>
#include <limits>
#include "ir/ir.hh"
#include "ir_generator.hh"
#include "name_generator.hh"

namespace {

class ConstToStringVisitor : public ir::Constant::Visitor
{
private:
    std::string res_;
    IrGenerator *generator_;

public:
    ConstToStringVisitor(IrGenerator *generator)
        : generator_(generator) {}

    void visit_const_arr_elm(ir::ArrayElementConstant *instr)
    {
        std::stringstream out;
        out << generator_->value(instr->array()) << "[" << instr->index() << "]";
        res_ = out.str();;
    }

    void visit_const_callee(ir::CalleeConstant *instr)
    {
        res_ = "callee";
    }

    void visit_const_ret(ir::ReturnConstant *instr)
    {
        res_ = "result";
    }

    void visit_const_null(ir::NullConstant *instr)
    {
        res_ = "NULL";
    }

    void visit_const_bool(ir::BooleanConstant *instr)
    {
        res_ = instr->value() ? "true" : "false";
    }

    void visit_const_double(ir::DoubleConstant *instr)
    {
        res_ = IrGenerator::number(instr->value());
    }

    void visit_const_strdouble(ir::StringifiedDoubleConstant *instr)
    {
        std::string val = instr->value().utf8();
        if (val.find_first_of("XxEe.") == val.npos)
            res_ = val + std::string(".0");
        else
            res_ = val;
    }

    void visit_const_str(ir::StringConstant *instr)
    {
        res_ = IrGenerator::string(instr->value());
    }

    void visit_const_val(ir::ValueConstant *instr)
    {
        switch (instr->value())
        {
            case ir::ValueConstant::VALUE_NOTHING:
                res_ = "nothing";
                break;
            case ir::ValueConstant::VALUE_UNDEFINED:
                res_ = "undefined";
                break;
            case ir::ValueConstant::VALUE_NULL:
                res_ = "null";
                break;
            case ir::ValueConstant::VALUE_TRUE:
                res_ = "true";
                break;
            case ir::ValueConstant::VALUE_FALSE:
                res_ = "false";
                break;
            default:
                assert(false);
                break;
        }
    }

    std::string operator()(ir::Constant *constant)
    {
        res_.clear();
        visit(constant);
        return res_;
    }
};

}

IrGenerator::IrGenerator()
{
}

std::string IrGenerator::string(const String &str)
{
    std::string utf_str = str.utf8();
    std::string esc_str = escape(utf_str);

    return std::string("\"") + esc_str + std::string("\"");
}

std::string IrGenerator::boolean(bool val)
{
    return val ? "true" : "false";
}

std::string IrGenerator::number(double val)
{
    if (val == std::numeric_limits<double>::infinity())
    {
        return "<infinity>";
    }
    else if (val == -std::numeric_limits<double>::infinity())
    {
        return "-<infinity>";
    }
    else if (std::isnan(val))
    {
        return "<nan>";
    }

    std::stringstream out;
    out.precision(16);
    out << std::scientific << val;

    return out.str();
}

std::string IrGenerator::value(ir::Value *val)
{
    if (val->is_constant())
    {
        ConstToStringVisitor const_to_str(this);
        return const_to_str(static_cast<ir::Constant *>(val));
    }

    ValueNameMap::iterator it = val_names_.find(val);
    if (it == val_names_.end())
    {
        std::string name = NameGenerator::instance().next();
        val_names_.insert(std::make_pair(val, name));
        return std::string("%") + name;
    }

    return std::string("%") + it->second;
}

std::string IrGenerator::uint32(uint32_t val)
{
    std::stringstream out;
    out.fill('0');
    out << "0x" << std::setw(8) << std::hex << val;

    return out.str();
}

std::string IrGenerator::uint64(uint64_t val)
{
    std::stringstream out;
    out.fill('0');
    out << "0x" << std::setw(16) << std::hex << val;

    return out.str();
}

void IrGenerator::visit_module(ir::Module *module)
{
    ir::FunctionVector::const_iterator it;
    for (it = module->functions().begin(); it != module->functions().end(); ++it)
        ir::Node::Visitor::visit(*it);
}

void IrGenerator::visit_fun(ir::Function *fun)
{
    raw() << "define value %" << fun->name() << " {\n";

    ir::BlockList::ConstIterator it_block;
    for (it_block = fun->blocks().begin(); it_block != fun->blocks().end(); ++it_block)
        ir::Node::Visitor::visit(const_cast<ir::Block *>(it_block.raw_pointer()));

    raw() << "}\n";
}

void IrGenerator::visit_block(ir::Block *block)
{
    if (!block->label().empty())
        raw() << block->label() << ":\n";

    ir::InstructionVector::const_iterator it;
    for (it = block->instructions().begin();
        it != block->instructions().end(); ++it)
    {
        ir::Instruction::Visitor::visit(*it);
    }
}

void IrGenerator::visit_instr_args_obj_init(ir::ArgumentsObjectInitInstruction *instr)
{
    out() << value(instr) << " = args.obj.init ctx callee argc" << "\n";
}

void IrGenerator::visit_instr_args_obj_link(ir::ArgumentsObjectLinkInstruction *instr)
{
    out() << "args.obj.link" << value(instr->arguments()) << " "
          << instr->index() << " " << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_arr(ir::ArrayInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::ArrayInstruction::GET:
        {
            out() << value(instr) << " = array get "
                  << value(instr->array()) << " "
                  << instr->index() << "\n";
            break;
        }
        case ir::ArrayInstruction::PUT:
        {
            out() << "array put " << value(instr->array()) << " "
                  << instr->index() << " " << value(instr->value()) << "\n";
            break;
        }
        default:
            assert(false);
            break;
    }
}

void IrGenerator::visit_instr_bin(ir::BinaryInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::BinaryInstruction::ADD:
            out() << value(instr) << " = add " << value(instr->left())
                  << " " << value(instr->right()) << "\n";
            break;
        case ir::BinaryInstruction::SUB:
            out() << value(instr) << " = sub " << value(instr->left())
                  << " " << value(instr->right()) << "\n";
            break;
        case ir::BinaryInstruction::OR:
            out() << value(instr) << " = or " << value(instr->left())
                  << " " << value(instr->right()) << "\n";
            break;
        case ir::BinaryInstruction::EQ:
            out() << value(instr) << " = eq " << value(instr->left())
                  << " " << value(instr->right()) << "\n";
            break;
        default:
            assert(false);
            break;
    }
}

void IrGenerator::visit_instr_bnd_extra_init(ir::BindExtraInitInstruction *instr)
{
    out() << value(instr) << " = bnd.extra.init " << instr->num_extra() << "\n";
}

void IrGenerator::visit_instr_bnd_extra_ptr(ir::BindExtraPtrInstruction *instr)
{
    out() << value(instr) << " = bnd.extra.ptr " << instr->hops() << "\n";
}

void IrGenerator::visit_instr_call(ir::CallInstruction *instr)
{
    std::string kind;
    switch (instr->operation())
    {
        case ir::CallInstruction::NORMAL:
            kind = "call";
            break;
        case ir::CallInstruction::NEW:
            kind = "construct";
            break;
        default:
            assert(false);
            break;
    }

    out() << value(instr) << " = "<< kind << " " << value(instr->function())
          << " (" << instr->argc() << ", " << value(instr->argv()) << ", "
                 << value(instr->result()) << ")\n";
}

void IrGenerator::visit_instr_call_keyed(ir::CallKeyedInstruction *instr)
{
    out() << value(instr) << " = call " << value(instr->object()) << " "
          << uint64(instr->key())
          << " (" << instr->argc() << ", " << value(instr->argv()) << ", "
                 << value(instr->result()) << ")\n";
}

void IrGenerator::visit_instr_call_keyed_slow(ir::CallKeyedSlowInstruction *instr)
{
    out() << value(instr) << " = call " << value(instr->object()) << " "
          << value(instr->key())
          << " (" << instr->argc() << ", " << value(instr->argv()) << ", "
                 << value(instr->result()) << ")\n";
}

void IrGenerator::visit_instr_call_named(ir::CallNamedInstruction *instr)
{
    out() << value(instr) << " = call " << uint64(instr->key())
          << " (" << instr->argc() << ", " << value(instr->argv()) << ", "
                 << value(instr->result()) << ")\n";
}

void IrGenerator::visit_instr_val(ir::ValueInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::ValueInstruction::TO_BOOLEAN:
        {
            out() << value(instr) << " = val.to_boolean "
                  << value(instr->value()) << "\n";
            break;
        }
        case ir::ValueInstruction::TO_DOUBLE:
        {
            out() << value(instr) << " = val.to_double "
                  << value(instr->value()) << " " << value(instr->result())
                  << "\n";
            break;
        }
        case ir::ValueInstruction::TO_STRING:
        {
            out() << value(instr) << " = val.to_string "
                  << value(instr->value()) << " " << value(instr->result())
                  << "\n";
            break;
        }

        case ir::ValueInstruction::FROM_BOOLEAN:
            out() << value(instr) << " = " << "val.from_boolean "
                  << value(instr->value()) << "\n";
            break;
        case ir::ValueInstruction::FROM_DOUBLE:
            out() << value(instr) << " = " << "val.from_double "
                  << value(instr->value()) << "\n";
            break;
        case ir::ValueInstruction::FROM_STRING:
            out() << value(instr) << " = val.from_string "
                  << value(instr->value()) << "\n";
            break;

        case ir::ValueInstruction::IS_NULL:
            out() << value(instr) << " = val.is_null "
                  << value(instr->value()) << "\n";
            break;
        case ir::ValueInstruction::IS_UNDEFINED:
            out() << value(instr) << " = val.is_undefined "
                  << value(instr->value()) << "\n";
            break;

        case ir::ValueInstruction::TEST_COERCIBILITY:
            out() << value(instr) << " = val.test_coeribility "
                  << value(instr->value()) << "\n";
            break;

        default:
            assert(false);
            break;
    }
}

void IrGenerator::visit_instr_br(ir::BranchInstruction *instr)
{
    out() << "br " << value(instr->condition()) << " "
          << instr->true_block()->label() << " "
          << instr->false_block()->label() << "\n";
}

void IrGenerator::visit_instr_jmp(ir::JumpInstruction *instr)
{
    out() << "jmp " << instr->block()->label() << "\n";
}

void IrGenerator::visit_instr_ret(ir::ReturnInstruction *instr)
{
    out() << "ret " << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_mem_alloc(ir::MemoryAllocInstruction *instr)
{
    // FIXME:
    out() << value(instr) << " = ?\n";
}

void IrGenerator::visit_instr_mem_store(ir::MemoryStoreInstruction *instr)
{
    out() << value(instr->destination()) << " = " << value(instr->source()) << "\n";
}

void IrGenerator::visit_instr_mem_elm_ptr(ir::MemoryElementPointerInstruction *instr)
{
    out() << value(instr) << " = element_ptr " << value(instr->value()) << " "
          << instr->index() << "\n";
}

void IrGenerator::visit_instr_ctx_set_strict(ir::ContextSetStrictInstruction *instr)
{
    out() << "ctx.set_strict " << boolean(instr->strict()) << "\n";
}

void IrGenerator::visit_instr_ctx_enter_catch(ir::ContextEnterCatchInstruction *instr)
{
    out() << value(instr) << " = " << "ctx.enter_catch "
          << uint64(instr->key()) << "\n";
}

void IrGenerator::visit_instr_ctx_enter_with(ir::ContextEnterWithInstruction *instr)
{
    out() << value(instr) << " = " << "ctx.enter_with "
          << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_ctx_leave(ir::ContextLeaveInstruction *instr)
{
    out() << "ctx.leave\n";
}

void IrGenerator::visit_instr_ctx_this(ir::ContextThisInstruction *instr)
{
    out() << value(instr) << " = " << "ctx.this\n";
}

void IrGenerator::visit_instr_ctx_get(ir::ContextGetInstruction *instr)
{
    out() << value(instr) << " = " << "ctx.get "
          << uint64(instr->key()) << " " << value(instr->result()) << "\n";
}

void IrGenerator::visit_instr_ctx_put(ir::ContextPutInstruction *instr)
{
    out() << value(instr) << " = " << "ctx.put "
          << uint64(instr->key()) << " " << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_ctx_del(ir::ContextDeleteInstruction *instr)
{
    out() << value(instr) << " = " << "ctx.delete "
          << uint64(instr->key()) << " " << value(instr->result()) << "\n";
}

void IrGenerator::visit_instr_ex_save_state(ir::ExceptionSaveStateInstruction *instr)
{
    out() << value(instr) << " = " << "ex.save_state\n";
}

void IrGenerator::visit_instr_ex_load_state(ir::ExceptionLoadStateInstruction *instr)
{
    out() << "ex.load_state " << value(instr->state()) << "\n";
}

void IrGenerator::visit_instr_ex_set(ir::ExceptionSetInstruction *instr)
{
    out() << "ex.set " << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_ex_clear(ir::ExceptionClearInstruction *instr)
{
    out() << "ex.clear " << "\n";
}

void IrGenerator::visit_instr_init_args(ir::InitArgumentsInstruction *instr)
{
    out() << "init.args " << value(instr->destination()) << " "
          << instr->parameter_count() << "\n";
}

void IrGenerator::visit_instr_init_args_obj(ir::InitArgumentsObjectInstruction *instr)
{
    out() << "init.args_obj " << instr->parameter_count()
          << value(instr->parameter_array()) << "\n";
}

void IrGenerator::visit_instr_decl(ir::Declaration *instr)
{
    switch (instr->kind())
    {
        case ir::Declaration::FUNCTION:
        {
            out() << value(instr) << " = " << "decl.fun "
                  << uint64(instr->key()) << " false "
                  << boolean(instr->is_strict()) << " "
                  << value(instr->value()) << "\n";
            break;
        }

        case ir::Declaration::VARIABLE:
        {
            out() << value(instr) << " = " << "decl.var "
                  << uint64(instr->key()) << " false "
                  << boolean(instr->is_strict()) << "\n";
            break;
        }

        case ir::Declaration::PARAMETER:
        {
            out() << value(instr) << " = " << "decl.prm "
                  << uint64(instr->key()) << " false "
                  << boolean(instr->is_strict()) << " "
                  << value(instr->parameter_array())
                  << "[" << instr->parameter_index() << "]" << "\n";
            break;
        }

        default:
            assert(false);
            break;
    }
}

void IrGenerator::visit_instr_link(ir::Link *instr)
{
    switch (instr->kind())
    {
        case ir::Link::FUNCTION:
        {
            out() << "link.fun "
                  << uint64(instr->key()) << " "
                  << value(instr->value()) << "\n";
            break;
        }

        case ir::Link::VARIABLE:
        {
            out() << "link.var "
                  << uint64(instr->key()) << " "
                  << value(instr->value()) << "\n";
            break;
        }

        case ir::Link::PARAMETER:
        {
            out() << "link.prm "
                  << uint64(instr->key()) << " "
                  << value(instr->value()) << "\n";
            break;
        }

        default:
            assert(false);
            break;
    }
}

void IrGenerator::visit_instr_prp_def_data(ir::PropertyDefineDataInstruction *instr)
{
    out() << value(instr) << " = " << "prop.data "
          << value(instr->object()) << " "
          << value(instr->key()) << " "
          << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_prp_def_accessor(ir::PropertyDefineAccessorInstruction *instr)
{
    out() << value(instr) << " = " << "prop.accessor "
          << value(instr->object()) << " "
          << uint64(instr->key()) << " "
          << value(instr->function()) << " "
          << boolean(instr->is_setter()) << "\n";
}

void IrGenerator::visit_instr_prp_it_new(ir::PropertyIteratorNewInstruction *instr)
{
    out() << value(instr) << " = " << "prop.iter "
          << value(instr->object()) << "\n";
}

void IrGenerator::visit_instr_prp_it_next(ir::PropertyIteratorNextInstruction *instr)
{
    out() << value(instr) << " = " << "prop.next "
          << value(instr->iterator()) << " " << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_prp_get(ir::PropertyGetInstruction *instr)
{
    out() << value(instr) << " = " << "prop.get "
          << value(instr->object()) << " " << uint64(instr->key()) << " "
          << value(instr->result()) << "\n";
}

void IrGenerator::visit_instr_prp_get_slow(ir::PropertyGetSlowInstruction *instr)
{
    out() << value(instr) << " = " << "prop.get "
          << value(instr->object()) << " " << value(instr->key()) << " "
          << value(instr->result()) << "\n";
}

void IrGenerator::visit_instr_prp_put(ir::PropertyPutInstruction *instr)
{
    out() << value(instr) << " = " << "prop.put "
          << value(instr->object()) << " " << uint64(instr->key()) << " "
          << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_prp_put_slow(ir::PropertyPutSlowInstruction *instr)
{
    out() << value(instr) << " = " << "prop.put "
          << value(instr->object()) << " " << value(instr->key()) << " "
          << value(instr->value()) << "\n";
}

void IrGenerator::visit_instr_prp_del(ir::PropertyDeleteInstruction *instr)
{
    out() << value(instr) << " = " << "prop.delete "
          << value(instr->object()) << " " << uint64(instr->key()) << " "
          << value(instr->result()) << "\n";
}

void IrGenerator::visit_instr_prp_del_slow(ir::PropertyDeleteSlowInstruction *instr)
{
    out() << value(instr) << " = " << "delete "
          << value(instr->object()) << " " << value(instr->key()) << " "
          << value(instr->result()) << "\n";
}

void IrGenerator::visit_instr_es_new_arr(ir::EsNewArrayInstruction *instr)
{
    out() << value(instr) << " = " << "es.new_arr "
          << instr->length() << " "
          << value(instr->values()) << "\n";
}

void IrGenerator::visit_instr_es_new_fun_decl(ir::EsNewFunctionDeclarationInstruction *instr)
{
    out() << value(instr) << " = " << "es.new_fun_decl "
          << instr->function()->name() << " "
          << boolean(instr->is_strict()) << " "
          << instr->parameter_count() << "\n";
}

void IrGenerator::visit_instr_es_new_fun_expr(ir::EsNewFunctionExpressionInstruction *instr)
{
    out() << value(instr) << " = " << "es.new_fun_expr "
          << instr->function()->name() << " "
          << boolean(instr->is_strict()) << " "
          << instr->parameter_count() << "\n";
}

void IrGenerator::visit_instr_es_new_obj(ir::EsNewObjectInstruction *instr)
{
    out() << value(instr) << " = " << "es.new_obj\n";
}

void IrGenerator::visit_instr_es_new_rex(ir::EsNewRegexInstruction *instr)
{
    out() << value(instr) << " = " << "es.new_rex "
          << string(instr->pattern()) << " "
          << string(instr->flags()) << "\n";
}

void IrGenerator::visit_instr_es_bin(ir::EsBinaryInstruction *instr)
{
    switch (instr->operation())
    {
        // Arithmetic.
        case ir::EsBinaryInstruction::MUL:
            out() << value(instr) << " = es.mul "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::DIV:
            out() << value(instr) << " = es.div "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::MOD:
            out() << value(instr) << " = es.mod "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::ADD:
            out() << value(instr) << " = es.add "
                  << value(instr->left()) << " "
                  << value(instr->right()) << ", "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::SUB:
            out() << value(instr) << " = es.sub "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::LS:
            out() << value(instr) << " = es.shl "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::RSS:
            out() << value(instr) << " = es.sar "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::RUS:
            out() << value(instr) << " = es.shr "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;

        // Relational.
        case ir::EsBinaryInstruction::LT:
            out() << value(instr) << " = es.lt "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::GT:
            out() << value(instr) << " = es.gt "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::LTE:
            out() << value(instr) << " = es.lte "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::GTE:
            out() << value(instr) << " = es.gte "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::IN:
            out() << value(instr) << " = es.in "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::INSTANCEOF:
            out() << value(instr) << " = es.iof "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;

        // Equality.
        case ir::EsBinaryInstruction::EQ:
            out() << value(instr) << " = es.eq "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::NEQ:
            out() << value(instr) << " = es.neq "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::STRICT_EQ:
            out() << value(instr) << " = es.strict_eq "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::STRICT_NEQ:
            out() << value(instr) << " = es.strict_neq "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;

        // Bitwise.
        case ir::EsBinaryInstruction::BIT_AND:
            out() << value(instr) << " = es.band "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::BIT_XOR:
            out() << value(instr) << " = es.bxor "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsBinaryInstruction::BIT_OR:
            out() << value(instr) << " = es.bor "
                  << value(instr->left()) << " "
                  << value(instr->right()) << " "
                  << value(instr->result()) << "\n";
            break;

        default:
            assert(false);
            break;
    }
}

void IrGenerator::visit_instr_es_unary(ir::EsUnaryInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::EsUnaryInstruction::TYPEOF:
            out() << value(instr) << " = es.tof "
                  << value(instr->value()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsUnaryInstruction::NEG:
            out() << value(instr) << " = es.neg "
                  << value(instr->value()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsUnaryInstruction::BIT_NOT:
            out() << value(instr) << " = es.bnot "
                  << value(instr->value()) << " "
                  << value(instr->result()) << "\n";
            break;
        case ir::EsUnaryInstruction::LOG_NOT:
            out() << value(instr) << " = " << "es.lnot "
                  << value(instr->value()) << " "
                  << value(instr->result()) << "\n";
            break;
        default:
            assert(false);
            break;
    }
}

void IrGenerator::generate(ir::Module *module, const std::string &file_path)
{
    NameGenerator::instance().reset();  // FIXME:

    // Clear any previous data.
    out_.clear();

    // Write body.
    try
    {
        ir::Node::Visitor::visit(module);
        write(file_path);
    }
    catch (...)
    {
        write(file_path);

        throw;
    }
}

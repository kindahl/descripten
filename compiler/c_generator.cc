/*
 * descripten - ECMAScript to native compiler
 * Copyright (C) 2011-2014 Christian Kindahl
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
#include "config.hh"
#include "ir/ir.hh"
#include "c_generator.hh"
#include "name_generator.hh"

namespace {

class ConstToStringVisitor : public ir::Constant::Visitor
{
private:
    std::string res_;
    Cgenerator *generator_;

public:
    ConstToStringVisitor(Cgenerator *generator)
        : generator_(generator) {}

    void visit_const_arr_elm(ir::ArrayElementConstant *instr)
    {
        std::stringstream out;
        out << generator_->value(instr->array()) << "[" << instr->index() << "]";
        res_ = out.str();
    }

    void visit_const_fp(ir::FramePointer *instr)
    {
        res_ = "fp";
    }

    void visit_const_vp(ir::ValuePointer *instr)
    {
        res_ = "vp";
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
        res_ = Cgenerator::number(instr->value());
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
        res_ = Cgenerator::string(instr->value());
    }

    void visit_const_val(ir::ValueConstant *instr)
    {
        switch (instr->value())
        {
            case ir::ValueConstant::VALUE_NOTHING:
                res_ = "es_value_nothing()";
                break;
            case ir::ValueConstant::VALUE_UNDEFINED:
                res_ = "es_value_undefined()";
                break;
            case ir::ValueConstant::VALUE_NULL:
                res_ = "es_value_null()";
                break;
            case ir::ValueConstant::VALUE_TRUE:
                res_ = "es_value_true()";
                break;
            case ir::ValueConstant::VALUE_FALSE:
                res_ = "es_value_false()";
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

Cgenerator::Cgenerator()
    : decl_out_(NULL)
    , main_out_(NULL)
    , cur_block_(NULL)
{
}

std::string Cgenerator::string(const String &str)
{
    std::string esc_str = escape(str);

    std::stringstream strstr;
    strstr << "esa_new_str(U\"" << esc_str << "\", " << str.length() << ")";
    return strstr.str();
}

std::string Cgenerator::boolean(bool val)
{
    return val ? "true" : "false";
}

std::string Cgenerator::number(double val)
{
    if (val == std::numeric_limits<double>::infinity())
        return "INFINITY";
    else if (val == -std::numeric_limits<double>::infinity())
        return "-INFINITY";
    else if (std::isnan(val))
        return "NAN";

    std::stringstream out;
    out.precision(16);
    out << std::scientific << val;

    return out.str();
}

std::string Cgenerator::value(ir::Value *val)
{
    if (val->is_constant())
    {
        ConstToStringVisitor const_to_str(this);
        return const_to_str(static_cast<ir::Constant *>(val));
    }

    std::stringstream slot;
    slot << "__" << allocator_.lookup(val);
    return slot.str();
}

std::string Cgenerator::type(const ir::Type *type)
{
    std::stringstream str;

    switch (type->identifier())
    {
        // Primitive types.
        case ir::Type::ID_VOID:
            str << "void";
            break;
        case ir::Type::ID_BOOLEAN:
            str << "bool";
            break;
        case ir::Type::ID_DOUBLE:
            str << "double";
            break;
        case ir::Type::ID_STRING:
            str << "const EsString *";
            break;

        // Complex types.
        case ir::Type::ID_VALUE:
            str << "EsValueData";
            break;
        case ir::Type::ID_REFERENCE:
            str << "EsReference";
            break;

        // Derived types.
        case ir::Type::ID_ARRAY:
        {
            const ir::ArrayType *arr_type = static_cast<const ir::ArrayType *>(type);
            str << Cgenerator::type(arr_type->type()) << "[" << arr_type->length() << "]";
            break;
        }
        case ir::Type::ID_POINTER:
        {
            const ir::PointerType *ptr_type = static_cast<const ir::PointerType *>(type);
            str << Cgenerator::type(ptr_type->type()) << "*";
            break;
        }
        case ir::Type::ID_OPAQUE:
        {
            const ir::OpaqueType *ptr_type = static_cast<const ir::OpaqueType *>(type);
            str << "struct " << ptr_type->name() << "*";
            break;
        }

        default:
            assert(false);
            break;
    }

    return str.str();
}

std::string Cgenerator::allocate(const ir::Type *type, const std::string &name)
{
    std::string str = Cgenerator::type(type);
    std::string::size_type pos = str.find_first_of('[');
    if (pos != str.npos)
        str.insert(pos, std::string(" ") + name);
    else
        str.append(std::string(" ") + name);

    return str;
}

std::string Cgenerator::uint32(uint32_t val)
{
    std::stringstream out;
    out.fill('0');
    out << "0x" << std::setw(8) << std::hex << val;

    return out.str();
}

std::string Cgenerator::uint64(uint64_t val)
{
    std::stringstream out;
    out.fill('0');
    out << "0x" << std::setw(16) << std::hex << val;

    return out.str();
}

void Cgenerator::visit_module(ir::Module *module)
{
    main_out_->stream() << "void " RUNTIME_DATA_FUNCTION_NAME "()\n";
    main_out_->stream() << "{" << "\n";
    for (const ir::Resource *res : module->resources())
        ir::Resource::Visitor::visit(const_cast<ir::Resource *>(res));
    main_out_->stream() << "}" << "\n";

    ir::FunctionVector::const_iterator it;
    for (it = module->functions().begin(); it != module->functions().end(); ++it)
        ir::Node::Visitor::visit(*it);
}

void Cgenerator::visit_fun(ir::Function *fun)
{
    decl_out_->stream() << "bool " << fun->name()
                        << "(struct EsContext *ctx, uint32_t argc, EsValueData *fp, EsValueData *vp);\n";

    main_out_->stream() << "bool " << fun->name()
                        << "(struct EsContext *ctx, uint32_t argc, EsValueData *fp, EsValueData *vp)\n";

    main_out_->stream() << "{" << "\n";

    const Allocator::RegisterVector &registers = allocator_.allocations(fun);
    Allocator::RegisterVector::const_iterator it_reg;
    for (it_reg = registers.begin(); it_reg != registers.end(); ++it_reg)
    {
        std::stringstream name;
        name << "__" << (*it_reg)->number();

        out() << allocate((*it_reg)->type(), name.str()) << ";\n";
    }

    ir::BlockList::ConstIterator it_block;
    for (it_block = fun->blocks().begin(); it_block != fun->blocks().end(); ++it_block)
    {
        ir::Node::Visitor::visit(const_cast<ir::Block *>(it_block.raw_pointer()));
    }

    main_out_->stream() << "}\n";
}

void Cgenerator::visit_block(ir::Block *block)
{
    cur_block_ = block;

    bool output_label = !block->label().empty() && !block->referrers().empty();
    if (output_label)
        raw() << block->label() << ":\n";

    ir::InstructionVector::const_iterator it;
    for (it = block->instructions().begin();
        it != block->instructions().end(); ++it)
    {
        ir::Instruction::Visitor::visit(*it);
    }

    if (output_label && block->instructions().empty())
        out() << ";\n";
}

void Cgenerator::visit_instr_args_obj_init(ir::ArgumentsObjectInitInstruction *instr)
{
    out() << value(instr) << " = " << "esa_args_obj_init(ctx, argc, fp, vp)"
          << ";\n";
}

void Cgenerator::visit_instr_args_obj_link(ir::ArgumentsObjectLinkInstruction *instr)
{
    out() << "esa_args_obj_link(" << value(instr->arguments()) << ", "
          << instr->index() << ", " << value(instr->value()) << ")" << ";\n";
}

void Cgenerator::visit_instr_arr(ir::ArrayInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::ArrayInstruction::GET:
        {
            out() << value(instr) << " = " << value(instr->array())
                  << "[" << instr->index() << "];\n";
            break;
        }
        case ir::ArrayInstruction::PUT:
        {
            out() << value(instr->array()) << "[" << instr->index() << "]"
                  << " = " << value(instr->value()) << ";\n";
            break;
        }
        default:
            assert(false);
            break;
    }
}

void Cgenerator::visit_instr_bin(ir::BinaryInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::BinaryInstruction::ADD:
            out() << value(instr) << " = " << value(instr->left())
                  << " + " << value(instr->right()) << ";\n";
            break;
        case ir::BinaryInstruction::SUB:
            out() << value(instr) << " = " << value(instr->left())
                  << " - " << value(instr->right()) << ";\n";
            break;
        case ir::BinaryInstruction::OR:
            out() << value(instr) << " = " << value(instr->left())
                  << " || " << value(instr->right()) << ";\n";
            break;
        case ir::BinaryInstruction::EQ:
            out() << value(instr) << " = " << value(instr->left())
                  << " == " << value(instr->right()) << ";\n";
            break;
        default:
            assert(false);
            break;
    }
}

void Cgenerator::visit_instr_bnd_extra_init(ir::BindExtraInitInstruction *instr)
{
    out() << value(instr) << " = " << "esa_bnd_extra_init(ctx, "
          << instr->num_extra() << ")" << ";\n";
}

void Cgenerator::visit_instr_bnd_extra_ptr(ir::BindExtraPtrInstruction *instr)
{
    out() << value(instr) << " = " << "esa_bnd_extra_ptr(argc, fp, vp, "
          << instr->hops() << ")" << ";\n";
}

void Cgenerator::visit_instr_call(ir::CallInstruction *instr)
{
    std::string kind;
    switch (instr->operation())
    {
        case ir::CallInstruction::NORMAL:
            kind = "esa_call";
            break;
        case ir::CallInstruction::NEW:
            kind = "esa_call_new";
            break;
        default:
            assert(false);
            break;
    }

    out() << value(instr) << " = "<< kind
          << "(" << value(instr->function()) << ", "
                 << instr->argc() << ", &"
                 << value(instr->result()) << ");\n";
}

void Cgenerator::visit_instr_call_keyed(ir::CallKeyedInstruction *instr)
{
    out() << value(instr) << " = esa_call_keyed("
          << value(instr->object()) << ", "
          << uint64(instr->key()) << ", "
          << instr->argc() << ", &"
          << value(instr->result()) << ");\n";
}

void Cgenerator::visit_instr_call_keyed_slow(ir::CallKeyedSlowInstruction *instr)
{
    out() << value(instr) << " = esa_call_keyed_slow("
          << value(instr->object()) << ", "
          << value(instr->key()) << ", "
          << instr->argc() << ", &"
          << value(instr->result()) << ");\n";
}

void Cgenerator::visit_instr_call_named(ir::CallNamedInstruction *instr)
{
    out() << value(instr) << " = esa_call_named("
          << uint64(instr->key()) << ", "
          << instr->argc() << ", &"
          << value(instr->result()) << ");\n";
}

void Cgenerator::visit_instr_val(ir::ValueInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::ValueInstruction::TO_BOOLEAN:
        {
            out() << value(instr) << " = esa_val_to_bool("
                  << value(instr->value()) << ");\n";
            break;
        }
        case ir::ValueInstruction::TO_DOUBLE:
        {
            out() << value(instr) << " = esa_val_to_num("
                  << value(instr->value()) << ", &" << value(instr->result())
                  << ");\n";
            break;
        }

        case ir::ValueInstruction::FROM_BOOLEAN:
            out() << value(instr->result()) << " = " << "es_value_from_boolean("
                  << value(instr->value()) << ");\n";
            break;
        case ir::ValueInstruction::FROM_DOUBLE:
            out() << value(instr->result()) << " = " << "es_value_from_number("
                  << value(instr->value()) << ");\n";
            break;
        case ir::ValueInstruction::FROM_STRING:
            out() << value(instr->result()) << " = " << "es_value_from_string("
                  << value(instr->value()) << ");\n";
            break;

        case ir::ValueInstruction::IS_NULL:
            out() << value(instr) << " = es_value_is_null("
                  << value(instr->value()) << ");\n";
            break;
        case ir::ValueInstruction::IS_UNDEFINED:
            out() << value(instr) << " = es_value_is_undefined("
                  << value(instr->value()) << ");\n";
            break;

        case ir::ValueInstruction::TEST_COERCIBILITY:
            out() << value(instr) << " = esa_val_chk_coerc("
                  << value(instr->value()) << ");\n";
            break;

        default:
            assert(false);
            break;
    }
}

void Cgenerator::visit_instr_br(ir::BranchInstruction *instr)
{
    if (instr == cur_block_->last_instr() &&
        instr->true_block() == cur_block_->next())
    {
        out() << "if (!(" << value(instr->condition()) << "))\n";
        out() << "  goto " << instr->false_block()->label() << ";\n";

        instr->true_block()->remove_referrer(instr);
    }
    else if (instr == cur_block_->last_instr() &&
             instr->false_block() == cur_block_->next())
    {
        out() << "if (" << value(instr->condition()) << ")\n";
        out() << "  goto " << instr->true_block()->label() << ";\n";

        instr->false_block()->remove_referrer(instr);
    }
    else
    {
        out() << "if (" << value(instr->condition()) << ")\n";
        out() << "  goto " << instr->true_block()->label() << ";\n";
        out() << "else\n";
        out() << "  goto " << instr->false_block()->label() << ";\n";
    }
}

void Cgenerator::visit_instr_jmp(ir::JumpInstruction *instr)
{
    if (instr != cur_block_->last_instr() ||
        instr->block() != cur_block_->next())
    {
        out() << "goto " << instr->block()->label() << ";\n";
    }
    else
    {
        instr->block()->remove_referrer(instr);
    }
}

void Cgenerator::visit_instr_ret(ir::ReturnInstruction *instr)
{
    out() << "return " << value(instr->value()) << ";\n";
}

void Cgenerator::visit_instr_mem_store(ir::MemoryStoreInstruction *instr)
{
    out() << value(instr->destination()) << " = " << value(instr->source()) << ";\n";
}

void Cgenerator::visit_instr_mem_elm_ptr(ir::MemoryElementPointerInstruction *instr)
{
    out() << value(instr) << " = &" << value(instr->value())
          << "[" << instr->index() << "];\n";
}

void Cgenerator::visit_instr_stk_alloc(ir::StackAllocInstruction *instr)
{
    out() << "esa_stk_alloc(" << instr->count() << ");\n";
}

void Cgenerator::visit_instr_stk_free(ir::StackFreeInstruction *instr)
{
    out() << "esa_stk_free(" << instr->count() << ");\n";
}

void Cgenerator::visit_instr_stk_push(ir::StackPushInstruction *instr)
{
    out() << "esa_stk_push(" << value(instr->value()) << ");\n";
}

void Cgenerator::visit_instr_ctx_set_strict(ir::ContextSetStrictInstruction *instr)
{
    out() << "esa_ctx_set_strict(ctx, " << boolean(instr->strict()) << ");\n";
}

void Cgenerator::visit_instr_ctx_enter_catch(ir::ContextEnterCatchInstruction *instr)
{
    out() << value(instr) << " = " << "esa_ctx_enter_catch(ctx, "
          << uint64(instr->key()) << ");\n";
    out() << "ctx = esa_ctx_running();\n";
}

void Cgenerator::visit_instr_ctx_enter_with(ir::ContextEnterWithInstruction *instr)
{
    out() << value(instr) << " = " << "esa_ctx_enter_with(ctx, "
          << value(instr->value()) << ");\n";
    out() << "ctx = esa_ctx_running();\n";
}

void Cgenerator::visit_instr_ctx_leave(ir::ContextLeaveInstruction *instr)
{
    out() << "esa_ctx_leave();\n";
    out() << "ctx = esa_ctx_running();\n";
}

void Cgenerator::visit_instr_ctx_get(ir::ContextGetInstruction *instr)
{
    out() << value(instr) << " = " << "esa_ctx_get(ctx, "
          << uint64(instr->key()) << ", &" << value(instr->result())
          << ", " << instr->cache_id() << ");\n";
}

void Cgenerator::visit_instr_ctx_put(ir::ContextPutInstruction *instr)
{
    out() << value(instr) << " = " << "esa_ctx_put(ctx, "
          << uint64(instr->key()) << ", " << value(instr->value())
          << ", " << instr->cache_id() << ");\n";
}

void Cgenerator::visit_instr_ctx_del(ir::ContextDeleteInstruction *instr)
{
    out() << value(instr) << " = " << "esa_ctx_del(ctx, "
          << uint64(instr->key()) << ", &" << value(instr->result())
          << ");\n";
}

void Cgenerator::visit_instr_ex_save_state(ir::ExceptionSaveStateInstruction *instr)
{
    out() << value(instr->result()) << " = " << "esa_ex_save_state(ctx);\n";
}

void Cgenerator::visit_instr_ex_load_state(ir::ExceptionLoadStateInstruction *instr)
{
    out() << "esa_ex_load_state(ctx, " << value(instr->state()) << ");\n";
}

void Cgenerator::visit_instr_ex_set(ir::ExceptionSetInstruction *instr)
{
    out() << "esa_ex_set(ctx, " << value(instr->value()) << ");\n";
}

void Cgenerator::visit_instr_ex_clear(ir::ExceptionClearInstruction *instr)
{
    out() << "esa_ex_clear(ctx);" << "\n";
}

void Cgenerator::visit_instr_init_args(ir::InitArgumentsInstruction *instr)
{
    out() << "esa_init_args(" << value(instr->destination()) << ", argc, fp, "
          << instr->parameter_count() << ");" << "\n";
}

void Cgenerator::visit_instr_decl(ir::Declaration *instr)
{
    switch (instr->kind())
    {
        case ir::Declaration::FUNCTION:
        {
            out() << value(instr) << " = " << "esa_ctx_decl_fun(ctx, false, "
                  << boolean(instr->is_strict()) << ", "
                  << uint64(instr->key()) << ", "
                  << value(instr->value()) << ");\n";
            break;
        }

        case ir::Declaration::VARIABLE:
        {
            out() << value(instr) << " = " << "esa_ctx_decl_var(ctx, false, "
                  << boolean(instr->is_strict()) << ", "
                  << uint64(instr->key()) << ");\n";
            break;
        }

        case ir::Declaration::PARAMETER:
        {
            out() << value(instr) << " = " << "esa_ctx_decl_prm(ctx, "
                  << boolean(instr->is_strict()) << ", "
                  << uint64(instr->key()) << ", "
                  << value(instr->parameter_array())
                  << "[" << instr->parameter_index() << "]" << ");\n";
            break;
        }

        default:
            assert(false);
            break;
    }
}

void Cgenerator::visit_instr_link(ir::Link *instr)
{
    switch (instr->kind())
    {
        case ir::Link::FUNCTION:
        {
            out() << "esa_ctx_link_fun(ctx, "
                  << uint64(instr->key()) << ", "
                  << value(instr->value()) << ");\n";
            break;
        }

        case ir::Link::VARIABLE:
        {
            out() << "esa_ctx_link_var(ctx, "
                  << uint64(instr->key()) << ", "
                  << value(instr->value()) << ");\n";
            break;
        }

        case ir::Link::PARAMETER:
        {
            out() << "esa_ctx_link_prm(ctx, "
                  << uint64(instr->key()) << ", "
                  << value(instr->value()) << ");\n";
            break;
        }

        default:
            assert(false);
            break;
    }
}

static int cid_counter = 0;

static int next_cid()
{
    int cid = cid_counter++;
    if (cid_counter >= FEATURE_PROPERTY_CACHE_SIZE)
        cid_counter = 0;
    return cid;
}

void Cgenerator::visit_instr_prp_def_data(ir::PropertyDefineDataInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_def_data("
          << value(instr->object()) << ", "
          << value(instr->key()) << ", "
          << value(instr->value()) << ");\n";
}

void Cgenerator::visit_instr_prp_def_accessor(ir::PropertyDefineAccessorInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_def_accessor("
          << value(instr->object()) << ", "
          << uint64(instr->key()) << ", "
          << value(instr->function()) << ", "
          << boolean(instr->is_setter()) << ");\n";
}

void Cgenerator::visit_instr_prp_it_new(ir::PropertyIteratorNewInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_it_new("
          << value(instr->object()) << ");\n";
}

void Cgenerator::visit_instr_prp_it_next(ir::PropertyIteratorNextInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_it_next("
          << value(instr->iterator()) << ", &" << value(instr->value()) << ");\n";
}

void Cgenerator::visit_instr_prp_get(ir::PropertyGetInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_get("
          << value(instr->object()) << ", " << uint64(instr->key()) << ", &"
          << value(instr->result()) << ", " << next_cid() << ");\n";
}

void Cgenerator::visit_instr_prp_get_slow(ir::PropertyGetSlowInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_get_slow("
          << value(instr->object()) << ", " << value(instr->key()) << ", &"
          << value(instr->result()) << ", " << next_cid() << ");\n";
}

void Cgenerator::visit_instr_prp_put(ir::PropertyPutInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_put(ctx, "
          << value(instr->object()) << ", " << uint64(instr->key()) << ", "
          << value(instr->value()) << ", " << next_cid() << ");\n";
}

void Cgenerator::visit_instr_prp_put_slow(ir::PropertyPutSlowInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_put_slow(ctx, "
          << value(instr->object()) << ", " << value(instr->key()) << ", "
          << value(instr->value()) << ", " << next_cid() << ");\n";
}

void Cgenerator::visit_instr_prp_del(ir::PropertyDeleteInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_del(ctx, "
          << value(instr->object()) << ", " << uint64(instr->key()) << ", &"
          << value(instr->result()) << ");\n";
}

void Cgenerator::visit_instr_prp_del_slow(ir::PropertyDeleteSlowInstruction *instr)
{
    out() << value(instr) << " = " << "esa_prp_del_slow(ctx, "
          << value(instr->object()) << ", " << value(instr->key()) << ", &"
          << value(instr->result()) << ");\n";
}

void Cgenerator::visit_instr_es_new_arr(ir::EsNewArrayInstruction *instr)
{
    out() << value(instr->result()) << " = " << "esa_new_arr("
          << instr->length() << ", "
          << value(instr->values()) << ");\n";
}

void Cgenerator::visit_instr_es_new_fun_decl(ir::EsNewFunctionDeclarationInstruction *instr)
{
    out() << value(instr->result()) << " = " << "esa_new_fun_decl(ctx, "
          << instr->function()->name() << ", "
          << boolean(instr->is_strict()) << ", "
          << instr->parameter_count() << ");\n";
}

void Cgenerator::visit_instr_es_new_fun_expr(ir::EsNewFunctionExpressionInstruction *instr)
{
    out() << value(instr->result()) << " = " << "esa_new_fun_expr(ctx, "
          << instr->function()->name() << ", "
          << boolean(instr->is_strict()) << ", "
          << instr->parameter_count() << ");\n";
}

void Cgenerator::visit_instr_es_new_obj(ir::EsNewObjectInstruction *instr)
{
    // FIXME: Inline.
    out() << value(instr->result()) << " = " << "esa_new_obj();\n";
}

void Cgenerator::visit_instr_es_new_rex(ir::EsNewRegexInstruction *instr)
{
    out() << value(instr->result()) << " = " << "esa_new_reg_exp("
          << string(instr->pattern()) << ", "
          << string(instr->flags()) << ");\n";
}

void Cgenerator::visit_instr_es_bin(ir::EsBinaryInstruction *instr)
{
    switch (instr->operation())
    {
        // Arithmetic.
        case ir::EsBinaryInstruction::MUL:
            out() << value(instr) << " = " << "esa_b_mul("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::DIV:
            out() << value(instr) << " = " << "esa_b_div("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::MOD:
            out() << value(instr) << " = " << "esa_b_mod("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::ADD:
            out() << value(instr) << " = " << "esa_b_add("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::SUB:
            out() << value(instr) << " = " << "esa_b_sub("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::LS:
            out() << value(instr) << " = " << "esa_b_shl("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::RSS:
            out() << value(instr) << " = " << "esa_b_sar("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::RUS:
            out() << value(instr) << " = " << "esa_b_shr("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;

        // Relational.
        case ir::EsBinaryInstruction::LT:
            out() << value(instr) << " = " << "esa_c_lt("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::GT:
            out() << value(instr) << " = " << "esa_c_gt("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::LTE:
            out() << value(instr) << " = " << "esa_c_lte("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::GTE:
            out() << value(instr) << " = " << "esa_c_gte("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::IN:
            out() << value(instr) << " = " << "esa_c_in("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::INSTANCEOF:
            out() << value(instr) << " = " << "esa_c_instance_of("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;

        // Equality.
        case ir::EsBinaryInstruction::EQ:
            out() << value(instr) << " = " << "esa_c_eq("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::NEQ:
            out() << value(instr) << " = " << "esa_c_neq("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::STRICT_EQ:
            out() << value(instr) << " = " << "esa_c_strict_eq("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::STRICT_NEQ:
            out() << value(instr) << " = " << "esa_c_strict_neq("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;

        // Bitwise.
        case ir::EsBinaryInstruction::BIT_AND:
            out() << value(instr) << " = " << "esa_b_and("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::BIT_XOR:
            out() << value(instr) << " = " << "esa_b_xor("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsBinaryInstruction::BIT_OR:
            out() << value(instr) << " = " << "esa_b_or("
                  << value(instr->left()) << ", "
                  << value(instr->right()) << ", &"
                  << value(instr->result()) << ");\n";
            break;

        default:
            assert(false);
            break;
    }
}

void Cgenerator::visit_instr_es_unary(ir::EsUnaryInstruction *instr)
{
    switch (instr->operation())
    {
        case ir::EsUnaryInstruction::TYPEOF:
            out() << value(instr) << " = " << "esa_u_typeof("
                  << value(instr->value()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsUnaryInstruction::NEG:
            out() << value(instr) << " = " << "esa_u_sub("
                  << value(instr->value()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsUnaryInstruction::BIT_NOT:
            out() << value(instr) << " = " << "esa_u_bit_not("
                  << value(instr->value()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        case ir::EsUnaryInstruction::LOG_NOT:
            out() << value(instr) << " = " << "esa_u_not("
                  << value(instr->value()) << ", &"
                  << value(instr->result()) << ");\n";
            break;
        default:
            assert(false);
            break;
    }
}

void Cgenerator::visit_str_res(ir::StringResource *res)
{
    out() << "esa_str_intern("
          << string(res->string()) << ", "
          << uint32(res->id()) << ");\n";
}

void Cgenerator::generate(ir::Module *module, const std::string &file_path)
{
    NameGenerator::instance().reset();  // FIXME:

    allocator_.run(module);

    // Clear any previous data.
    out_.clear();

    decl_out_ = out_.fork();
    main_out_ = out_.fork();

    // Generate include conditions.
    decl_out_->stream() << "#include <stddef.h>" << "\n";
    decl_out_->stream() << "#include \"runtime.h\"" << "\n";

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

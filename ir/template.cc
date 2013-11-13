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

#include <gc_cpp.h>
#include "compiler.hh"
#include "template.hh"
#include "utility.hh"

namespace ir {

void MultiTemplateBlock::push_back(TemplateBlock *block)
{
    blocks_.push_back(block);
}

void MultiTemplateBlock::inflate(Block *block, Function *fun) const
{
    TemplateBlockVector::const_iterator it;
    for (it = blocks_.begin(); it != blocks_.end(); ++it)
        (*it)->inflate(block, fun);
}

void ReturnFalseTemplateBlock::inflate(Block *block, Function *fun) const
{
    block->push_trm_ret(new (GC)BooleanConstant(false));
}

JumpTemplateBlock::JumpTemplateBlock(Block *dst)
    : dst_(dst)
{
}

void JumpTemplateBlock::inflate(Block *block, Function *fun) const
{
    block->push_trm_jmp(dst_);
}

void LeaveContextTemplateBlock::inflate(Block *block, Function *fun) const
{
    block->push_ctx_leave();
}

FinallyTemplateBlock::FinallyTemplateBlock(Compiler *compiler,
                                           const parser::Statement *stmt,
                                           TemplateBlock *expt_block)
    : compiler_(compiler)
    , stmt_(stmt)
    , expt_block_(expt_block)
{
}

void FinallyTemplateBlock::inflate(Block *block, Function *fun) const
{
    ScopedVectorValue<TemplateBlock> v(compiler_->exception_actions_, expt_block_);

    compiler_->parse(const_cast<parser::Statement *>(stmt_), fun, NULL);
}

}

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

#pragma once
#include <vector>
#include "common/core.hh"
#include "ir.hh"

namespace ir {

/**
 * @brief Template block.
 */
class TemplateBlock
{
public:
    virtual ~TemplateBlock() {}

    /**
     * Inflates the block template into the specified block.
     * @param [in] block Destination block.
     * @param [in] fun Function hosting block.
     */
    virtual void inflate(Block *block, Function *fun) const = 0;
};

/**
 * @brief Template block containing multiple other template blocks.
 */
class MultiTemplateBlock : public TemplateBlock
{
private:
    typedef std::vector<TemplateBlock *,
                        gc_allocator<TemplateBlock *> > TemplateBlockVector;
    TemplateBlockVector blocks_;

public:
    /**
     * Adds a template block to the multi-block.
     * @param [in] block Block to add.
     */
    void push_back(TemplateBlock *block);

    /**
     * @copydoc TemplateBlock::inflate
     */
    virtual void inflate(Block *block, Function *fun) const OVERRIDE;
};

/**
 * @brief Template block returning false.
 */
class ReturnFalseTemplateBlock : public TemplateBlock
{
public:
    /**
     * @copydoc TemplateBlock::inflate
     */
    virtual void inflate(Block *block, Function *fun) const OVERRIDE;
};

/**
 * @brief Template block performing an unconditional jump.
 */
class JumpTemplateBlock : public TemplateBlock
{
private:
    Block *dst_;

public:
    /**
     * Constructs a new jump template block.
     * @param [in] dst Destination block.
     */
    JumpTemplateBlock(Block *dst);

    /**
     * @copydoc TemplateBlock::inflate
     */
    virtual void inflate(Block *block, Function *fun) const OVERRIDE;
};

/**
 * @brief Template block leaving the current context.
 */
class LeaveContextTemplateBlock : public TemplateBlock
{
public:
    /**
     * @copydoc TemplateBlock::inflate
     */
    virtual void inflate(Block *block, Function *fun) const OVERRIDE;
};

/**
 * @brief Template block executing a finally block.
 */
class FinallyTemplateBlock : public TemplateBlock
{
private:
    Compiler *compiler_;
    const parser::Statement *stmt_;
    TemplateBlock *expt_block_;

public:
    /**
     * Constructs a new finally template block.
     * @param [in] compiler Compiler to use for generating the block.
     * @param [in] stmt Finally block to use as basis for the finally block.
     * @param [in] expt_block Exception block to use in case of exceptions.
     */
    FinallyTemplateBlock(Compiler *compiler,
                         const parser::Statement *stmt,
                         TemplateBlock *expt_block);

    /**
     * @copydoc TemplateBlock::inflate
     */
    virtual void inflate(Block *block, Function *fun) const OVERRIDE;
};

}

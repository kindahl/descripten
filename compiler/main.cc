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

#include <iostream>
#include <fstream>
#include <memory>
#include <string.h>
#include "ir/compiler.hh"
#include "ir/optimizer.hh"
#include "parser/lexer.hh"
#include "parser/parser.hh"
#include "parser/utility.hh"
#include "c_generator.hh"
#include "ir_generator.hh"

using ir::Compiler;
using ir::Module;
using ir::Optimizer;
using parser::FunctionLiteral;
using parser::Lexer;
using parser::ParseException;
using parser::Parser;
using parser::StreamFactory;
using parser::UnicodeStream;

namespace {

std::string change_file_ext(const std::string &path, const std::string ext)
{
    std::string dir_name;
    std::string base_name;

    // Split path into directory and base names.
    size_t pos = path.find_last_of("/\\");
    if (pos == path.npos)
    {
        base_name = path;
    }
    else
    {
        dir_name = path.substr(0, pos + 1);
        base_name = path.substr(pos + 1);
    }

    // Strip file extension if present.
    pos = base_name.find('.');
    if (pos != path.npos)
        base_name = base_name.substr(0, pos);

    // Append new extension.
    assert(!ext.empty());
    if (ext[0] != '.')
        base_name.append(".");

    base_name.append(ext);

    return dir_name + base_name;
}

}

int main (int argc, const char *argv[])
{
    // Initialize garbage collector.
    GC_INIT();

    if (argc < 2)
    {
        std::cerr << "error: invalid usage." << std::endl;
        return 1;
    }
    
    // Parse program options.
    std::string dst_path = "a.c";
    std::vector<std::string> src_paths;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-o"))
        {
            if (++i < argc)
            {
                dst_path = argv[i];
            }
            else
            {
                std::cerr << "error: no output file specified after '-o' option." << std::endl;
                return 1;
            }

            continue;
        }

        src_paths.push_back(argv[i]);
    }
    
    // Parse source files.
    std::vector<std::string>::const_iterator it_src;

    FunctionLiteral *fun = NULL;
    try
    {
        for (it_src = src_paths.begin(); it_src != src_paths.end(); ++it_src)
        {
            // Read source file.
            std::auto_ptr<UnicodeStream> stream(StreamFactory::from_file((*it_src).c_str()));

            Lexer lexer(*stream);
            Parser parser(lexer);

            if (fun == NULL)
                fun = parser.parse();
            else
                merge_asts(fun, parser.parse());
        }
    }
    catch (ParseException &e)
    {
        std::cerr << "in: " << *it_src << std::endl << "  " << e.what() << std::endl;
        return 1;
    }
    catch (Exception &e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    // Compile the AST into the IR.
    Module *module = NULL;
    try
    {
        Compiler compiler;
        module = compiler.compile(fun);

        Optimizer optimizer;
        optimizer.optimize(module);
    }
    catch (Exception &e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    // Generate IR code from the IR.
    /*try
    {
        IrGenerator generator;
        generator.generate(module, change_file_ext(dst_path, ".ir"));
    }
    catch (Exception &e)
    {
        std::cerr << "error: " << e.what() << std::endl;
    }*/

    // Generate code from the IR.
    try
    {
        Cgenerator generator;
        generator.generate(module, dst_path);
    }
    catch (Exception &e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

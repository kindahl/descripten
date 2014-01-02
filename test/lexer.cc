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

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string.h>
#include "parser/lexer.hh"
#include "parser/stream.hh"

using namespace parser;

std::string read_file(const char * file_path)
{
    std::ifstream file(file_path);

    file.seekg(0, std::ios::end);   
    std::string str;
    str.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    str.assign(std::istreambuf_iterator<char>(file),
               std::istreambuf_iterator<char>());

    return str;
}

int main(int argc, const char * argv[])
{
    // Initialize garbage collector.
    GC_INIT();

    if (argc < 2)
        return 1;

    bool verbose = false;
    int file_name_idx = 1;
    if (argc >= 3 && !strcmp(argv[1], "-v"))
    {
        file_name_idx++;
        verbose = true;
    }

    const char * file_name = argv[file_name_idx] + strlen(argv[file_name_idx]);
    while (*file_name != '/')
        file_name--;
    file_name++;

    //std::cout << file_name << ": ";
    //std::cout.flush();

    std::auto_ptr<UnicodeStream>  stream(StreamFactory::from_file(argv[file_name_idx]));

    Lexer lexer(*stream);
    
    int tok_count = 0;
    Token tok = lexer.next();
    while (tok != Token::EOI/* && tok != Token::ILLEGAL*/)
    {
        if (verbose)
        {
            if (tok == Token::ILLEGAL)
                std::cout << "tok: " << "<ILLEGAL>" << std::endl;
            else
                std::cout << "tok: " << tok.string().utf8() << std::endl;
        }

        tok_count++;
        tok = lexer.next();
    }

    std::cout << "tot: " << tok_count << " tokens." << std::endl;

    return 0;
}

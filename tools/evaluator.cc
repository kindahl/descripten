#include <iostream>
#include <fstream>
#include <memory>
#include <string.h>
#include "parser/lexer.hh"
#include "parser/parser.hh"
#include "parser/utility.hh"
#include "runtime/context.hh"
#include "runtime/eval.hh"
#include "runtime/global.hh"
#include "runtime/frame.hh"
#include "runtime/runtime.hh"

using parser::FunctionLiteral;
using parser::Lexer;
using parser::ParseException;
using parser::Parser;
using parser::StreamFactory;
using parser::UnicodeStream;

void data()
{
}

int main (int argc, const char *argv[])
{
    // Initialize garbage collector.
    if (!runtime::init(data))
    {
        std::cerr << "error: failed to initialize runtime." << std::endl; return 1;
    }

    if (argc < 2)
    {
        std::cerr << "error: invalid usage." << std::endl;
        return 1;
    }
    
    // Parse program options.
    std::string dst_path = "a.cc";
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
    FunctionLiteral *prog = NULL;
    try
    {
        std::vector<std::string>::const_iterator it_src;
        for (it_src = src_paths.begin(); it_src != src_paths.end(); ++it_src)
        {
            // Read source file.
            std::auto_ptr<UnicodeStream> stream(StreamFactory::from_file(argv[1]));

            Lexer lexer(*stream);
            Parser parser(lexer);

            if (prog == NULL)
                prog = parser.parse();
            else
                merge_asts(prog, parser.parse());
        }
    }
    catch (ParseException &e)
    {
        std::cerr << "in: " << argv[1] << std::endl << "  " << e.what() << std::endl;
        return 1;
    }
    catch (Exception &e)
    {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    EsContextStack::instance().push_global(prog->is_strict_mode());
    EsContext *ctx = EsContextStack::instance().top();

    EsCallFrame eval_frame = EsCallFrame::push_global();

    Evaluator eval(prog, Evaluator::TYPE_PROGRAM, eval_frame);
    if (!eval.exec(ctx))
    {
        assert(EsContextStack::instance().top()->has_pending_exception());
        EsValue e = EsContextStack::instance().top()->get_pending_exception();

        String err_msg;
        e.to_string(err_msg);

        std::cerr << err_msg.utf8() << std::endl;
        return 1;
    }
    
    return 0;
}

#include <iostream>
#include "runtime.hh"

void _global_data();
bool _global_main(EsContext *ctx, int argc,
                  EsValue *fp, EsValue *vp);

int main(int argc, const char * arv[])
{
    if (!runtime::init(_global_data))
    {
        std::cerr << runtime::error() << std::endl;
        return 1;
    }
    
    if (!runtime::run(_global_main))
    {
        std::cerr << runtime::error() << std::endl;
        return 1;
    }

    return 0;
}

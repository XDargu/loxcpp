#include "Natives.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cmath>
#include <time.h>

#include "Vm.h"
#include "Debug.h"
#include "Compiler.h"
#include "Object.h"

Value clock(int argCount, Value* args, VM* vm)
{
    return Value((double)clock() / CLOCKS_PER_SEC);
}

void registerNatives(VM* vm)
{
    vm->defineNative("clock", 1, &clock);
}
#ifndef loxcpp_natives_h
#define loxcpp_natives_h

#include "Value.h"

class VM;

Value clock(int argCount, Value* args, VM* vm);

void registerNatives(VM* vm);

#endif
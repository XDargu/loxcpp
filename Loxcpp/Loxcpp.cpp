#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "Chunk.h"
#include "Debug.h"
#include "Vm.h"

void repl()
{
    std::string line;

    for (;;)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, line))
        {
            break;
        }
        VM::getInstance().interpret(line);
    }
}

void runFile(const std::string& path)
{
    std::ifstream fileStream(path.data());
    if (fileStream.fail())
    {
        std::cerr << "Could not open file " << path << "." << std::endl;
        exit(74);
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    fileStream.close();

    const InterpretResult result = VM::getInstance().interpret(buffer.str());

    if(result == InterpretResult::INTERPRET_COMPILE_ERROR) exit(65);
    if (result == InterpretResult::INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[])
{
    if (argc == 1)
    {
        repl();
    }
    else if (argc == 2)
    {
        runFile(argv[1]);
        repl();
    }
    else
    {
        std::cerr << "Usage: loxcpp [path]" << std::endl;
        exit(64);
    }
}
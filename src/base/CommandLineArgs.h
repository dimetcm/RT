#pragma once 

#include <vector>

struct CommandLineArgs
{
	CommandLineArgs();
	CommandLineArgs(int argc, char** argv);

	std::vector<const char*> args;
};
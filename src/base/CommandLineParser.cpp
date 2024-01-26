#include "CommandLineParser.h"
#include <iostream>

void CommandLineParser::Add(const std::string& name, std::vector<std::string> commands, bool hasValue, const std::string& help)
{
    options[name].commands = commands;
    options[name].help = help;
    if (hasValue)
    {
        options[name].value = "";    
    }
}

void CommandLineParser::PrintHelp() const
{
    std::cout << "Available command line options:\n";
	for (const auto& option : options)
    {
	    std::cout << " ";
		for (size_t i = 0; i < option.second.commands.size(); ++i)
        {
            std::cout << option.second.commands[i];
			if (i < option.second.commands.size() - 1)
            {
				std::cout << ", ";
			}
		}
		std::cout << ": " << option.second.help << "\n";
	}
	std::cout << "Press any key to close...";
}

void CommandLineParser::Parse(int argc, char* argv[])
{
    std::vector<const char*> args;
    for (int i = 0; i < argc; i++)
    {
        args.push_back(argv[i]);
    };
    Parse(args);
}

void CommandLineParser::Parse(std::vector<const char*> arguments)
{
    bool printHelp = false;
    // Known arguments
    for (auto& option : options)
    {
        for (auto& command : option.second.commands)
        {
            for (size_t i = 0; i < arguments.size(); i++)
            {
                if (strcmp(arguments[i], command.c_str()) == 0)
                {
                    option.second.isSet = true;
                    if (option.second.value)
                    {
                        if (arguments.size() > i + 1)
                        {
                            option.second.value = arguments[i + 1];
                        }

                        if (option.second.value && option.second.value == "")
                        {
                            printHelp = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    // Print help for unknown arguments or missing argument values
    if (printHelp) 
    {
        options["help"].isSet = true;
    }
}

bool CommandLineParser::IsSet(const std::string& name) const
{
    auto it = options.find(name);
    return it != options.end() && it->second.isSet;
}

std::string CommandLineParser::GetValueAsString(const std::string& name, const std::string& defaultValue) const
{
    auto it = options.find(name);
    assert(it != options.end());
    std::string value = *(it->second.value);
    return (value != "") ? value : defaultValue;
}

int32_t CommandLineParser::GetValueAsInt(const std::string& name, int32_t defaultValue) const
{
    auto it = options.find(name);
    assert(it != options.end());
    std::string value = *(it->second.value);
    if (value != "") {
        char* numConvPtr;
        int32_t intVal = strtol(value.c_str(), &numConvPtr, 10);
        return (intVal > 0) ? intVal : defaultValue;
    }
    else
    {
        return defaultValue;
    }
    return int32_t();
}
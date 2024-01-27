#include "CommandLineOptions.h"
#include "CommandLineArgs.h"
#include <assert.h>
#include <iostream>

void CommandLineOptions::Add(const std::string& name, std::vector<std::string> commands, bool hasValue, const std::string& help)
{
    options[name].commands = commands;
    options[name].help = help;
    options[name].isSet = false;
    options[name].hasValue = hasValue;
    options[name].value = "";    
}

void CommandLineOptions::PrintHelp() const
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

void CommandLineOptions::Parse(const CommandLineArgs& arguments)
{
    bool printHelp = false;
    // Known arguments
    for (auto& option : options)
    {
        for (auto& command : option.second.commands)
        {
            for (size_t i = 0; i < arguments.args.size(); ++i)
            {
                if (strcmp(arguments.args[i], command.c_str()) == 0)
                {
    				option.second.isSet = true;
					// Get value
					if (option.second.hasValue) 
                    {
						if (arguments.args.size() > i + 1)
                        {
							option.second.value = arguments.args[i + 1];
						}
						if (option.second.value == "")
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
    if (printHelp || options["help"].isSet) 
    {
        PrintHelp();
        std::cin.get();
        exit(0);
    }
}

bool CommandLineOptions::IsSet(const std::string& name) const
{
    auto it = options.find(name);
    return it != options.end() && it->second.isSet;
}

std::string CommandLineOptions::GetValueAsString(const std::string& name, const std::string& defaultValue) const
{
    auto it = options.find(name);
    assert(it != options.end());
    return it->second.value;
}

int32_t CommandLineOptions::GetValueAsInt(const std::string& name, int32_t defaultValue) const
{
    auto it = options.find(name);
    assert(it != options.end());
    if (it->second.value != "") 
    {
        char* numConvPtr;
        int32_t intVal = strtol(it->second.value.c_str(), &numConvPtr, 10);
        return (intVal > 0) ? intVal : defaultValue;
    }
    else
    {
        return defaultValue;
    }
    return int32_t();
}
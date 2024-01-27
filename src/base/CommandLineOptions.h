#pragma once

#include <vector>
#include <unordered_map>
#include <string>

struct CommandLineArgs;

class CommandLineOptions
{
public:
	void Add(const std::string& name, std::vector<std::string> commands, bool hasValue, const std::string& help);
	void PrintHelp() const;
	void Parse(const CommandLineArgs& args);
	bool IsSet(const std::string& name) const;
	std::string GetValueAsString(const std::string& name, const std::string& defaultValue) const;
	int32_t GetValueAsInt(const std::string& name, int32_t defaultValue) const;
private:
	struct CommandLineOption {
		std::vector<std::string> commands;
		std::string help;
		std::string value;
		bool hasValue;
		bool isSet;
	};

	std::unordered_map<std::string, CommandLineOption> options;
};
#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <assert.h>

class CommandLineParser
{
public:
	void Add(const std::string& name, std::vector<std::string> commands, bool hasValue, const std::string& help);
	void PrintHelp() const;
	void Parse(int argc, char* argv[]);
	void Parse(std::vector<const char*> arguments);
	bool IsSet(const std::string& name) const;
	std::string GetValueAsString(const std::string& name, const std::string& defaultValue) const;
	int32_t GetValueAsInt(const std::string& name, int32_t defaultValue) const;
private:
	struct CommandLineOption {
		std::vector<std::string> commands;
		std::string help;
		std::optional<std::string> value;
		bool isSet = false;
	};

	std::unordered_map<std::string, CommandLineOption> options;
};
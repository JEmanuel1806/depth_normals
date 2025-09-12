#pragma once
#include <string>

class CommandLine {
public:
    CommandLine();
    CommandLine(const std::string& exePath);

    void arg(const std::string& argument);
    int executeAndWait();

private:
    std::string m_command;
};

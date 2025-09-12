#include "CommandLine.h"
#include <windows.h>
#include <iostream>

CommandLine::CommandLine()
    : m_command("") {}

CommandLine::CommandLine(const std::string& exePath)
    : m_command(exePath) {}

void CommandLine::arg(const std::string& argument) {
    m_command += " " + argument;
}

int CommandLine::executeAndWait() {
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::wstring wcmd = std::wstring(m_command.begin(), m_command.end());
    wchar_t* cmdLine = wcmd.data();

    if (!CreateProcessW(
        NULL, cmdLine, NULL, NULL,
        FALSE, 0, NULL, NULL,
        &si, &pi))
    {
        std::cerr << "CreateProcess failed. Error: " << GetLastError() << "\n";
        return -1;
    }

    // warten bis der Prozess fertig ist
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
}

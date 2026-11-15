// debug_info.h -- source-level debug information (`csec++ -g`): a compile unit, a subprogram per
// function and lambda, and the source line of every statement, so debuggers (gdb, lldb, Visual
// Studio/WinDbg) can set breakpoints and step by line and show Tessera function names in stack traces.
// CodeView on Windows, DWARF elsewhere. Local variables are not described yet.
#pragma once

#include <string>

namespace llvm { class Function; }

namespace debuginfo {
void enable(const std::string& mainFile, bool codeView);
bool enabled();
// Starts the debug description of `function` (defined at `expandedLine` of the expanded program).
void beginFunction(llvm::Function* function, const std::string& name, int expandedLine);
// The source line of the code generated next.
void setLine(int expandedLine);
// Repairs locations and finishes the debug information; call before verifying the module.
void finalize();
}

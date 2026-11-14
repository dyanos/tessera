#pragma once

// Imports are expanded textually before lexing, so token lines count lines of the expanded program.
// expandImports records where each expanded line came from.

#include <string>
#include <vector>

struct SourceLocation {
    std::string file;
    int line = 0;
};

// Index i holds the origin of expanded line i + 1.
std::vector<SourceLocation>& expandedLineMap();
// The file and line of an expanded-program line (file "" when unknown).
SourceLocation sourceLocationOf(int expandedLine);

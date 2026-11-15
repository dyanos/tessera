// default_arguments.h -- expands default parameter values and named call arguments (see the .cpp).
#pragma once

class ASTNode;

// Rewrites calls in the parsed program into positional calls. Prints diagnostics and returns false
// on an invalid call or declaration.
bool expandDefaultAndNamedArguments(ASTNode* program);

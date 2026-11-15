// default_arguments.h -- default parameter values and named call arguments (see the .cpp).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class ASTNode;

// Checks the declarations of the parsed program: a parameter after one with a default needs one,
// and a default may not refer to the other parameters. Prints diagnostics; false on an error.
bool checkDefaultParameters(ASTNode* program);

// Records every declaration whose parameters a call may name or omit (after enums and interfaces
// were lowered, so their methods are included).
void indexCallSignatures(ASTNode* program);

enum class CallTarget { None, Function, Namespaced, Method, Constructor };

struct CallReport {
    std::function<void(const std::string&)> error;
    // Whether `name` at the call site is a local that hides the declaration-site meaning.
    std::function<bool(const std::string&)> isShadowed;
};

bool hasNamedArguments(const std::vector<std::unique_ptr<ASTNode>>& args);
// Whether `className` or a parent declares a method `method`.
bool hasMethodSignatures(const std::string& className, const std::string& method);

// Whether calling `method` on an object of class `className` may let `this` outlive the call
// (the method returns or stores it, or captures it in a lambda). Unknown methods may.
bool methodMayKeepThis(const std::string& className, const std::string& method);

// Rewrites `args` of a call of `name` (a function, `owner.name` of an object, a method of class
// `owner`, or the constructor of class `owner`) into positional arguments. Reports through `report`
// and returns false on an invalid call.
bool expandCallArguments(std::vector<std::unique_ptr<ASTNode>>& args, CallTarget kind, const std::string& owner,
                         const std::string& name, int line, const CallReport& report);

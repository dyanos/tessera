// symbol_table.cpp

#include "symbol_table.h"
#include "codegen.h"
#include "ast.h"
#include "type_utils.h"
#include "utils.h"

#include "context.h"

#include <functional>
#include <iostream>
#include <memory>

namespace {
std::string makeUniqueKey(const std::unordered_map<std::string, std::unique_ptr<Symbol>>& table, const std::string& base) {
    if (table.count(base) == 0) {
        return base;
    }

    int suffix = 1;
    std::string candidate;
    do {
        candidate = base + "#" + std::to_string(suffix++);
    } while (table.count(candidate) != 0);
    return candidate;
}

// How well `argTypes` fit the function `symbol`: -1 = no match, 0 = exact, higher = needs more
// conversion. Type::equals is loose on numeric types, so same-named types score 0, a numeric
// widening (Int -> Long -> Double) scores its widening cost, and other loose matches score 3.
int functionArgumentCost(const Symbol* symbol, const std::string& name, const std::vector<std::unique_ptr<Type>>& argTypes) {
    if (!symbol || symbol->symbolType != SymbolType::FUNCTION || symbol->name != name) {
        return -1;
    }

    auto* funcType = dynamic_cast<FunctionType*>(symbol->type.get());
    if (!funcType) {
        return -1;
    }

    if (funcType->parameterTypes.size() != argTypes.size()) {
        return -1;
    }

    int cost = 0;
    for (size_t i = 0; i < argTypes.size(); ++i) {
        if (!argTypes[i] || !funcType->parameterTypes[i]) {
            return -1;
        }
        if (argTypes[i]->getName() == funcType->parameterTypes[i]->getName()) {
            continue;
        }
        const int widen = numericWideningCost(*argTypes[i], *funcType->parameterTypes[i]);
        if (widen > 0) {
            cost += widen;
            continue;
        }
        if (!argTypes[i]->equals(funcType->parameterTypes[i]) &&
            !argTypes[i]->isSubtypeOf(funcType->parameterTypes[i])) {
            return -1;
        }
        cost += 3;
    }
    return cost;
}

bool matchesFunctionArguments(const Symbol* symbol, const std::string& name, const std::vector<std::unique_ptr<Type>>& argTypes) {
    return functionArgumentCost(symbol, name, argTypes) >= 0;
}
}


SymbolTable::SymbolTable() {
    //enterScope();
    auto rootScope = std::make_unique<Scope>();
    rootScope->outer = nullptr;
    rootScope->count = 0;
    this->currentScope = rootScope.get();
    this->ownedScopes.push_back(std::move(rootScope));
    this->currentScopeLevel = 1;
}

void SymbolTable::initializeBuiltInTypes(llvm::LLVMContext& context) {
    //auto& ctxt = Context::getInstance();

    auto anyType = std::make_unique<ClassType>("Any");

    //addTypeSymbol("Any", anyType);
    this->currentScope->symbols["Any"] = std::make_unique<ClassSymbol>("Any", llvm::Type::getInt32Ty(context), "System.lang.Object");
    //(static_cast<NamespaceSymbol*>(&root))->classes["Any"] = ClassSymbol("Any", llvm::Type::getInt32Ty(context), "System.lang.Object");

    //auto anyValType = std::make_shared<ClassType>("AnyVal", anyType);
    //addTypeSymbol("AnyVal", anyValType);
    this->currentScope->symbols["AnyVal"] = std::make_unique<ClassSymbol>("AnyVal", llvm::Type::getInt32Ty(context), "System.lang.Object");

    //addTypeSymbol("Int", std::make_shared<BasicType>("Int", anyValType));
    //addTypeSymbol("Float", std::make_shared<BasicType>("Float", anyValType));
    //addTypeSymbol("Double", std::make_shared<BasicType>("Double", anyValType));
    //addTypeSymbol("Char", std::make_shared<BasicType>("Char", anyValType));
    //addTypeSymbol("Boolean", std::make_shared<BasicType>("Boolean", anyValType));
	this->currentScope->symbols["Short"] = std::make_unique<ClassSymbol>("Short", llvm::Type::getInt16Ty(context), "System.lang.Object");
    this->currentScope->symbols["Int"] = std::make_unique<ClassSymbol>("Int", llvm::Type::getInt32Ty(context), "System.lang.Object");
	this->currentScope->symbols["Long"] = std::make_unique<ClassSymbol>("Long", llvm::Type::getInt64Ty(context), "System.lang.Object");
    this->currentScope->symbols["Float"] = std::make_unique<ClassSymbol>("Float", llvm::Type::getFloatTy(context), "System.lang.Object");
    this->currentScope->symbols["Double"] = std::make_unique<ClassSymbol>("Double", llvm::Type::getDoubleTy(context), "System.lang.Object");
    this->currentScope->symbols["Char"] = std::make_unique<ClassSymbol>("Char", llvm::Type::getInt32Ty(context), "System.lang.Object");
    this->currentScope->symbols["Boolean"] = std::make_unique<ClassSymbol>("Boolean", llvm::Type::getInt32Ty(context), "System.lang.Object");

    // Number set alias types
    this->currentScope->symbols["Natural"] = std::make_unique<ClassSymbol>("Natural", llvm::Type::getInt64Ty(context), "System.lang.Object");
    this->currentScope->symbols["Integer"] = std::make_unique<ClassSymbol>("Integer", llvm::Type::getInt64Ty(context), "System.lang.Object");
    this->currentScope->symbols["Real"] = std::make_unique<ClassSymbol>("Real", llvm::Type::getDoubleTy(context), "System.lang.Object");

    // Complex = { double re, double im }
    {
        auto* complexStructTy = llvm::StructType::create(context, { llvm::Type::getDoubleTy(context), llvm::Type::getDoubleTy(context) }, "Complex");
        auto complexSymbol = std::make_unique<StructSymbol>("Complex", complexStructTy);
        complexSymbol->fields["re"] = std::make_unique<Symbol>("re", std::make_unique<BasicType>("Double"), nullptr, false, SymbolType::FIELD);
        complexSymbol->fields["im"] = std::make_unique<Symbol>("im", std::make_unique<BasicType>("Double"), nullptr, false, SymbolType::FIELD);
        this->currentScope->symbols["Complex"] = std::move(complexSymbol);
    }

    // Rational = { i64 numerator, i64 denominator }
    {
        auto* rationalStructTy = llvm::StructType::create(context, { llvm::Type::getInt64Ty(context), llvm::Type::getInt64Ty(context) }, "Rational");
        auto rationalSymbol = std::make_unique<StructSymbol>("Rational", rationalStructTy);
        rationalSymbol->fields["numerator"] = std::make_unique<Symbol>("numerator", std::make_unique<BasicType>("Long"), nullptr, false, SymbolType::FIELD);
        rationalSymbol->fields["denominator"] = std::make_unique<Symbol>("denominator", std::make_unique<BasicType>("Long"), nullptr, false, SymbolType::FIELD);
        this->currentScope->symbols["Rational"] = std::move(rationalSymbol);
    }

    // Quaternion = { double w, double x, double y, double z }
    {
        auto* quaternionStructTy = llvm::StructType::create(context, { llvm::Type::getDoubleTy(context), llvm::Type::getDoubleTy(context), llvm::Type::getDoubleTy(context), llvm::Type::getDoubleTy(context) }, "Quaternion");
        auto quaternionSymbol = std::make_unique<StructSymbol>("Quaternion", quaternionStructTy);
        quaternionSymbol->fields["w"] = std::make_unique<Symbol>("w", std::make_unique<BasicType>("Double"), nullptr, false, SymbolType::FIELD);
        quaternionSymbol->fields["x"] = std::make_unique<Symbol>("x", std::make_unique<BasicType>("Double"), nullptr, false, SymbolType::FIELD);
        quaternionSymbol->fields["y"] = std::make_unique<Symbol>("y", std::make_unique<BasicType>("Double"), nullptr, false, SymbolType::FIELD);
        quaternionSymbol->fields["z"] = std::make_unique<Symbol>("z", std::make_unique<BasicType>("Double"), nullptr, false, SymbolType::FIELD);
        this->currentScope->symbols["Quaternion"] = std::move(quaternionSymbol);
    }

    // Nat = an ordinary natural number, as people use them, but with no size ceiling -- an
    // arbitrary-precision integer. It is a reference type: a value is an opaque heap handle (a `ptr`)
    // backed by the csec_bigint_* runtime, so it grows without bound instead of overflowing at 2^63.
    // Registered as a non-struct ClassSymbol so it reuses the class-operator path and is excluded from
    // scalar auto-load. Its +, -, * and comparisons are lowered in BinaryExpressionNode; an integer
    // literal or value is coerced to a handle wherever a Nat is expected (declarations, returns, args).
    {
        auto* natOpaqueTy = llvm::StructType::create(context, "CsecNat");  // opaque; a Nat value is a ptr to it
        this->currentScope->symbols["Nat"] =
            std::make_unique<ClassSymbol>("Nat", natOpaqueTy, "System.lang.Object");
    }

    auto anyRefType = std::make_unique<ClassType>("AnyRef");
    //addTypeSymbol("AnyRef", anyRefType);
    this->currentScope->symbols["AnyRef"] = std::make_unique<ClassSymbol>("AnyRef", llvm::Type::getInt32Ty(context), "System.lang.Object");

    //addTypeSymbol("String", std::make_shared<ClassType>("String", anyRefType));
    this->currentScope->symbols["String"] = std::make_unique<ClassSymbol>("String", llvm::Type::getInt32Ty(context), "System.lang.Object");

    //addTypeSymbol("Unit", std::make_shared<BasicType>("Unit", anyValType));
    this->currentScope->symbols["Unit"] = std::make_unique<ClassSymbol>("Unit", llvm::Type::getInt32Ty(context), "System.lang.Object");
	this->currentScope->symbols["Void"] = std::make_unique<ClassSymbol>("Void", llvm::Type::getVoidTy(context), "System.lang.Object");

    //auto& intClassSymbol = classSymbols["Int"];

    std::vector<std::unique_ptr<Type>> emptyParams;
    std::unique_ptr<Type> rightCopy = std::make_unique<BasicType>(std::string("String"));
    (static_cast<ClassSymbol*>(this->currentScope->symbols["String"].get()))->methods["toString"] = std::make_unique<Symbol>(
        std::string("toString"),
        std::make_unique<FunctionType>(emptyParams, rightCopy),
        nullptr,
        false,
        SymbolType::METHOD);
}

SymbolTable::~SymbolTable() {
    this->currentScope = nullptr;
}

SymbolTable::SymbolTable(SymbolTable&& other) noexcept
    : currentScope(other.currentScope),
      currentScopeLevel(other.currentScopeLevel),
      currentSymbol(other.currentSymbol),
      ownedScopes(std::move(other.ownedScopes)),
      symbolStack(std::move(other.symbolStack)),
      rejectedSymbols(std::move(other.rejectedSymbols)) {
    other.currentScope = nullptr;
    other.currentScopeLevel = 0;
    other.currentSymbol = nullptr;
}

SymbolTable& SymbolTable::operator=(SymbolTable&& other) noexcept {
    if (this != &other) {
        currentScope = other.currentScope;
        currentScopeLevel = other.currentScopeLevel;
        currentSymbol = other.currentSymbol;
        ownedScopes = std::move(other.ownedScopes);
        symbolStack = std::move(other.symbolStack);
        rejectedSymbols = std::move(other.rejectedSymbols);
        other.currentScope = nullptr;
        other.currentScopeLevel = 0;
        other.currentSymbol = nullptr;
    }
    return *this;
}

bool SymbolTable::addSymbol(const std::string& name, std::unique_ptr<Symbol> symbol) {
    const bool added = insertSymbol(name, symbol);
    if (symbol) rejectedSymbols.push_back(std::move(symbol));
    return added;
}

bool SymbolTable::insertSymbol(const std::string& name, std::unique_ptr<Symbol>& symbol) {
    if (!symbol || !this->currentScope) {
        return false;
    }

    Symbol* rawSymbol = symbol.get();
    CodeGenerator::clearFreshTensor(rawSymbol->value);
    auto& ctxt = Context::getInstance();
    auto current = ctxt.getCurrentNamespace();

    if (this->currentSymbol == nullptr) {
        std::string key = name;
        if (rawSymbol->symbolType == SymbolType::FUNCTION || rawSymbol->symbolType == SymbolType::METHOD) {
            key = makeUniqueKey(this->currentScope->symbols, name);
        }
        else if (this->currentScope->symbols.count(name) != 0) {
            // A module-scope var is bound twice across the two shared-symbol-table passes: once in
            // type-checking (placeholder, value null) and once in codegen (the real global). Refresh
            // the entry so the codegen value wins — mirroring the namespace VARIABLE path below.
            // Without this the top-level global resolves to its null placeholder and reads as 0.
            Symbol* existing = this->currentScope->symbols[name].get();
            if (existing && existing->value == nullptr && rawSymbol->value != nullptr) {
                this->currentScope->symbols[name] = std::move(symbol);
                return true;
            }
            return false;
        }

		this->currentScope->symbols[key] = std::move(symbol);
		return true;
    }

    switch (this->currentSymbol->symbolType) {
    case SymbolType::NAMESPACE:
        {
            auto* target = dynamic_cast<NamespaceSymbol*>(this->currentSymbol);
            if (!target) {
                std::cerr << "Error: Current symbol is not a namespace" << std::endl;
                return false;
            }

            switch (rawSymbol->symbolType) {
            case SymbolType::VARIABLE:
            case SymbolType::FIELD:
                // The namespace's `variables` map persists across the type-check and codegen
                // phases (it lives on the shared NamespaceSymbol), so a field can already be
                // present here when codegen re-adds it. Refresh the entry and always (re)bind
                // the scope symbol, otherwise the codegen value (e.g. the field's global) never
                // lands in the current scope and lookups inside object methods resolve to nothing.
                target->variables[name] = rawSymbol->clone();
                this->currentScope->symbols[name] = std::move(symbol);
                return true;

            case SymbolType::FUNCTION:
            case SymbolType::METHOD:
            {
                auto functionKey = makeUniqueKey(target->functions, name);
                auto scopeKey = makeUniqueKey(this->currentScope->symbols, name);
                target->functions[functionKey] = rawSymbol->clone();
                this->currentScope->symbols[scopeKey] = std::move(symbol);
                return true;
            }

            case SymbolType::CLASS:
            {
                if (target->classes.count(name) != 0)
                    return false;

                auto* classRaw = dynamic_cast<ClassSymbol*>(rawSymbol);
                if (!classRaw) {
                    std::cerr << "Error: Symbol marked as CLASS is not a ClassSymbol" << std::endl;
                    return false;
                }
                target->classes[name] = std::make_unique<ClassSymbol>(*classRaw);
                this->currentScope->symbols[name] = std::move(symbol);
                return true;
            }

            case SymbolType::NAMESPACE:
            {
                if (target->namespaces.count(name) != 0)
                    return false;

                auto* nsRaw = dynamic_cast<NamespaceSymbol*>(rawSymbol);
                if (!nsRaw) {
                    std::cerr << "Error: Symbol marked as NAMESPACE is not a NamespaceSymbol" << std::endl;
                    return false;
                }
                target->namespaces[name] = std::make_unique<NamespaceSymbol>(*nsRaw);
                this->currentScope->symbols[name] = std::move(symbol);
                return true;
            }

            default:
                std::cerr << "Unsupported symbol type for namespace: " << (int)rawSymbol->symbolType << std::endl;
                return false;
            }
        }

        break;

    case SymbolType::CLASS:
        {
            auto* target = dynamic_cast<ClassSymbol*>(this->currentSymbol);
            if (!target) {
                std::cerr << "Error: Current symbol is not a class" << std::endl;
                return false;
            }

            switch (rawSymbol->symbolType) {
            case SymbolType::VARIABLE:
            case SymbolType::FIELD:
                if (target->fields.count(name) != 0)
                    return false;

                target->fields[name] = rawSymbol->clone();
                this->currentScope->symbols[name] = std::move(symbol);
                return true;

            case SymbolType::FUNCTION:
            case SymbolType::METHOD:
            {
                auto methodKey = makeUniqueKey(target->methods, name);
                auto scopeKey = makeUniqueKey(this->currentScope->symbols, name);
                target->methods[methodKey] = rawSymbol->clone();
                this->currentScope->symbols[scopeKey] = std::move(symbol);
                return true;
            }

            default:
                std::cerr << "Unsupported symbol type for class: " << (int)rawSymbol->symbolType << std::endl;
                return false;
            }
        }
        break;

    case SymbolType::FUNCTION:
        {
            switch (rawSymbol->symbolType) {
            case SymbolType::VARIABLE:
            case SymbolType::FIELD:
				this->currentScope->symbols[name] = std::move(symbol);
                return true;
            case SymbolType::FUNCTION:
				this->currentScope->symbols[name] = std::move(symbol);
                return true;
            default:
                std::cerr << "Unsupported symbol type for function: " << (int)rawSymbol->symbolType << std::endl;
                return false;
            }
        }
		break;

    default:
        if (current) {
            std::cerr << "Unsupported current symbol type: " << (int)current->symbolType << std::endl;
        } else {
            std::cerr << "Error: No current namespace available" << std::endl;
        }
        return false;
    }

    return false;
}

Symbol* checkSymbol(Symbol* symbol, const std::string& name) {
    switch (symbol->symbolType) {
    case SymbolType::NAMESPACE:
    {
        auto* target = dynamic_cast<NamespaceSymbol*>(symbol);
        if (!target) return nullptr;

        if (target->namespaces.count(name) != 0) {
            return target->namespaces[name].get();
        }

        if (target->classes.count(name) != 0) {
            return target->classes[name].get();
        }

        if (target->functions.count(name) != 0) {
            return target->functions[name].get();
        }

        if (target->variables.count(name) != 0) {
            return target->variables[name].get();
        }

        return nullptr;
    }
    break;

    case SymbolType::CLASS:
    {
        auto* target = dynamic_cast<ClassSymbol*>(symbol);
        if (!target) return nullptr;

        if (target->methods.count(name) != 0) {
            return target->methods[name].get();
        }
        if (target->fields.count(name) != 0) {
            return target->fields[name].get();
        }
        return nullptr;
    }
    break;

    case SymbolType::VARIABLE:
    case SymbolType::FIELD:
    {
        if (!symbol->type || symbol->type->getKind() != Type::Kind::CLASS) {
            return nullptr;
        }

        auto* classSymbol = CodeGenerator::getInstance().symbolTable.lookupClass(symbol->type->getName());
        if (!classSymbol) {
            return nullptr;
        }

        if (classSymbol->methods.count(name) != 0) {
            return classSymbol->methods[name].get();
        }
        if (classSymbol->fields.count(name) != 0) {
            return classSymbol->fields[name].get();
        }
        if (classSymbol->constructorParams.count(name) != 0) {
            return classSymbol->constructorParams[name].get();
        }
        return nullptr;
    }
    break;

    default:
        std::cerr << "Unsupported symbol type for lookup: " << (int)symbol->symbolType << std::endl;
        break;
    }

    return nullptr;
}

Symbol* SymbolTable::lookupGlobal(const std::string& name) {
    Scope* scope = this->currentScope;
    while (scope && scope->outer) scope = scope->outer;
    if (!scope) return nullptr;
    auto it = scope->symbols.find(name);
    return it == scope->symbols.end() ? nullptr : it->second.get();
}

Symbol* SymbolTable::lookup(const std::string& name) {
    auto names = split(name, '.');

    if (names.size() > 1) {
        // Multi-part name: resolve first part via scope chain, then walk members
        Symbol* current = nullptr;
        Scope* curScope = this->currentScope;
        while (curScope != nullptr) {
            auto it = curScope->symbols.find(names[0]);
            if (it != curScope->symbols.end()) {
                current = it->second.get();
                break;
            }
            curScope = curScope->outer;
        }
        if (!current) return nullptr;

        for (size_t i = 1; i < names.size(); ++i) {
            current = checkSymbol(current, names[i]);
            if (!current) return nullptr;
        }
        return current;
    }

    // Single-part name: scope chain walk
    Scope* curScope = this->currentScope;
    while (curScope != nullptr) {
        for (auto& pair : curScope->symbols) {
            if (pair.first != name) {
                continue;
            }

            return pair.second.get();
        }

        curScope = curScope->outer;
    }

    return nullptr;
}

ClassSymbol* SymbolTable::lookupClass(const std::string& name) {
	auto symbol = lookup(name);
    if (symbol && symbol->symbolType == SymbolType::CLASS) {
        return static_cast<ClassSymbol*>(symbol);
	}
	return nullptr;
}

StructSymbol* SymbolTable::lookupStruct(const std::string& name) {
    auto symbol = lookup(name);
    if (symbol && symbol->symbolType == SymbolType::STRUCT) {
        return static_cast<StructSymbol*>(symbol);
    }
    return nullptr;
}

NamespaceSymbol* SymbolTable::lookupNamespace(const std::string& name) {
    auto symbol = lookup(name);
    if (symbol && symbol->symbolType == SymbolType::NAMESPACE) {
        return static_cast<NamespaceSymbol*>(symbol);
    }
    return nullptr;
}

FunctionSymbol* SymbolTable::lookupFunction(const std::string& name, const std::vector<std::unique_ptr<Type>>& argTypes) {
    // Innermost scope first; within a scope the overload needing the least conversion wins.
    Scope* curScope = this->currentScope;
    while (curScope != nullptr) {
        FunctionSymbol* best = nullptr;
        int bestCost = 0;
        for (const auto& pair : curScope->symbols) {
            Symbol* candidate = pair.second.get();
            const int cost = functionArgumentCost(candidate, name, argTypes);
            if (cost < 0) {
                continue;
            }

            auto* functionSymbol = dynamic_cast<FunctionSymbol*>(candidate);
            if (functionSymbol && (!best || cost < bestCost)) {
                best = functionSymbol;
                bestCost = cost;
            }
        }
        if (best) {
            return best;
        }
        curScope = curScope->outer;
    }

    auto symbol = lookup(name);
    if (symbol && symbol->symbolType == SymbolType::FUNCTION) {
        return dynamic_cast<FunctionSymbol*>(symbol);
    }

    return nullptr;
}

namespace {
bool sameParameterTypes(const Symbol* symbol, const std::vector<std::unique_ptr<Type>>& parameterTypes) {
    auto* funcType = symbol ? dynamic_cast<FunctionType*>(symbol->type.get()) : nullptr;
    if (!funcType || funcType->parameterTypes.size() != parameterTypes.size()) {
        return false;
    }
    for (size_t i = 0; i < parameterTypes.size(); ++i) {
        if (!parameterTypes[i] || !funcType->parameterTypes[i] ||
            parameterTypes[i]->getName() != funcType->parameterTypes[i]->getName()) {
            return false;
        }
    }
    return true;
}
}

FunctionSymbol* SymbolTable::lookupFunctionExact(const std::string& name, const std::vector<std::unique_ptr<Type>>& parameterTypes) {
    for (Scope* scope = this->currentScope; scope != nullptr; scope = scope->outer) {
        for (const auto& pair : scope->symbols) {
            Symbol* candidate = pair.second.get();
            if (candidate && candidate->symbolType == SymbolType::FUNCTION && candidate->name == name &&
                sameParameterTypes(candidate, parameterTypes)) {
                if (auto* functionSymbol = dynamic_cast<FunctionSymbol*>(candidate)) {
                    return functionSymbol;
                }
            }
        }
    }
    return nullptr;
}

std::vector<FunctionSymbol*> SymbolTable::functionOverloads(const std::string& name) {
    std::vector<FunctionSymbol*> result;
    for (Scope* scope = this->currentScope; scope != nullptr; scope = scope->outer) {
        for (const auto& pair : scope->symbols) {
            Symbol* candidate = pair.second.get();
            if (candidate && candidate->symbolType == SymbolType::FUNCTION && candidate->name == name) {
                if (auto* functionSymbol = dynamic_cast<FunctionSymbol*>(candidate)) {
                    result.push_back(functionSymbol);
                }
            }
        }
    }
    return result;
}

FunctionSymbol* SymbolTable::lookupFunctionInNamespace(
    const std::string& namespaceName,
    const std::string& functionName,
    const std::vector<std::unique_ptr<Type>>& argTypes) {
    auto* namespaceSymbol = lookupNamespace(namespaceName);
    if (!namespaceSymbol) {
        return nullptr;
    }

    FunctionSymbol* best = nullptr;
    int bestCost = 0;
    for (const auto& pair : namespaceSymbol->functions) {
        Symbol* candidate = pair.second.get();
        const int cost = functionArgumentCost(candidate, functionName, argTypes);
        if (cost < 0) {
            continue;
        }

        auto* functionSymbol = dynamic_cast<FunctionSymbol*>(candidate);
        if (functionSymbol && (!best || cost < bestCost)) {
            best = functionSymbol;
            bestCost = cost;
        }
    }

    return best;
}

Symbol* SymbolTable::lookupMethod(const ClassSymbol& symbol, const std::string& methodName)
{
    const ClassSymbol* current = &symbol;
    while (current) {
        auto methodIt = current->methods.find(methodName);
        if (methodIt != current->methods.end()) {
            return methodIt->second.get();
        }
        // Methods are stored under a key that includes their parameter types; fall back to the
        // first method with this name (any overload) when the caller has no argument types.
        for (const auto& pair : current->methods) {
            Symbol* candidate = pair.second.get();
            if (candidate && candidate->name == methodName && candidate->symbolType == SymbolType::METHOD) {
                return candidate;
            }
        }
        current = current->superClassSymbol;
    }
    return nullptr;
}

// Cost of the implicit numeric widening allowed when choosing a method overload (0 = not allowed):
// Int -> Long (1) or Double (2), Long -> Double (1), Float -> Double (1). "Real" is Double.
int numericWideningCost(const Type& from, const Type& to) {
    const std::string a = from.getName();
    std::string b = to.getName();
    if (b == "Real") b = "Double";
    if (a == "Int") return b == "Long" ? 1 : b == "Double" ? 2 : 0;
    if (a == "Long") return b == "Double" ? 1 : 0;
    if (a == "Float") return b == "Double" ? 1 : 0;
    return 0;
}

bool isNumericWidening(const Type& from, const Type& to) {
    return numericWideningCost(from, to) > 0;
}

Symbol* SymbolTable::lookupMethod(const ClassSymbol& symbol, const std::string& methodName, const std::vector<std::unique_ptr<Type>>& argTypes)
{
    // An exact (or subtype) match wins. Otherwise the overload needing the cheapest numeric widening
    // is chosen, so `e > 5` picks operator>(Long) over operator>(Double).
    Symbol* best = nullptr;
    int bestCost = 0;
    const ClassSymbol* current = &symbol;
    while (current) {
        for (const auto& pair : current->methods) {
            Symbol* candidate = pair.second.get();
            if (!candidate || candidate->name != methodName ||
                (candidate->symbolType != SymbolType::METHOD && candidate->symbolType != SymbolType::FUNCTION)) {
                continue;
            }
            auto* funcType = dynamic_cast<FunctionType*>(candidate->type.get());
            if (!funcType) continue;
            if (candidate->symbolType == SymbolType::FUNCTION && candidate->value) continue;
            size_t offset = 1;
            if (funcType->parameterTypes.size() == argTypes.size() && !candidate->value) offset = 0;
            else if (funcType->parameterTypes.size() != argTypes.size() + 1) continue;
            int cost = 0;
            bool matches = true;
            for (size_t i = 0; i < argTypes.size(); ++i) {
                const auto& argType = argTypes[i];
                const auto& paramType = funcType->parameterTypes[i + offset];
                if (!argType || !paramType) {
                    matches = false;
                    break;
                }
                // Type::equals is loose on numeric types, so check widening first by name.
                const int widen = argType->getName() != paramType->getName() ? numericWideningCost(*argType, *paramType) : 0;
                if (widen > 0) {
                    cost += widen;
                    continue;
                }
                if ((argType->equals(paramType) && argType->getName() == paramType->getName()) || argType->isSubtypeOf(paramType)) {
                    continue;
                }
                if (argType->equals(paramType)) {
                    cost += 100;
                    continue;
                }
                matches = false;
                break;
            }
            if (!matches) {
                continue;
            }
            if (cost == 0) {
                return candidate;
            }
            if (!best || cost < bestCost) {
                best = candidate;
                bestCost = cost;
            }
        }
        current = current->superClassSymbol;
    }
    return best;
}

void SymbolTable::merge(const SymbolTable& other) {
    return;
}

void SymbolTable::enterScope() {
    auto newScope = std::make_unique<Scope>();
    newScope->outer = this->currentScope;
    this->currentScope = newScope.get();
    this->ownedScopes.push_back(std::move(newScope));
    this->currentScopeLevel += 1;
}

void SymbolTable::exitScope() {
	if (this->currentScope == nullptr) {
        return;
    }
    this->currentScope = this->currentScope->outer;
    this->currentScopeLevel -= 1;
}

void SymbolTable::print(std::ostream& os, int indent) const {
    auto& ctxt = Context::getInstance();
    auto root = ctxt.getRootNamespace();
    std::function<void(const NamespaceSymbol*, int)> printNamespace;
    printNamespace = [&](const NamespaceSymbol* ns, int level) {
        std::string indentStr(level * 2, ' ');
        os << indentStr << "Namespace: " << ns->name << "\n";
        for (const auto& varPair : ns->variables) {
            os << indentStr << "  Variable: " << varPair.first << "\n";
        }
        for (const auto& funcPair : ns->functions) {
            os << indentStr << "  Function: " << funcPair.first << "\n";
        }
        for (const auto& classPair : ns->classes) {
            os << indentStr << "  Class: " << classPair.first << "\n";
            auto* classSym = classPair.second.get();
            for (const auto& fieldPair : classSym->fields) {
                os << indentStr << "    Field: " << fieldPair.first << "\n";
            }
            for (const auto& methodPair : classSym->methods) {
                os << indentStr << "    Method: " << methodPair.first << "\n";
            }
        }
        for (const auto& nsPair : ns->namespaces) {
            printNamespace(nsPair.second.get(), level + 1);
        }
    };
    printNamespace(root, indent);
}


#include "parser.h"
#include "enum_nodes.h"

#include "all_ast.h"
#include "utils.h"

#include <algorithm>
#include <cctype>

namespace {
std::string lowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string mapSystemPathToBuiltin(const std::vector<std::string>& pathComponents) {
    if (pathComponents.empty()) {
        return "";
    }

    auto equalsPath = [&](std::initializer_list<const char*> parts) {
        if (pathComponents.size() != parts.size()) return false;
        size_t index = 0;
        for (const char* part : parts) {
            if (pathComponents[index++] != part) return false;
        }
        return true;
    };

    auto mapMathName = [](const std::string& rawName) -> std::string {
        std::string name = lowerAscii(rawName);
        if (name == "ceiling") return "ceil";
        if (name == "log10") return "lg";
        if (name == "asin") return "arcsin";
        if (name == "acos") return "arccos";
        if (name == "atan") return "arctan";
        static const char* supported[] = {
            "sin", "cos", "tan", "cot", "sec", "csc", "arcsin", "arccos", "arctan",
            "sinh", "cosh", "tanh", "coth", "sqrt", "ln", "log", "lg", "exp",
            "frac", "binom", "min", "max", "gcd", "pow", "abs", "sign", "floor",
            "ceil", "round", "lcm", "approxeq", "clamp", "between"
        };
        for (const char* candidate : supported) {
            if (name == candidate) {
                return name == "approxeq" ? "approxEq" : name;
            }
        }
        return "";
    };

    const std::string& method = pathComponents.back();
    if (pathComponents.size() == 3 &&
        pathComponents[0] == "System" &&
        pathComponents[1] == "Console") {
        if (method == "Write" || method == "write" || method == "print") return "print";
        if (method == "WriteLine" || method == "writeln" || method == "println") return "println";
        if (method == "ReadLine" || method == "readLine") return "readLine";
        if (method == "Read" || method == "ReadKey" || method == "read" || method == "readChar") return "readChar";
        if (method == "ReadInt" || method == "readInt") return "readInt";
        if (method == "ReadDouble" || method == "readDouble") return "readDouble";
    }

    if (pathComponents.size() == 3 &&
        pathComponents[0] == "System" &&
        pathComponents[1] == "Math") {
        return mapMathName(method);
    }

    if (pathComponents.size() == 4 &&
        pathComponents[0] == "System" &&
        pathComponents[1] == "IO" &&
        pathComponents[2] == "File") {
        if (method == "ReadAllText" || method == "readAllText") return "systemFileReadAllText";
        if (method == "WriteAllText" || method == "writeAllText") return "systemFileWriteAllText";
        if (method == "AppendAllText" || method == "appendAllText") return "systemFileAppendAllText";
        if (method == "Exists" || method == "exists") return "systemFileExists";
        if (method == "Delete" || method == "delete") return "systemFileDelete";
    }

    if (pathComponents.size() == 3 &&
        pathComponents[0] == "System" &&
        pathComponents[1] == "Environment") {
        if (method == "GetEnvironmentVariable") return "posixGetenv";
        if (method == "CommandLineArgCount" || method == "commandLineArgCount") return "commandLineArgCount";
        if (method == "CommandLineArg" || method == "commandLineArg") return "commandLineArg";
    }

    if (pathComponents.size() == 4 &&
        pathComponents[0] == "System" &&
        pathComponents[1] == "Net" &&
        pathComponents[2] == "Tcp") {
        if (method == "Connect" || method == "connect") return "tcpConnect";
        if (method == "Listen" || method == "listen") return "tcpListen";
        if (method == "Accept" || method == "accept") return "tcpAccept";
        if (method == "Send" || method == "send") return "tcpSend";
        if (method == "Recv" || method == "recv") return "tcpRecv";
        if (method == "Close" || method == "close") return "tcpClose";
    }

    return "";
}
}

// Fold a dotted path (`a`, `a.b`, `a.b.c`, ...) into a left-nested AccessFieldNode chain so member
// access is a first-class l-value/expression AST rather than a joined "a.b.c" identifier string.
static std::unique_ptr<ASTNode> buildFieldChain(const std::vector<std::string>& comps) {
    std::unique_ptr<ASTNode> node = std::make_unique<IdentifierNode>(comps[0]);
    for (size_t i = 1; i < comps.size(); ++i) {
        node = std::make_unique<AccessFieldNode>(std::move(node), std::make_unique<IdentifierNode>(comps[i]));
    }
    return node;
}

// A dotted path is a member-access l-value/expression (build a field chain) rather than a namespaced
// name when it has >= 2 components and does not start with `this`/`super` (those keep their special
// dotted-identifier handling).
static bool isFieldChainPath(const std::vector<std::string>& comps) {
    return comps.size() >= 2 && comps[0] != "this" && comps[0] != "super";
}

std::unique_ptr<ASTNode> Parser::parseSimpleExpression() {
	std::unique_ptr<ASTNode> expr;

	if (match(TokenType::KEYWORD, "if")) {
		return parseIfStatement();
	}
    else if (match(TokenType::KEYWORD, "build")) {
        return parseStringBuildExpression();
    }
    else if (match(TokenType::KEYWORD, "molecule")) {
        return parseMoleculeSimulationExpression();
    }
    else if (match(TokenType::KEYWORD, "cfd")) {
        return parseCfdSimulationExpression();
    }
    else if (match(TokenType::KEYWORD, "protein")) {
        return parseProteinMcmcExpression();
    }
	else if (match(TokenType::KEYWORD, "ode")) {
        return parseOdeSimulationExpression();
    }
    else if (match(TokenType::KEYWORD, "map")) {
        return parseMapStatement();
    }
    else if (match(TokenType::KEYWORD, "pmap")) {
        return parsePMapStatement();
    }
    else if (match(TokenType::KEYWORD, "reduce")) {
        return parseReduceStatement();
    }
    else if (match(TokenType::KEYWORD, "preduce")) {
        return parsePReduceStatement();
    }
    else if (match(TokenType::KEYWORD, "filter")) {
        return parseFilterStatement();
    }
	else if (check(TokenType::IDENTIFIER) ||
        (check(TokenType::KEYWORD, "this") ||
         check(TokenType::KEYWORD, "super") ||
         check(TokenType::KEYWORD, "box"))) {
        advance();
        if (previous().value == "_") {
            return std::make_unique<UnitNode>();
        }
		std::vector<std::string> pathComponents;
		pathComponents.push_back(previous().value);
		while (match(TokenType::OPERATOR, ".")) {
			if (matchMemberName()) {
				pathComponents.push_back(previous().value);
			}
			else {
				error("Expected identifier after '.'");
				return nullptr;
			}
		}

		// Check for explicit template arguments: identifier<Type, ...>(args)
		if (pathComponents.size() == 1 &&
            check(TokenType::OPERATOR, "<") &&
            (peek(1).type == TokenType::IDENTIFIER ||
             peek(1).type == TokenType::INTEGER_LITERAL ||
             peek(1).type == TokenType::FLOAT_LITERAL ||
             peek(1).type == TokenType::EXPONENTIAL_LITERAL ||
             peek(1).type == TokenType::HEX_LITERAL ||
             peek(1).type == TokenType::BINARY_LITERAL ||
             peek(1).type == TokenType::OCTAL_LITERAL ||
             peek(1).type == TokenType::BOOLEAN_LITERAL ||
             (peek(1).type == TokenType::OPERATOR && peek(1).value == "("))) {
			saveTokenPosition();
			advance(); // consume '<'
			std::vector<std::unique_ptr<Type>> typeArgs;
			bool isTemplateCall = false;
			// Try to parse template arg list followed by '>' and '('
			auto firstType = parseTemplateArgumentAsType();
			if (firstType && firstType->getKind() != Type::Kind::UNKNOWN) {
				typeArgs.push_back(std::move(firstType));
				while (match(TokenType::OPERATOR, ",")) {
					typeArgs.push_back(parseTemplateArgumentAsType());
				}
				if (match(TokenType::OPERATOR, ">")) {
					if (match(TokenType::OPERATOR, "(")) {
						isTemplateCall = true;
					}
					else if (check(TokenType::OPERATOR, ".") && peek(1).type == TokenType::IDENTIFIER) {
						// `Option<Int>.None`, `Result<Int, String>.Ok(1)`: a variant of a generic enum.
						advance();  // '.'
						discardTokenPosition();
						auto construct = std::make_unique<EnumConstructNode>();
						construct->enumName = pathComponents[0];
						construct->variantName = advance().value;
						construct->typeArguments = std::move(typeArgs);
						construct->qualified = true;
						construct->line = previous().line;
						if (match(TokenType::OPERATOR, "(")) {
							construct->arguments = parseCallParameterList();
						}
						return construct;
					}
				}
			}
			if (isTemplateCall) {
				discardTokenPosition();
				auto callNode = std::make_unique<FunctionCallNode>();
				callNode->functionName = pathComponents[0];
				callNode->explicitTypeArgs = std::move(typeArgs);
				callNode->arguments = parseCallParameterList();
				return callNode;
			}
			else {
				restoreTokenPosition();
				typeArgs.clear();
			}
		}

        int derivativeOrder = 0;
        if (pathComponents.size() == 1) {
            while (match(TokenType::OPERATOR, "'")) {
                ++derivativeOrder;
            }
        }

        if (derivativeOrder > 0) {
            if (derivativeOrder > 2) {
                error("Only first and second derivative postfix syntax is supported");
            }
            expect(TokenType::OPERATOR, "(");
            auto derivativeArgs = parseCallParameterList();
            if (derivativeArgs.size() != 1) {
                error("Derivative call syntax requires exactly one argument");
                return nullptr;
            }

            auto callNode = std::make_unique<FunctionCallNode>();
            callNode->functionName = "numericDerivative";
            callNode->arguments.push_back(std::make_unique<IdentifierNode>(pathComponents[0]));
            callNode->arguments.push_back(std::make_unique<ValueNode>(std::to_string(derivativeOrder), TokenType::INTEGER_LITERAL));
            callNode->arguments.push_back(std::move(derivativeArgs[0]));
            return callNode;
        }

		if (match(TokenType::OPERATOR, "(")) {
            if (auto builtinName = mapSystemPathToBuiltin(pathComponents); !builtinName.empty()) {
                auto callNode = std::make_unique<FunctionCallNode>();
                callNode->functionName = builtinName;
                callNode->arguments = parseCallParameterList();
                return callNode;
            }
			if (pathComponents.size() == 1) {
				auto callNode = std::make_unique<FunctionCallNode>();
				callNode->functionName = pathComponents[0];
				callNode->arguments = parseCallParameterList();
				return callNode;
			}
			else {
				auto callNode = std::make_unique<MethodCallNode>();

				std::vector<std::string> result;
				std::copy(pathComponents.begin(), pathComponents.end() - 1, std::back_inserter(result));

				callNode->object = std::make_unique<IdentifierNode>(join(result, "."));
				callNode->methodName = pathComponents.back();
				callNode->arguments = parseCallParameterList();

				return callNode;
			}
		}
		else if (match(TokenType::OPERATOR, "=") || match(TokenType::OPERATOR, "<-")) {
            std::string op = previous().value;
			auto assignNode = std::make_unique<AssignmentExpressionNode>();
            assignNode->op = op;
			// `base.member = ...` (any depth) targets a field/vector-component chain; build an
			// AccessFieldNode l-value so the assignment resolves the element address. `this`/`super`
			// member writes keep their existing dotted-identifier handling.
			if (isFieldChainPath(pathComponents)) {
				assignNode->left = buildFieldChain(pathComponents);
			}
			else {
				assignNode->left = std::make_unique<IdentifierNode>(join(pathComponents, "."));
			}
			assignNode->right = parseExpression();
			return assignNode;
		}
        else if (match(TokenType::OPERATOR, "+=") ||
            match(TokenType::OPERATOR, "-=") ||
            match(TokenType::OPERATOR, "*=") ||
            match(TokenType::OPERATOR, "/=") ||
            match(TokenType::OPERATOR, "%=")) {
            std::string op = previous().value.substr(0, 1);
            auto assignNode = std::make_unique<AssignmentExpressionNode>();
            // `base.member OP= x` desugars to `base.member = base.member OP x`; build the target and the
            // read-back as AccessFieldNode l-values (matching plain `=`) rather than a dotted identifier,
            // so the field/component address resolves. `this`/`super` keep dotted-identifier handling.
            auto makeTarget = [&]() -> std::unique_ptr<ASTNode> {
                if (isFieldChainPath(pathComponents)) {
                    return buildFieldChain(pathComponents);
                }
                return std::make_unique<IdentifierNode>(join(pathComponents, "."));
            };
            assignNode->left = makeTarget();
            assignNode->right = std::make_unique<BinaryExpressionNode>(
                makeTarget(),
                op,
                parseExpression());
            return assignNode;
        }

        // Read-back of a dotted path: build an AccessFieldNode chain for any depth >= 2, including
        // `this`/`super` member reads (e.g. `return this.value`). Unlike the assignment LHS, reads have
        // no special dotted-identifier handling for this/super, so they must resolve via the chain too.
        if (pathComponents.size() >= 2) {
            return buildFieldChain(pathComponents);
        }

		return std::make_unique<IdentifierNode>(join(pathComponents, "."));
	}
	else if (match(TokenType::INTEGER_LITERAL) ||
		match(TokenType::FLOAT_LITERAL) ||
		match(TokenType::EXPONENTIAL_LITERAL) ||
		match(TokenType::HEX_LITERAL) ||
		match(TokenType::BINARY_LITERAL) ||
		match(TokenType::OCTAL_LITERAL) ||
        match(TokenType::CHAR_LITERAL) ||
		match(TokenType::STRING_LITERAL) ||
        match(TokenType::REGEX_LITERAL) ||
		match(TokenType::BOOLEAN_LITERAL)) {
		return std::make_unique<ValueNode>(previous().value, previous().type);
	}
	else if (match(TokenType::OPERATOR, "$")) {
		expr = parseInlineMathLatex();
		match(TokenType::OPERATOR, "$");
		return expr;
	}
	else if (match(TokenType::OPERATOR, "$$")) {
		expr = parseBlockMathLatex();
		match(TokenType::OPERATOR, "$$");
		return expr;
	}
	else if (match(TokenType::OPERATOR, "_")) {
		return std::make_unique<UnitNode>();
	}
	else if (match(TokenType::OPERATOR, "{")) {
		if (looksLikeDictLiteral()) {
			return parseDictLiteral();
		}
		if (looksLikeSetLiteral()) {
			return parseSetLiteral();
		}
		return parseBlock();
	}
	else if (check(TokenType::OPERATOR, "[")) {
        // A lambda starts with a capture list -- identifiers, `&`, `=` and commas -- closed by `]` and
        // followed by `(`. Anything else (`[f(x), 1]`, `[a + b]`) is an array literal.
        bool looksLikeLambda = false;
        for (int k = 1; position + k < tokens.size(); ++k) {
            const Token& t = peek(k);
            if (t.type == TokenType::OPERATOR && t.value == "]") {
                looksLikeLambda = peek(k + 1).type == TokenType::OPERATOR && peek(k + 1).value == "(";
                break;
            }
            const bool captureToken = t.type == TokenType::IDENTIFIER ||
                (t.type == TokenType::KEYWORD && t.value == "mut") ||
                (t.type == TokenType::OPERATOR && (t.value == "&" || t.value == "=" || t.value == ","));
            if (!captureToken) break;
        }

        if (looksLikeLambda) {
		    saveTokenPosition();
		    auto lambdaExpr = parseLambdaExpression();
		    if (lambdaExpr) {
			    discardTokenPosition();
			    return lambdaExpr;
		    }
            restoreTokenPosition();
        }

		advance(); // consume '['
		std::vector<std::unique_ptr<ASTNode>> elements;
		if (!check(TokenType::OPERATOR, "]")) {
			do {
				elements.push_back(parseExpression());
			} while (match(TokenType::OPERATOR, ","));
		}
		expect(TokenType::OPERATOR, "]");
		auto node = std::make_unique<ArrayLiteralNode>();
		node->elements = std::move(elements);
		return node;
	}
	else if (match(TokenType::KEYWORD, "new")) {
		if (match(TokenType::IDENTIFIER)) {
			auto id = previous().value;

			// Check for template arguments: new ClassName<Type, ...>(args)
			std::vector<std::unique_ptr<Type>> tmplArgs;
			if (match(TokenType::OPERATOR, "<")) {
				do {
					tmplArgs.push_back(parseTemplateArgumentAsType());
				} while (match(TokenType::OPERATOR, ","));
				expectClosingAngle();
			}

			if (match(TokenType::OPERATOR, "[")) {
				std::vector<std::unique_ptr<ASTNode>> sizes;
				while (!check(TokenType::OPERATOR, "]")) {
					sizes.push_back(parseExpression());
					if (!match(TokenType::OPERATOR, ",")) {
						break;
					}
				}
				expect(TokenType::OPERATOR, "]");
				auto node = std::make_unique<ArrayCreationExpressionNode>(id);
				node->sizes = std::move(sizes);
				return node;
			}
			else if (match(TokenType::OPERATOR, "(")) {
				auto node = std::make_unique<ClassInstanceCreationNode>(id, parseArgumentList());
				node->templateArgs = std::move(tmplArgs);
				return node;
			}
			else {
				auto node = std::make_unique<ClassInstanceCreationNode>(id);
				node->templateArgs = std::move(tmplArgs);
				return node;
			}
		}
		else {
			error("Expected class name after 'new'");
			return nullptr;
		}
	}
	else {
		return nullptr;
	}

	return expr;
}

std::unique_ptr<ASTNode> Parser::parseAssignmentExpression() {
	if (match(TokenType::OPERATOR, "=")) {
		return std::make_unique<AssignmentExpressionNode>(parseExpression(), parseExpression());
	}
	else {
		return parseSimpleExpression();
	}
}

std::unique_ptr<ASTNode> Parser::parsePrimaryExpression() {
	std::unique_ptr<ASTNode> expr = parseSimpleExpression();
	if (expr != nullptr) {
		return expr;
	}

	if (match(TokenType::OPERATOR, "(")) {
		auto grouped = parseExpression();
		if (match(TokenType::OPERATOR, ",")) {
			auto tuple = std::make_unique<TupleExpressionNode>();
			tuple->elements.push_back(std::move(grouped));
			do {
				tuple->elements.push_back(parseExpression());
			} while (match(TokenType::OPERATOR, ","));
			expect(TokenType::OPERATOR, ")");
			return tuple;
		}
		expect(TokenType::OPERATOR, ")");
		return grouped;
	}
	else {
		return parseSimpleExpression();
	}
}

std::unique_ptr<ASTNode> Parser::parseMulDivExpression() {
	std::unique_ptr<ASTNode> expr = parsePrefixExpression();
	while (true) {
        bool matched = match(TokenType::OPERATOR, "*") ||
            match(TokenType::OPERATOR, "**") ||
            match(TokenType::OPERATOR, "/") ||
            match(TokenType::OPERATOR, "%") ||
            match(TokenType::OPERATOR, "@");
        if (!matched &&
            (check(TokenType::IDENTIFIER, "inner") ||
             check(TokenType::IDENTIFIER, "outer") ||
             check(TokenType::IDENTIFIER, "tensor") ||
             check(TokenType::IDENTIFIER, "intersect") ||
             check(TokenType::IDENTIFIER, "div") ||
             check(TokenType::IDENTIFIER, "mod"))) {
            advance();
            matched = true;
        }
        if (!matched) {
            break;
        }
		std::string op = previous().value;
		auto rightCopy = parsePrefixExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseAddSubExpression() {
	std::unique_ptr<ASTNode> expr = parseMulDivExpression();
	while (true) {
        bool matched = match(TokenType::OPERATOR, "+") || match(TokenType::OPERATOR, "-");
        if (!matched &&
            (check(TokenType::IDENTIFIER, "union") ||
             check(TokenType::IDENTIFIER, "diff") ||
             check(TokenType::IDENTIFIER, "without") ||
             check(TokenType::IDENTIFIER, "symdiff"))) {
            advance();
            matched = true;
        }
        if (!matched) {
            break;
        }
		std::string op = previous().value;
		auto rightCopy = parseMulDivExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseComparisonExpression() {
	std::unique_ptr<ASTNode> expr = parseShiftExpression();
	while (true) {
        bool matched = match(TokenType::OPERATOR, "<") || match(TokenType::OPERATOR, ">") ||
		    match(TokenType::OPERATOR, "<=") || match(TokenType::OPERATOR, ">=");
        if (!matched &&
            (check(TokenType::IDENTIFIER, "in") ||
             check(TokenType::IDENTIFIER, "notin") ||
             check(TokenType::IDENTIFIER, "subset") ||
             check(TokenType::IDENTIFIER, "superset") ||
             check(TokenType::IDENTIFIER, "properSubset") ||
             check(TokenType::IDENTIFIER, "properSuperset"))) {
            advance();
            matched = true;
        }
        if (!matched) {
            break;
        }
		std::string op = previous().value;
		auto rightCopy = parseShiftExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseShiftExpression() {
	std::unique_ptr<ASTNode> expr = parseAddSubExpression();
	while (match(TokenType::OPERATOR, "<<") || match(TokenType::OPERATOR, ">>")) {
		std::string op = previous().value;
		auto rightCopy = parseAddSubExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseEqualityExpression() {
	std::unique_ptr<ASTNode> expr = parseComparisonExpression();
	while (true) {
        // Token-pattern sugar: `token(tokens, ordinal) is keyword("if")`.
        // It is lowered before type checking to the native token matcher.
        if (check(TokenType::IDENTIFIER, "is")) {
            auto* tokenCall = dynamic_cast<FunctionCallNode*>(expr.get());
            if (!tokenCall || tokenCall->functionName != "token" || tokenCall->arguments.size() != 2) break;
            advance();
            if (!(check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD))) {
                error("Expected token pattern kind after 'is'");
                return nullptr;
            }
            std::string patternKind = peek().value;
            advance();
            expect(TokenType::OPERATOR, "(");
            if (!check(TokenType::STRING_LITERAL)) {
                error("Expected string literal token pattern");
                return nullptr;
            }
            std::string patternText = peek().value;
            advance();
            expect(TokenType::OPERATOR, ")");
            char kindCode = 'I';
            if (patternKind == "keyword") kindCode = 'K';
            else if (patternKind == "operator") kindCode = 'O';
            else if (patternKind == "string") kindCode = 'S';
            else if (patternKind == "integer") kindCode = 'N';
            else if (patternKind == "comment") kindCode = 'M';
            else if (patternKind != "identifier") {
                error("Unknown token pattern kind: " + patternKind);
                return nullptr;
            }
            auto matchCall = std::make_unique<FunctionCallNode>();
            matchCall->functionName = "__token_matches";
            matchCall->arguments.push_back(std::move(tokenCall->arguments[0]));
            matchCall->arguments.push_back(std::move(tokenCall->arguments[1]));
            matchCall->arguments.push_back(std::make_unique<ValueNode>(std::string("'") + kindCode + "'", TokenType::CHAR_LITERAL));
            matchCall->arguments.push_back(std::make_unique<ValueNode>(patternText, TokenType::STRING_LITERAL));
            expr = std::move(matchCall);
            continue;
        }
        bool matched = match(TokenType::OPERATOR, "==") || match(TokenType::OPERATOR, "!=") || check(TokenType::IDENTIFIER, "iff");
        if (!matched) break;
        if (check(TokenType::IDENTIFIER, "iff")) {
            advance();
        }
		std::string op = previous().value;
		auto rightCopy = parseComparisonExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseConditionExpression() {
	auto expr = parseEqualityExpression();
	while (match(TokenType::KEYWORD, "and") || match(TokenType::KEYWORD, "or") || match(TokenType::KEYWORD, "xor")) {
		std::string op = previous().value;
		auto rightCopy = parseEqualityExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseOrExpression() {
	std::unique_ptr<ASTNode> expr = parseAndExpression();
	while (match(TokenType::KEYWORD, "or") || check(TokenType::IDENTIFIER, "implies")) {
        if (check(TokenType::IDENTIFIER, "implies")) {
            advance();
        }
		std::string op = previous().value;
		auto rightCopy = parseAndExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseAndExpression() {
	std::unique_ptr<ASTNode> expr = parseBitwiseOrExpression();
	while (match(TokenType::KEYWORD, "and")) {
		std::string op = previous().value;
		auto rightCopy = parseBitwiseOrExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseXorExpression() {
	std::unique_ptr<ASTNode> expr = parseBitwiseAndExpression();
	while (match(TokenType::KEYWORD, "xor") || match(TokenType::OPERATOR, "^")) {
		std::string op = previous().value;
		auto rightCopy = parseBitwiseAndExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseBitwiseOrExpression() {
	std::unique_ptr<ASTNode> expr = parseXorExpression();
	while (match(TokenType::OPERATOR, "|")) {
		std::string op = previous().value;
		auto rightCopy = parseXorExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseBitwiseAndExpression() {
	std::unique_ptr<ASTNode> expr = parseEqualityExpression();
	while (match(TokenType::OPERATOR, "&")) {
		std::string op = previous().value;
		auto rightCopy = parseEqualityExpression();
		expr = std::make_unique<BinaryExpressionNode>(std::move(expr), op, std::move(rightCopy));
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parseUnaryExpression() {
	if (match(TokenType::OPERATOR, "-") ||
		match(TokenType::OPERATOR, "+") ||
		match(TokenType::OPERATOR, "*") ||
		match(TokenType::OPERATOR, "~") ||
		match(TokenType::OPERATOR, "!")) {
		std::string op = previous().value;
		// The operand includes its postfix chain: `!s.contains(x)` is `!(s.contains(x))`, and
		// `-a[i]` is `-(a[i])` (postfix binds tighter than prefix, as in C).
		auto rightCopy = parsePostfixExpression();
		return std::make_unique<UnaryExpressionNode>(op, std::move(rightCopy));
	}
	else {
		return parsePrimaryExpression();
	}
}

std::unique_ptr<ASTNode> Parser::parsePostfixExpression() {
	auto expr = parseUnaryExpression();
	while (true) {
		if (match(TokenType::OPERATOR, "++") || match(TokenType::OPERATOR, "--")) {
			std::string op = previous().value;
			expr = std::make_unique<PostfixExpressionNode>(op, std::move(expr));
		}
		else if (match(TokenType::OPERATOR, "?")) {
			// `expr?`: unwrap an Option/Result or return its None/Err from the enclosing function.
			auto tryNode = std::make_unique<TryNode>();
			tryNode->line = previous().line;
			tryNode->expression = std::move(expr);
			expr = std::move(tryNode);
		}
		else if (match(TokenType::OPERATOR, "[")) {
            if (check(TokenType::OPERATOR, "]") || check(TokenType::OPERATOR, ",")) {
                error("Expected index expression");
            }
            auto accessNode = std::make_unique<ArrayAccessNode>();
            accessNode->array = std::move(expr);
            do {
                auto spec = std::make_unique<ArrayIndexSpec>();
                if (match(TokenType::OPERATOR, ":")) {
                    spec->isSlice = true;
                    if (!check(TokenType::OPERATOR, "]") &&
                        !check(TokenType::OPERATOR, ",") &&
                        !check(TokenType::OPERATOR, ":")) {
                        spec->end = parseExpression();
                    }
                    if (match(TokenType::OPERATOR, ":")) {
                        if (!check(TokenType::OPERATOR, "]") && !check(TokenType::OPERATOR, ",")) {
                            spec->step = parseExpression();
                        }
                    }
                }
                else {
                    auto first = parseExpression();
                    if (match(TokenType::OPERATOR, ":")) {
                        spec->isSlice = true;
                        spec->start = std::move(first);
                        if (!check(TokenType::OPERATOR, "]") &&
                            !check(TokenType::OPERATOR, ",") &&
                            !check(TokenType::OPERATOR, ":")) {
                            spec->end = parseExpression();
                        }
                        if (match(TokenType::OPERATOR, ":")) {
                            if (!check(TokenType::OPERATOR, "]") && !check(TokenType::OPERATOR, ",")) {
                                spec->step = parseExpression();
                            }
                        }
                    }
                    else {
                        spec->index = std::move(first);
                    }
                }
                if (!accessNode->index && !spec->isSlice && spec->index) {
                    accessNode->index = spec->index->clone();
                }
                accessNode->indices.push_back(std::move(spec));
                if (check(TokenType::OPERATOR, "]")) {
                    break;
                }
                expect(TokenType::OPERATOR, ",");
                if (check(TokenType::OPERATOR, "]")) {
                    error("Expected index expression after ','");
                }
            } while (true);
			expect(TokenType::OPERATOR, "]");
            expr = std::move(accessNode);
		}
		else if (match(TokenType::OPERATOR, ".")) {
			// Member access on an arbitrary expression, e.g. a method call on a string literal
			// (`" x ".trim()`). Identifier-rooted paths are handled in parseSimpleExpression;
			// this covers the cases that reach here as a completed primary expression.
			if (!matchMemberName()) {
				error("Expected member name after '.'");
				break;
			}
			std::string member = previous().value;
			if (match(TokenType::OPERATOR, "(")) {
				auto methodCall = std::make_unique<MethodCallNode>();
				methodCall->object = std::move(expr);
				methodCall->methodName = member;
				methodCall->arguments = parseCallParameterList();
				expr = std::move(methodCall);
			}
			else {
				expr = std::make_unique<AccessFieldNode>(std::move(expr), std::make_unique<IdentifierNode>(member));
			}
		}
		else {
			break;
		}
	}
	return expr;
}

std::unique_ptr<ASTNode> Parser::parsePrefixExpression()
{
	if (match(TokenType::OPERATOR, "++") || match(TokenType::OPERATOR, "--")) {
		std::string op = previous().value;
		auto rightCopy = parsePrefixExpression();
		return std::make_unique<PrefixExpressionNode>(op, std::move(rightCopy));
	}
	else if (match(TokenType::OPERATOR, "<-")) {
		auto rightCopy = parsePrefixExpression();
		return std::make_unique<PrefixExpressionNode>("<-", std::move(rightCopy));
	}
	else if (match(TokenType::KEYWORD, "async")) {
		// `async f(args)` runs the call in a new microthread and yields a Future<T> for `await`.
		auto rightCopy = parsePrefixExpression();
		return std::make_unique<PrefixExpressionNode>("async", std::move(rightCopy));
	}
	else if (match(TokenType::KEYWORD, "go")) {
		// `go f(args)` runs the call in a new microthread, discarding its result; yields the pid.
		auto rightCopy = parsePrefixExpression();
		return std::make_unique<PrefixExpressionNode>("go", std::move(rightCopy));
	}
	else if (match(TokenType::KEYWORD, "await")) {
		// `await expr` suspends until expr's async result is ready. Modelled as a prefix op so no new
		// AST node is needed; the type checker enforces async-context + the no-borrow-across-await
		// rule (M8), and codegen lowers it to the operand value (sequential evaluation for now).
		auto rightCopy = parsePrefixExpression();
		return std::make_unique<PrefixExpressionNode>("await", std::move(rightCopy));
	}
	else if (match(TokenType::OPERATOR, "&")) {
		std::string op = match(TokenType::KEYWORD, "mut") ? "&mut" : "&";
		auto rightCopy = parsePrefixExpression();
		return std::make_unique<PrefixExpressionNode>(op, std::move(rightCopy));
	}
	else {
		return parsePostfixExpression();
	}
}

std::unique_ptr<ASTNode> Parser::parseStringBuildExpression() {
    expect(TokenType::OPERATOR, "{");
    std::unique_ptr<ASTNode> result = std::make_unique<ValueNode>("", TokenType::STRING_LITERAL);
    while (!check(TokenType::OPERATOR, "}") && !isAtEnd()) {
        auto segment = parseExpression();
        if (!segment) return nullptr;
        result = std::make_unique<BinaryExpressionNode>(std::move(result), "+", std::move(segment));
        match(TokenType::OPERATOR, ";");
    }
    expect(TokenType::OPERATOR, "}");
    return result;
}

namespace {
// `value >>| f(a)` and `f(a) <<| value` both become `f(value, a)`.
// Desugaring here preserves the usual call type checking and overload lookup.
std::unique_ptr<ASTNode> applyPipeline(std::unique_ptr<ASTNode> callable,
                                       std::unique_ptr<ASTNode> value) {
    std::vector<std::unique_ptr<ASTNode>> values;
    if (auto* tuple = dynamic_cast<TupleExpressionNode*>(value.get())) {
        values = std::move(tuple->elements);
    } else {
        values.push_back(std::move(value));
    }

    auto prependValues = [&](std::vector<std::unique_ptr<ASTNode>>& arguments) {
        for (auto it = values.rbegin(); it != values.rend(); ++it) {
            arguments.insert(arguments.begin(), std::move(*it));
        }
    };
    if (auto* call = dynamic_cast<FunctionCallNode*>(callable.get())) {
        prependValues(call->arguments);
        return callable;
    }
    if (auto* method = dynamic_cast<MethodCallNode*>(callable.get())) {
        prependValues(method->arguments);
        return callable;
    }
    if (auto* identifier = dynamic_cast<IdentifierNode*>(callable.get())) {
        auto call = std::make_unique<FunctionCallNode>();
        call->functionName = identifier->value;
        call->arguments = std::move(values);
        return call;
    }
    return nullptr;
}
}

std::unique_ptr<ASTNode> Parser::parsePipelineExpression() {
    auto expr = parseOrExpression();
    while (match(TokenType::OPERATOR, ">>|") || match(TokenType::OPERATOR, "<<|")) {
        const std::string op = previous().value;
        auto other = parseOrExpression();
        auto piped = op == ">>|"
            ? applyPipeline(std::move(other), std::move(expr))
            : applyPipeline(std::move(expr), std::move(other));
        if (!piped) {
            error("Pipeline operator requires a function, function call, or method call");
            return nullptr;
        }
        expr = std::move(piped);
    }
    return expr;
}

std::unique_ptr<ASTNode> Parser::parseExpression() {
	std::unique_ptr<ASTNode> expr = parsePipelineExpression();
    if (!expr) {
        error("Expected expression");
    }

    if (match(TokenType::OPERATOR, "=") || match(TokenType::OPERATOR, "<-")) {
        std::string op = previous().value;
        return std::make_unique<AssignmentExpressionNode>(std::move(expr), parseExpression(), op);
    }
    if (match(TokenType::OPERATOR, "+=") || match(TokenType::OPERATOR, "-=") ||
        match(TokenType::OPERATOR, "*=") || match(TokenType::OPERATOR, "/=") ||
        match(TokenType::OPERATOR, "%=")) {
        const std::string op = previous().value;
        return std::make_unique<AssignmentExpressionNode>(std::move(expr), parseExpression(), op);
    }

	if (match(TokenType::KEYWORD, "match")) {
		auto matchNode = std::make_unique<MatchExpressionNode>();
		matchNode->expression = std::move(expr);

		expect(TokenType::OPERATOR, "{");

		while (!check(TokenType::OPERATOR, "}")) {
			if (isAtEnd()) {
				error("Unterminated 'match' block");
				return nullptr;
			}

			expect(TokenType::KEYWORD, "case");

			auto casePattern = parsePattern();
			std::vector<std::unique_ptr<ASTNode>> alternatives;
			while (match(TokenType::OPERATOR, "|")) {
				alternatives.push_back(parsePattern());
			}
			std::unique_ptr<ASTNode> guard;
			if (match(TokenType::KEYWORD, "if")) {
				guard = parsePipelineExpression();
			}

			expect(TokenType::OPERATOR, "=>");

			auto caseResult = parseExpression();

			matchNode->cases.emplace_back(std::move(casePattern), std::move(caseResult));
			matchNode->alternatives.push_back(std::move(alternatives));
			matchNode->guards.push_back(std::move(guard));
		}

		expect(TokenType::OPERATOR, "}");

		expr = std::move(matchNode);
	}
	else {
		match(TokenType::OPERATOR, ";");
	}
	return expr;
}

// A `case` pattern: `_`, a literal (`-1` included), a name, or a variant with sub-patterns
// (`Circle(r)`, `Shape.Rect(w, _)`, `Some(Ok(x))`). Parsed as an expression and resolved against
// the matched value's type by the type checker.
std::unique_ptr<ASTNode> Parser::parsePattern() {
	if (check(TokenType::OPERATOR, "-") &&
		(peek(1).type == TokenType::INTEGER_LITERAL || peek(1).type == TokenType::EXPONENTIAL_LITERAL ||
		 peek(1).type == TokenType::FLOAT_LITERAL)) {
		advance();
		const Token& number = advance();
		return std::make_unique<ValueNode>("-" + number.value, number.type);
	}
	return parseSimpleExpression();
}

std::unique_ptr<ASTNode> Parser::parseLambdaExpression() {
	// C++�� ���ٽİ� ������ ������ ����
	// ��: [&](x: Int, y: Int) -> { x + y }
	auto lambdaNode = std::make_unique<LambdaExpressionNode>();

	expect(TokenType::OPERATOR, "[");
	if (match(TokenType::OPERATOR, "&")) {
		lambdaNode->capturesByReference = true;
	}
	else if (match(TokenType::OPERATOR, "=")) {
		lambdaNode->capturesByReference = false;
	}
	else {
		lambdaNode->capturesByReference = false;
		while (!check(TokenType::OPERATOR, "]")) {
			if (match(TokenType::IDENTIFIER)) {
				lambdaNode->captureVariables.push_back(previous().value);
				if (!match(TokenType::OPERATOR, ",")) {
					break;
				}
			}
			else {
				error("Expected identifier in lambda capture list");
			}
		}
	}
	expect(TokenType::OPERATOR, "]");

	expect(TokenType::OPERATOR, "(");
    if (!check(TokenType::OPERATOR, ")")) {
        do {
            expect(TokenType::IDENTIFIER);
            auto paramNode = std::make_unique<ParameterNode>();
            paramNode->name = previous().value;
            if (match(TokenType::OPERATOR, ":")) {
                paramNode->type = parseType();
            }
            else {
                paramNode->type = std::make_unique<UnknownType>();
            }
            lambdaNode->arguments.push_back(std::move(paramNode));
        } while (match(TokenType::OPERATOR, ","));
    }
    expect(TokenType::OPERATOR, ")");

	expect(TokenType::OPERATOR, "->");
	if (match(TokenType::OPERATOR, "{")) {
		lambdaNode->body = parseBlock();
	}
	else {
		error("Expected '{' after '->' in lambda expression");
	}

	return lambdaNode;
}


// Called just after a '{' in expression position. A '{' opens a dictionary literal rather than a
// block when the first top-level item is `key: value` (a ':' before any ';', '=', '->' or the closing
// '}'), or when it is an empty `{}` used as a value (after '=', '(', ',', ':', '[', '<-' or `return`).
bool Parser::looksLikeDictLiteral() const {
	if (check(TokenType::OPERATOR, "}")) {
		if (position < 2) {
			return false;
		}
		const Token& before = tokens[position - 2];
		if (before.type == TokenType::KEYWORD) {
			return before.value == "return";
		}
		if (before.type != TokenType::OPERATOR) {
			return false;
		}
		return before.value == "=" || before.value == "(" || before.value == "," || before.value == ":" ||
			before.value == "[" || before.value == "<-";
	}
	if (peek().type == TokenType::KEYWORD && peek().value != "new") {
		return false;
	}
	int depth = 0;
	for (size_t i = position; i < tokens.size(); ++i) {
		const Token& token = tokens[i];
		if (token.type == TokenType::END_OF_FILE) {
			return false;
		}
		if (token.type != TokenType::OPERATOR) {
			continue;
		}
		const std::string& v = token.value;
		if (v == "(" || v == "[" || v == "{") {
			++depth;
		}
		else if (v == ")" || v == "]") {
			if (depth == 0) return false;
			--depth;
		}
		else if (v == "}") {
			if (depth == 0) return false;
			--depth;
		}
		else if (depth == 0) {
			if (v == ":") return true;
			if (v == ";" || v == "=" || v == "->" || v == "=>" || v == "<-") return false;
		}
	}
	return false;
}

// `{k1: v1, k2: v2, ...}` (a trailing comma is allowed) or `{}`; the '{' is already consumed.
std::unique_ptr<ASTNode> Parser::parseDictLiteral() {
	auto node = std::make_unique<DictLiteralNode>();
	while (!check(TokenType::OPERATOR, "}")) {
		if (isAtEnd()) {
			error("Unterminated dictionary literal");
			return nullptr;
		}
		auto key = parseExpression();
		expect(TokenType::OPERATOR, ":");
		auto value = parseExpression();
		node->keys.push_back(std::move(key));
		node->values.push_back(std::move(value));
		if (!match(TokenType::OPERATOR, ",")) {
			break;
		}
	}
	expect(TokenType::OPERATOR, "}");
	return node;
}

// Called just after a '{' in expression position (and after looksLikeDictLiteral). A '{' opens a set
// literal when its top-level items are separated by commas -- `{1, 2, 3}`, or `{x,}` for a single
// element -- with no ';', '=', '<-' or '->' before the closing '}' (so a block that starts with a
// destructuring assignment `{ q, r = f(); ... }` stays a block).
bool Parser::looksLikeSetLiteral() const {
	if (check(TokenType::OPERATOR, "}")) {
		return false;
	}
	if (peek().type == TokenType::KEYWORD && peek().value != "new") {
		return false;
	}
	int depth = 0;
	bool sawComma = false;
	for (size_t i = position; i < tokens.size(); ++i) {
		const Token& token = tokens[i];
		if (token.type == TokenType::END_OF_FILE) {
			return false;
		}
		if (token.type != TokenType::OPERATOR) {
			continue;
		}
		const std::string& v = token.value;
		if (v == "(" || v == "[" || v == "{") {
			++depth;
		}
		else if (v == ")" || v == "]") {
			if (depth == 0) return false;
			--depth;
		}
		else if (v == "}") {
			if (depth == 0) return sawComma;
			--depth;
		}
		else if (depth == 0) {
			if (v == ",") sawComma = true;
			else if (v == ";" || v == "=" || v == "->" || v == "=>" || v == "<-" || v == ":") return false;
		}
	}
	return false;
}

// `{e1, e2, ...}` (a trailing comma is allowed); the '{' is already consumed.
std::unique_ptr<ASTNode> Parser::parseSetLiteral() {
	auto node = std::make_unique<DictLiteralNode>();
	node->isSet = true;
	while (!check(TokenType::OPERATOR, "}")) {
		if (isAtEnd()) {
			error("Unterminated set literal");
			return nullptr;
		}
		node->keys.push_back(parseExpression());
		if (!match(TokenType::OPERATOR, ",")) {
			break;
		}
	}
	expect(TokenType::OPERATOR, "}");
	return node;
}

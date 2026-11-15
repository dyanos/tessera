#include "parser.h"
#include "LatexMathEqParser.h"
#include "utils.h"

#include <iostream>

#include "all_ast.h"
#include "intrinsics.h"


// parser占쏙옙占쏙옙占쏙옙 parsing占쏙옙 占쏙옙占쏙옙構占? symbol 占쏙옙占?占쏙옙 占싯삼옙占쏙옙 codegen占쌀띰옙 占쏙옙占?
Parser::Parser(std::vector<Token>& tokens)
	: tokens(tokens), position(0) {}

std::unique_ptr<ASTNode> Parser::parse() {
	auto program = std::make_unique<ProgramNode>();
	while (!isAtEnd()) {
		auto stmt = parseTopStatement();
		if (stmt) {
			program->statements.push_back(std::move(stmt));
		}
	}
	return program;
}

std::unique_ptr<Type> Parser::parseType() {
    // Parenthesized type: `(A, B) => C` is a function type; `(T1, …, Tn)` is a tuple (multiple
    // return); a single `(T)` is just a grouping. The `=> ` check disambiguates from function types.
    if (match(TokenType::OPERATOR, "(")) {
        std::vector<std::unique_ptr<Type>> elems;
        if (!check(TokenType::OPERATOR, ")")) {
            elems.push_back(parseType());
            while (match(TokenType::OPERATOR, ",")) {
                elems.push_back(parseType());
            }
        }
        expect(TokenType::OPERATOR, ")");
        if (match(TokenType::OPERATOR, "=>")) {
            auto returnType = parseType();
            return std::make_unique<FunctionType>(elems, returnType);
        }
        if (elems.size() == 1) {
            return std::move(elems[0]);
        }
        return std::make_unique<TupleType>(std::move(elems));
    }

    if (match(TokenType::KEYWORD, "box")) {
        return std::make_unique<BoxType>(parseType());
    }

    if (match(TokenType::OPERATOR, "&")) {
        bool isMutableBorrow = match(TokenType::KEYWORD, "mut");
        return std::make_unique<BorrowType>(parseType(), isMutableBorrow);
    }

    if (match(TokenType::KEYWORD, "unsafe")) {
        expect(TokenType::OPERATOR, "*");
        return std::make_unique<UnsafePointerType>(parseType());
    }

	if (match(TokenType::IDENTIFIER) || match(TokenType::KEYWORD, "mut")) {
		std::string typeName = previous().value;

		// Check if this identifier is a template type parameter
		for (const auto& tp : templateTypeParams) {
			if (typeName == tp) {
				return std::make_unique<TypeVariableType>(typeName);
			}
		}

		std::unique_ptr<Type> baseType;
		if (typeName == "Int" ||
			typeName == "Float" ||
			typeName == "Double" ||
			typeName == "Real" ||
			typeName == "Unit" ||
			typeName == "Void" ||
			typeName == "Char" ||
			typeName == "Boolean" ||
			typeName == "Bool" ||
			typeName == "Long" ||
			typeName == "Natural" ||
			typeName == "Integer" ||
			typeName == "Short" ||
			typeName == "Byte" ||
			typeName == "String") {
			// `Real` is the math-oriented alias for a 64-bit float (getLLVMType maps it to double,
			// and it is the declared result type of sum/mean/norm/approxEq). Without it here, a
			// `val x: Real` was parsed as a reference ClassType and later loaded as a pointer,
			// dereferencing the double's bit pattern as an address and crashing.
			baseType = std::make_unique<BasicType>(typeName);
		}
		else {
			baseType = std::make_unique<ClassType>(typeName);
		}

		if (match(TokenType::OPERATOR, "[")) {
			auto tempArgsType = parseType();
			expect(TokenType::OPERATOR, "]");
			std::vector<std::unique_ptr<Type>> typeArgs;
			typeArgs.push_back(tempArgsType->clone());
			return std::make_unique<GenericType>(baseType, typeArgs);
		}

		// ClassName<Type, ...> syntax for template class instantiation types
		if (match(TokenType::OPERATOR, "<")) {
			std::vector<std::unique_ptr<Type>> typeArgs;
			do {
				typeArgs.push_back(parseTemplateArgumentAsType());
			} while (match(TokenType::OPERATOR, ","));
			expectClosingAngle();
			return std::make_unique<GenericType>(baseType, typeArgs);
		}

		return baseType;
	}
	else if (match(TokenType::OPERATOR, "(")) {
		// ?⑥닔 ????뚯떛
		std::vector<std::unique_ptr<Type>> paramTypes;
		if (!check(TokenType::OPERATOR, ")")) {
			do {
				auto paramType = parseType();
				paramTypes.push_back(std::move(paramType));
			} while (match(TokenType::OPERATOR, ","));
		}
		expect(TokenType::OPERATOR, ")");
		expect(TokenType::OPERATOR, "=>");
		auto returnType = parseType();
		return std::make_unique<FunctionType>(paramTypes, returnType);
	}
	else {
		error("Expected type after ':'");
		return std::make_unique<UnknownType>();
	}
}

std::unique_ptr<Type> Parser::parseTemplateArgumentAsType() {
    if (check(TokenType::IDENTIFIER) || check(TokenType::OPERATOR, "(")) {
        return parseType();
    }

    if (match(TokenType::INTEGER_LITERAL) ||
        match(TokenType::HEX_LITERAL) ||
        match(TokenType::BINARY_LITERAL) ||
        match(TokenType::OCTAL_LITERAL)) {
        return std::make_unique<TypeVariableType>(previous().value);
    }

    if (match(TokenType::FLOAT_LITERAL) || match(TokenType::EXPONENTIAL_LITERAL)) {
        return std::make_unique<TypeVariableType>(previous().value);
    }

    if (match(TokenType::BOOLEAN_LITERAL)) {
        return std::make_unique<TypeVariableType>(previous().value);
    }

    error("Expected template argument");
    return std::make_unique<UnknownType>();
}

std::vector<std::string> Parser::parseTemplateParameters() {
	std::vector<std::string> params;
	currentTemplateParams.clear();
	expect(TokenType::OPERATOR, "<");
	do {
		if (match(TokenType::KEYWORD, "typename") || match(TokenType::KEYWORD, "class")) {
			expect(TokenType::IDENTIFIER);
			std::string name = previous().value;
			params.push_back(name);
			currentTemplateParams.push_back(TemplateParam(name));
		}
		else if (check(TokenType::IDENTIFIER)) {
			// Non-type template parameter: e.g. "Int N"
			std::string typeName = peek().value;
			// Check if it's a known type name (for non-type param)
			if (typeName == "Int" || typeName == "Long" || typeName == "Short" ||
				typeName == "Byte" || typeName == "Boolean" || typeName == "Bool") {
				advance(); // consume type name
				expect(TokenType::IDENTIFIER);
				std::string paramName = previous().value;
				params.push_back(paramName);
				currentTemplateParams.push_back(TemplateParam(paramName, std::make_unique<BasicType>(typeName)));
			}
			else {
				error("Expected 'typename', 'class', or a type name in template parameter list");
				break;
			}
		}
		else {
			error("Expected 'typename', 'class', or a type name in template parameter list");
			break;
		}
	} while (match(TokenType::OPERATOR, ","));
	expect(TokenType::OPERATOR, ">");
	return params;
}

std::unique_ptr<ASTNode> Parser::parseTopStatement() {
    if (match(TokenType::OPERATOR, ";")) {
        return nullptr;
    }

	std::unique_ptr<ASTNode> attributeNode = parseAnnotationStatement();

	// Handle template declarations
	if (match(TokenType::KEYWORD, "template")) {
		auto templateNode = std::make_unique<TemplateDeclarationNode>();
		templateNode->typeParameters = parseTemplateParameters();
		templateNode->templateParams = currentTemplateParams;

		// Set active template type params for parseType()
		templateTypeParams = templateNode->typeParameters;

		// Parse the inner declaration (def, class, or struct)
		if (match(TokenType::KEYWORD, "def")) {
			templateNode->declaration = parseFunctionDeclaration();
		}
		else if (match(TokenType::KEYWORD, "class")) {
			templateNode->declaration = parseClassDeclaration();
		}
		else if (match(TokenType::KEYWORD, "struct")) {
			auto structNode = parseClassDeclaration();
			if (structNode) {
				if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(structNode.get())) {
					classNode->isStruct = true;
				}
			}
			templateNode->declaration = std::move(structNode);
		}
		else {
			error("Expected 'def', 'class', or 'struct' after template parameters");
		}

		// Clear active template type params
		templateTypeParams.clear();
		return templateNode;
	}

	// Handle constexpr modifier
	bool isConstexpr = false;
	if (check(TokenType::KEYWORD, "constexpr")) {
		isConstexpr = true;
		advance();
	}

	bool isExternal = false;
	if (check(TokenType::KEYWORD, "external")) {
		isExternal = true;
		advance();
	}

	bool isAsyncFunction = false;
	// `async` is a declaration modifier only before `def`; otherwise it starts an `async call`.
	if (check(TokenType::KEYWORD, "async") && peek(1).type == TokenType::KEYWORD &&
	    (peek(1).value == "def" || peek(1).value == "unsafe")) {
		isAsyncFunction = true;
		advance();
	}

	bool isUnsafeFunction = false;
	if (check(TokenType::KEYWORD, "unsafe")) {
		isUnsafeFunction = true;
		advance();
		if (!check(TokenType::KEYWORD, "def")) {
			error("'unsafe' modifier is only valid on function declarations");
		}
	}

	if (isAsyncFunction && !check(TokenType::KEYWORD, "def")) {
		error("'async' modifier is only valid on function declarations");
	}

	std::unique_ptr<ASTNode> node = nullptr;
	if (match(TokenType::KEYWORD, "import")) {
		if (isExternal) {
			error("Import statement cannot be marked as external");
		}

		if (attributeNode) {
			error("Import statement cannot have attributes");
		}

		node = parseImportStatement();
	}
	else if (match(TokenType::KEYWORD, "class")) {
		node = parseClassDeclaration(isExternal);

		ClassDeclarationNode* classNode = dynamic_cast<ClassDeclarationNode*>(node.get());
		if (!classNode) {
			error("Expected class declaration node");
			return nullptr;
		}
		classNode->isExternal = isExternal;
	}
	else if (match(TokenType::KEYWORD, "struct")) {
		node = parseClassDeclaration(isExternal);

		ClassDeclarationNode* classNode = dynamic_cast<ClassDeclarationNode*>(node.get());
		if (!classNode) {
			error("Expected struct declaration node");
			return nullptr;
		}
		classNode->isExternal = isExternal;
		classNode->isStruct = true;
	}
	else if (match(TokenType::KEYWORD, "object")) {
		node = parseObjectDeclaration(isExternal);

		ObjectDeclarationNode* objectNode = dynamic_cast<ObjectDeclarationNode*>(node.get());
		if (!objectNode) {
			error("Expected object declaration node");
			return nullptr;
		}
		objectNode->isExternal = isExternal;
	}
	else if (match(TokenType::KEYWORD, "def")) {
		auto targetNode = parseFunctionDeclaration(isExternal);
		targetNode->isExternal = isExternal;
		targetNode->isConstexpr = isConstexpr;
		targetNode->isUnsafe = isUnsafeFunction;
		targetNode->isAsync = isAsyncFunction;
		node = std::move(targetNode);
	}
	else {
		if (isUnsafeFunction) {
			error("Expected 'def' after 'unsafe' at top level");
		}
		// constexpr can precede val/var in top-level statements
		if (isConstexpr) {
			if (match(TokenType::KEYWORD, "val") || match(TokenType::KEYWORD, "var")) {
				bool isMutable = previous().value == "var";
				auto varDecl = parseVariableDeclaration(isMutable);
				varDecl->isConstexpr = isConstexpr;
				match(TokenType::OPERATOR, ";");
				return varDecl;
			}
		}
		return parseStatement();
	}

	if (attributeNode) {
		auto* attrNode = dynamic_cast<AttributeNode*>(attributeNode.get());
		if (!attrNode) {
			error("Expected attribute node");
			return nullptr;
		}
		attrNode->target = std::move(node);
		// `[@Intrinsic] external def name(...)` enables the compiler intrinsic `name` (see intrinsics.h).
		if (auto* function = dynamic_cast<FunctionDeclarationNode*>(attrNode->target.get())) {
			const ASTNode* attrExpr = attrNode->expr.get();
			const auto* attrIdent = dynamic_cast<const IdentifierNode*>(attrExpr);
			const auto* attrCall = dynamic_cast<const FunctionCallNode*>(attrExpr);
			if ((attrIdent && attrIdent->value == "Intrinsic") || (attrCall && attrCall->functionName == "Intrinsic")) {
				function->isIntrinsic = true;
				declaredIntrinsics().insert(function->name);
			}
		}
		return attributeNode;
	}

	return node;
}

std::unique_ptr<ASTNode> Parser::parseAnnotationExpression() {
	if (match(TokenType::IDENTIFIER) || match(TokenType::KEYWORD)) {
		auto identifierNode = std::make_unique<IdentifierNode>(previous().value);
		if (!match(TokenType::OPERATOR, "(")) {
			return identifierNode;
		}
		else {
			auto callNode = std::make_unique<FunctionCallNode>();
			callNode->functionName = identifierNode->value;
			callNode->arguments = parseCallParameterList();
			return callNode;
		}
	}
	else if (match(TokenType::STRING_LITERAL)) {
		auto stringNode = std::make_unique<ValueNode>(previous().value, previous().type);
		return stringNode;
	}
	else if (match(TokenType::INTEGER_LITERAL)) {
		auto intNode = std::make_unique<ValueNode>(previous().value, previous().type);
		return intNode;
	}
	else {
		error("Expected simple expression in attribute");
		return nullptr;
	}
}

std::unique_ptr<ClassBodyNode> Parser::parseClassBody() {
	auto classBody = std::make_unique<ClassBodyNode>();
	while (!check(TokenType::OPERATOR, "}")) {
		if (isAtEnd()) {
			error("Unterminated class body");
			return nullptr;
		}

		if (match(TokenType::KEYWORD, "override")) {
			expect(TokenType::KEYWORD, "def");
			auto method = parseFunctionDeclaration();
			if (method) {
				method->isOverride = true;
			}
			classBody->methods.push_back(std::move(method));
            match(TokenType::OPERATOR, ";");
		}
		else if (match(TokenType::KEYWORD, "def")) {
			classBody->methods.push_back(parseFunctionDeclaration());
            match(TokenType::OPERATOR, ";");
		}
		else if (match(TokenType::KEYWORD, "val") || match(TokenType::KEYWORD, "var")) {
			bool isMutable = previous().value == "var";
			classBody->fields.push_back(parseVariableDeclaration(isMutable));
            match(TokenType::OPERATOR, ";");
		}
		else {
			error("Unexpected token in class body");
			advance();
		}
	}
	return classBody;
}

// One call argument. `name = value` is a named argument, matched to a parameter by name before
// type checking (default_arguments.cpp).
std::unique_ptr<ASTNode> Parser::parseCallArgument() {
	if (check(TokenType::IDENTIFIER) && position + 1 < tokens.size() &&
		tokens[position + 1].type == TokenType::OPERATOR && tokens[position + 1].value == "=") {
		const int argLine = peek().line;
		auto named = std::make_unique<AssignmentExpressionNode>();
		named->left = std::make_unique<IdentifierNode>(advance().value);
		advance();  // '='
		named->right = parseExpression();
		named->isNamedArgument = true;
		named->line = argLine;
		return named;
	}
	return parseExpression();
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseCallParameterList()
{
	std::vector<std::unique_ptr<ASTNode>> arguments;
	while (!check(TokenType::OPERATOR, ")")) {
		if (isAtEnd()) {
			error("Unterminated argument list");
			return std::vector<std::unique_ptr<ASTNode>>();
		}

		arguments.push_back(parseCallArgument());
		if (!match(TokenType::OPERATOR, ",")) {
			break;
		}
	}

	expect(TokenType::OPERATOR, ")");

	return arguments;
}

std::vector<std::unique_ptr<ParameterNode>> Parser::parseParameterList() {
	std::vector<std::unique_ptr<ParameterNode>> parameters;

	if (!check(TokenType::OPERATOR, ")")) {
		do {
			expect(TokenType::IDENTIFIER);
			std::string paramName = previous().value;

			const int paramLine = previous().line;
			expect(TokenType::OPERATOR, ":");
			std::unique_ptr<Type> paramType = parseType();

			auto parameter = std::make_unique<ParameterNode>(paramName, paramType->clone());
			parameter->line = paramLine;
			if (match(TokenType::OPERATOR, "=")) {
				parameter->defaultValue = parseExpression();
			}
			parameters.push_back(std::move(parameter));
		} while (match(TokenType::OPERATOR, ","));
	}

	return parameters;
}

std::unique_ptr<ParameterNode> Parser::parseParameter() {
	if (match(TokenType::IDENTIFIER)) {
		std::string paramName = previous().value;
		expect(TokenType::OPERATOR, ":");
		std::unique_ptr<Type> paramType = parseType();
		auto param = std::make_unique<ParameterNode>();
		param->name = paramName;
		param->type = paramType ? paramType->clone() : std::make_unique<UnknownType>();
		return param;
	}
	else {
		error("Expected parameter name");
		return nullptr;
	}
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseArgumentList() {
	std::vector<std::unique_ptr<ASTNode>> arguments;
	if (!check(TokenType::OPERATOR, ")")) {
		do {
			arguments.push_back(parseCallArgument());
		} while (match(TokenType::OPERATOR, ","));
	}
	expect(TokenType::OPERATOR, ")");
	return arguments;
}

std::unique_ptr<ASTNode> Parser::parseLatexCommand() {
	auto latexParser = LatexMathEqParser(&tokens, &position);
	return latexParser.parse();
}

std::unique_ptr<ASTNode> Parser::parseInlineMathLatex() {
	auto latexParser = LatexMathEqParser(&tokens, &position);
	return latexParser.parse();
}

std::unique_ptr<ASTNode> Parser::parseBlockMathLatex() {
	auto latexParser = LatexMathEqParser(&tokens, &position);
	return latexParser.parse();
}

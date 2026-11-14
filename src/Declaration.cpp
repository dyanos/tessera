#include "parser.h"
#include "all_ast.h"

std::unique_ptr<ASTNode> Parser::parseAnnotationStatement() {
	if (match(TokenType::OPERATOR, "[@")) {
		if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) {
			auto simpleExpr = parseAnnotationExpression();
			match(TokenType::OPERATOR, "]");
			auto attributeNode = std::make_unique<AttributeNode>();
			attributeNode->expr = std::move(simpleExpr);
			return attributeNode;
		}
		else {
			error("Expected identifier after '@' for attribute");
			return nullptr;
		}
	}

	return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseImportStatement() {
	std::vector<std::string> pathComponents;

	do {
		if (matchIdentifierName()) {
			pathComponents.push_back(previous().value);
		}
		else {
			error("Expected identifier in import statement");
			return nullptr;
		}
	} while (match(TokenType::OPERATOR, "."));

	// `import a.b = name;` -- the alias is applied during import expansion (src/import_alias.h);
	// an import that reaches the parser was not resolved, so only its syntax is checked here.
	if (match(TokenType::OPERATOR, "=")) {
		if (!matchIdentifierName()) {
			error("Expected an alias name after '=' in import statement");
			return nullptr;
		}
	}

	auto importNode = std::make_unique<ImportNode>();
	importNode->path = pathComponents;
	return importNode;
}

std::unique_ptr<ASTNode> Parser::parseClassDeclaration(bool isExternal) {
	if (matchIdentifierName()) {
		std::string className = previous().value;

		std::vector<std::unique_ptr<ParameterNode>> constructorParams;
		if (match(TokenType::OPERATOR, "(")) {
			constructorParams = parseParameterList();
			expect(TokenType::OPERATOR, ")");
		}

		std::string superClassName;
		if (match(TokenType::KEYWORD, "extends")) {
			if (match(TokenType::IDENTIFIER)) {
				superClassName = previous().value;
			}
			else {
				error("Expected superclass name after 'extends'");
			}
		}

		std::unique_ptr<ClassBodyNode> classBody = nullptr;
		if (isExternal == false) {
			if (match(TokenType::OPERATOR, "{")) {
				classBody = parseClassBody();
				expect(TokenType::OPERATOR, "}");
			}
			else {
				error("Expected '{' after class declaration");
			}
		}

		return std::make_unique<ClassDeclarationNode>(className, constructorParams, superClassName, std::move(classBody));
	}
	else {
		error("Expected class name after 'class'");
		return nullptr;
	}
}

std::unique_ptr<VariableDeclarationNode> Parser::parseVariableDeclaration(bool isMutable) {
	if (matchIdentifierName()) {
		std::string varName = previous().value;
		std::unique_ptr<Type> varType;
		std::unique_ptr<ASTNode> initializer;
		bool hasExplicitType = false;

		if (match(TokenType::OPERATOR, ":")) {
			hasExplicitType = true;
			varType = parseType();
		}
		else {
			varType = nullptr;
		}

		if (match(TokenType::OPERATOR, "=")) {
			initializer = parseExpression();
		}
		else {
			error("Expected '=' in variable declaration");
		}

		// TODO: initializer?? ???? ??? ?????? ?????? ??
		if (varType == nullptr) {
			if (initializer) {
				varType = initializer->getType();

				if (!varType) {
					//error("Cannot infer type of '" + varName + "'");
					varType = std::make_unique<UnknownType>();
				}
			}
		}

		auto varDecl = std::make_unique<VariableDeclarationNode>();
		varDecl->name = varName;
		varDecl->type = std::move(varType);
		varDecl->initializer = std::move(initializer);
		varDecl->isMutable = isMutable;
		varDecl->hasExplicitType = hasExplicitType;

		return varDecl;
	}
	else {
		error("Expected identifier after 'val' or 'var'");
		return nullptr;
	}
}

std::unique_ptr<FunctionDeclarationNode> Parser::parseFunctionDeclaration(bool isExternal) {
    const int declarationLine = previous().line;
    std::string functionName;
    if (match(TokenType::KEYWORD, "operator")) {
        expect(TokenType::OPERATOR);
        functionName = "operator" + previous().value;
    }
    else {
	    // A keyword may name a method (`def filter(...)`), which is then called as `x.filter(...)`.
	    if (!matchMemberName()) {
		    expect(TokenType::IDENTIFIER);
	    }
	    functionName = previous().value;
    }

	expect(TokenType::OPERATOR, "(");
	auto parameters = parseParameterList();
	expect(TokenType::OPERATOR, ")");

	std::unique_ptr<Type> returnType;
	if (match(TokenType::OPERATOR, ":")) {
		returnType = parseType();
	}
	else {
		returnType = std::make_unique<BasicType>("Unit");
	}

	// `requires` / `ensures` clauses sit between the signature and the body.
	auto contractHolder = std::make_unique<BlockNode>();
	parseContractClauses(*contractHolder, true);

	std::unique_ptr<BlockNode> body = nullptr;
	if (isExternal == false) {
		if (match(TokenType::OPERATOR, "{")) {
			body = parseBlock();
		}
		else if (match(TokenType::OPERATOR, "=")) {
			auto expr = parseExpression();
			body = std::make_unique<BlockNode>();
			if (returnType && !returnType->isVoidTy() && returnType->getName() != "Unit") {
				auto returnNode = std::make_unique<ReturnStatementNode>();
				returnNode->expression = std::move(expr);
				body->statements.push_back(std::move(returnNode));
			}
			else {
				body->statements.push_back(std::move(expr));
			}
		}
		else {
			error("Expected '{' or '=' after function declaration");
		}
	}

	auto function = std::make_unique<FunctionDeclarationNode>(functionName, parameters, std::move(returnType), std::move(body));
	function->line = declarationLine;
	function->contracts = contractHolder->contracts;
	return function;
}

std::unique_ptr<ASTNode> Parser::parseObjectDeclaration(bool isExternal) {
	if (matchIdentifierName()) {
		std::string objectName = previous().value;
		std::unique_ptr<ASTNode> body = nullptr;
		if (isExternal == false) {
			if (match(TokenType::OPERATOR, "{")) {
				body = parseBlock();
			}
			else if (match(TokenType::OPERATOR, "=")) {
				auto blockNode = std::make_unique<BlockNode>();
				if (match(TokenType::OPERATOR, "{")) {
					blockNode = parseBlock();
				}
				else {
					auto expr = parseExpression();
					blockNode->statements.push_back(std::move(expr));
				}
				body = std::move(blockNode);
			}
			else {
				error("Expected '{' after object declaration");
			}
		}
		auto objDecl = std::make_unique<ObjectDeclarationNode>();
		objDecl->name = objectName;
		objDecl->body = std::move(body);
		return objDecl;
	}
	else {
		error("Expected object name after 'object'");
		return nullptr;
	}
}

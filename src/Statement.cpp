#include "parser.h"
#include "all_ast.h"

#include <string>

namespace {
std::unique_ptr<ValueNode> intLiteral(int value) {
	return std::make_unique<ValueNode>(std::to_string(value), TokenType::INTEGER_LITERAL);
}

std::unique_ptr<ValueNode> doubleLiteral(double value) {
	return std::make_unique<ValueNode>(std::to_string(value), TokenType::FLOAT_LITERAL);
}
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
	const int startLine = isAtEnd() ? 0 : peek().line;
	auto statement = parseStatementWithoutLine();
	if (statement && statement->line == 0) {
		statement->line = startLine;
	}
	return statement;
}

void Parser::parseContractClauses(ASTNode& node, bool functionClauses) {
	while (true) {
		std::string word;
		if (functionClauses && (checkWord("requires") || checkWord("ensures"))) word = peek().value;
		else if (!functionClauses && (checkWord("invariant") || checkWord("decreases"))) word = peek().value;
		else break;
		const int line = peek().line;
		advance();
		const size_t first = position;
		ContractClause clause;
		// Not parseExpression(): in `def f(): Int requires x > 0 = body` the '=' starts the body.
		clause.expression = parsePipelineExpression();
		clause.line = line;
		for (size_t i = first; i < position && i < tokens.size(); ++i) {
			const Token& t = tokens[i];
			if (t.type == TokenType::COMMENT) continue;
			if (!clause.text.empty()) clause.text += " ";
			clause.text += t.type == TokenType::STRING_LITERAL ? "\"" + t.value + "\"" : t.value;
		}
		if (clause.expression) clause.expression->line = line;
		if (!node.contracts) node.contracts = std::make_shared<ContractClauses>();
		if (word == "requires") node.contracts->requiresClauses.push_back(std::move(clause));
		else if (word == "ensures") node.contracts->ensuresClauses.push_back(std::move(clause));
		else if (word == "invariant") node.contracts->invariants.push_back(std::move(clause));
		else node.contracts->decreases.push_back(std::move(clause));
	}
}

std::unique_ptr<ASTNode> Parser::parseStatementWithoutLine() {
	bool isConstexpr = false;
	if (check(TokenType::KEYWORD, "constexpr")) {
		isConstexpr = true;
		advance();
	}

	if (match(TokenType::KEYWORD, "unsafe")) {
		if (match(TokenType::KEYWORD, "def")) {
			auto functionNode = parseFunctionDeclaration();
			functionNode->isUnsafe = true;
			return functionNode;
		}
		expect(TokenType::OPERATOR, "{");
		auto block = parseBlock();
		if (block) {
			block->isUnsafeContext = true;
		}
		return block;
	}
	else if (match(TokenType::KEYWORD, "unatomic")) {
		expect(TokenType::OPERATOR, "{");
		auto block = parseBlock();
		if (block) {
			block->isUnatomic = true;
		}
		return block;
	}
	else if (match(TokenType::KEYWORD, "val") || match(TokenType::KEYWORD, "var")) {
		bool isMutable = previous().value == "var";
		auto expr = parseVariableDeclaration(isMutable);
		expr->isConstexpr = isConstexpr;
		match(TokenType::OPERATOR, ";");
		return expr;
	}
	else if (match(TokenType::KEYWORD, "if")) {
		auto ifStmt = parseIfStatement();
		auto* ifNode = dynamic_cast<IfStatementNode*>(ifStmt.get());
		if (ifNode && isConstexpr) ifNode->isConstexpr = true;
		return ifStmt;
	}
	else if (match(TokenType::KEYWORD, "for")) {
		return parseForStatement();
	}
	else if (match(TokenType::KEYWORD, "def")) {
		return parseFunctionDeclaration();
	}
	else if (match(TokenType::KEYWORD, "return")) {
		std::unique_ptr<ASTNode> expr = nullptr;
		if (!check(TokenType::END_OF_FILE) && !check(TokenType::OPERATOR, ";")) {
			expr = parseExpression();
				if (check(TokenType::OPERATOR, ",")) {
					auto tuple = std::make_unique<TupleExpressionNode>();
					tuple->elements.push_back(std::move(expr));
					while (match(TokenType::OPERATOR, ",")) {
						tuple->elements.push_back(parseExpression());
					}
					expr = std::move(tuple);
				}
		}
		match(TokenType::OPERATOR, ";");
		auto returnNode = std::make_unique<ReturnStatementNode>();
		returnNode->expression = std::move(expr);
		return returnNode;
	}
	else if (match(TokenType::KEYWORD, "while")) {
		return parseWhileStatement();
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
	else if (match(TokenType::KEYWORD, "object")) {
		return parseObjectDeclaration();
	}
	else {
		if (check(TokenType::IDENTIFIER)) {
			saveTokenPosition();
			std::vector<DestructuringAssignmentNode::Target> targets;
			bool destructuring = false;
			if (match(TokenType::IDENTIFIER)) {
				targets.push_back({ previous().value, false });
				if (check(TokenType::OPERATOR, ",")) {
					destructuring = true;
					while (match(TokenType::OPERATOR, ",")) {
						if (match(TokenType::IDENTIFIER)) {
							targets.push_back({ previous().value, false });
						} else { destructuring = false; break; }
					}
					if (destructuring && match(TokenType::OPERATOR, "=")) {
						auto rhs = parseExpression();
						match(TokenType::OPERATOR, ";");
						auto node = std::make_unique<DestructuringAssignmentNode>();
						node->targets = std::move(targets);
						node->value = std::move(rhs);
						discardTokenPosition();
						return node;
					}
				}
			}
			restoreTokenPosition();
		}
		auto expr = parseExpression();
		match(TokenType::OPERATOR, ";");
		return expr;
	}
}

std::unique_ptr<ASTNode> Parser::parseMoleculeSimulationExpression() {
	if (match(TokenType::IDENTIFIER)) {
		// Optional model name.
	}
	expect(TokenType::OPERATOR, "{");

	int atomCount = 0;
	int bondCount = 0;
	std::unique_ptr<ASTNode> steps = intLiteral(100);
	std::unique_ptr<ASTNode> dt = doubleLiteral(0.001);
	std::unique_ptr<ASTNode> temperature = doubleLiteral(300.0);

	while (!check(TokenType::OPERATOR, "}")) {
		if (isAtEnd()) {
			error("Unterminated molecule simulation block");
			return nullptr;
		}

		if (matchWord("atom")) {
			if (!checkWord("at") && (match(TokenType::IDENTIFIER) || match(TokenType::KEYWORD))) {
				// Optional atom label or element symbol.
			}
			if (matchWord("at")) {
				expect(TokenType::OPERATOR, "(");
				parseExpression();
				expect(TokenType::OPERATOR, ",");
				parseExpression();
				expect(TokenType::OPERATOR, ",");
				parseExpression();
				expect(TokenType::OPERATOR, ")");
			}
			++atomCount;
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("lattice")) {
			if (!match(TokenType::INTEGER_LITERAL)) {
				error("Expected lattice width");
			}
			int width = std::stoi(previous().value);
			expect(TokenType::IDENTIFIER, "x");
			if (!match(TokenType::INTEGER_LITERAL)) {
				error("Expected lattice height");
			}
			int height = std::stoi(previous().value);
			if (matchWord("spacing")) {
				parsePrimaryExpression();
			}
			atomCount += width * height;
			bondCount += (width > 1 ? (width - 1) * height : 0) + (height > 1 ? width * (height - 1) : 0);
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("bond")) {
			parseExpression();
			if (match(TokenType::OPERATOR, ",")) {
				parseExpression();
			}
			else {
				parseExpression();
			}
			++bondCount;
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("steps")) {
			steps = parseExpression();
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("dt")) {
			dt = parseExpression();
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("temperature")) {
			temperature = parseExpression();
			match(TokenType::OPERATOR, ";");
		}
		else {
			error("Expected atom, bond, steps, dt, or temperature in molecule block");
			advance();
		}
	}
	expect(TokenType::OPERATOR, "}");

	auto callNode = std::make_unique<FunctionCallNode>();
	callNode->functionName = "mdSimulate";
	callNode->arguments.push_back(intLiteral(atomCount));
	callNode->arguments.push_back(intLiteral(bondCount));
	callNode->arguments.push_back(std::move(steps));
	callNode->arguments.push_back(std::move(dt));
	callNode->arguments.push_back(std::move(temperature));
	return callNode;
}

std::unique_ptr<ASTNode> Parser::parseCfdSimulationExpression() {
	if (match(TokenType::IDENTIFIER)) {
		// Optional case name.
	}
	expect(TokenType::OPERATOR, "{");

	std::unique_ptr<ASTNode> width = intLiteral(32);
	std::unique_ptr<ASTNode> height = intLiteral(32);
	std::unique_ptr<ASTNode> steps = intLiteral(100);
	std::unique_ptr<ASTNode> dt = doubleLiteral(0.01);
	std::unique_ptr<ASTNode> viscosity = doubleLiteral(0.001);
	std::unique_ptr<ASTNode> velocity = doubleLiteral(1.0);

	while (!check(TokenType::OPERATOR, "}")) {
		if (isAtEnd()) {
			error("Unterminated cfd simulation block");
			return nullptr;
		}

		if (matchWord("grid")) {
			width = parseExpression();
			if (match(TokenType::IDENTIFIER, "x") || match(TokenType::OPERATOR, ",")) {
				height = parseExpression();
			}
			else {
				error("Expected 'x' or ',' in cfd grid declaration");
			}
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("steps")) {
			steps = parseExpression();
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("dt")) {
			dt = parseExpression();
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("viscosity")) {
			viscosity = parseExpression();
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("velocity")) {
			velocity = parseExpression();
			match(TokenType::OPERATOR, ";");
		}
		else {
			error("Expected grid, steps, dt, viscosity, or velocity in cfd block");
			advance();
		}
	}
	expect(TokenType::OPERATOR, "}");

	auto callNode = std::make_unique<FunctionCallNode>();
	callNode->functionName = "cfdSimulate";
	callNode->arguments.push_back(std::move(width));
	callNode->arguments.push_back(std::move(height));
	callNode->arguments.push_back(std::move(steps));
	callNode->arguments.push_back(std::move(dt));
	callNode->arguments.push_back(std::move(viscosity));
	callNode->arguments.push_back(std::move(velocity));
	return callNode;
}

std::unique_ptr<ASTNode> Parser::parseProteinMcmcExpression() {
	if (match(TokenType::IDENTIFIER)) {
		// Optional model name.
	}
	expect(TokenType::OPERATOR, "{");

	int residueCount = 0;
	std::unique_ptr<ASTNode> steps = intLiteral(1000);
	std::unique_ptr<ASTNode> temperature = doubleLiteral(300.0);

	while (!check(TokenType::OPERATOR, "}")) {
		if (isAtEnd()) {
			error("Unterminated protein MCMC block");
			return nullptr;
		}

		if (matchWord("chain")) {
			if (match(TokenType::STRING_LITERAL)) {
				residueCount += static_cast<int>(previous().value.size());
			}
			else {
				error("Expected protein chain string literal");
			}
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("mcmc")) {
			expectWord("steps");
			steps = parsePrimaryExpression();
			match(TokenType::OPERATOR, ";");
		}
		else if (matchWord("temperature")) {
			temperature = parsePrimaryExpression();
			match(TokenType::OPERATOR, ";");
		}
		else {
			error("Expected chain, mcmc, or temperature in protein block");
			advance();
		}
	}
	expect(TokenType::OPERATOR, "}");

	auto callNode = std::make_unique<FunctionCallNode>();
	callNode->functionName = "proteinMcmc";
	callNode->arguments.push_back(intLiteral(residueCount));
	callNode->arguments.push_back(std::move(steps));
	callNode->arguments.push_back(std::move(temperature));
	return callNode;
}

std::unique_ptr<ASTNode> Parser::parseOdeSimulationExpression() {
	if (!matchWord("euler")) {
		error("Expected ODE solver name 'euler'");
	}

	std::unique_ptr<ASTNode> differentialFunction;
	if (match(TokenType::IDENTIFIER) || match(TokenType::KEYWORD)) {
		differentialFunction = std::make_unique<IdentifierNode>(previous().value);
	}
	else {
		error("Expected differential function name after 'ode euler'");
		differentialFunction = std::make_unique<IdentifierNode>("unknown");
	}

	expectWord("from");
	expect(TokenType::OPERATOR, "(");
	auto t0 = parsePrimaryExpression();
	expect(TokenType::OPERATOR, ",");
	auto y0 = parsePrimaryExpression();
	expect(TokenType::OPERATOR, ")");

	expectWord("step");
	auto h = parsePrimaryExpression();

	expectWord("steps");
	auto n = parsePrimaryExpression();

	auto callNode = std::make_unique<FunctionCallNode>();
	callNode->functionName = "odeEuler";
	callNode->arguments.push_back(std::move(differentialFunction));
	callNode->arguments.push_back(std::move(t0));
	callNode->arguments.push_back(std::move(y0));
	callNode->arguments.push_back(std::move(h));
	callNode->arguments.push_back(std::move(n));
	return callNode;
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
	auto block = std::make_unique<BlockNode>();
	const Token openingBrace = previous();
	while (!match(TokenType::OPERATOR, "}")) {
		if (isAtEnd()) {
			error("Unterminated block opened at line " + std::to_string(openingBrace.line) +
				", column " + std::to_string(openingBrace.column));
			return nullptr;
		}
		auto stmt = parseStatement();
		if (stmt) {
			block->statements.push_back(std::move(stmt));
		}
	}
	return block;
}

std::unique_ptr<ASTNode> Parser::parseIfStatement() {
	auto ifNode = std::make_unique<IfStatementNode>();

	// Handle "if constexpr (...)" syntax
	if (match(TokenType::KEYWORD, "constexpr")) {
		ifNode->isConstexpr = true;
	}

	expect(TokenType::OPERATOR, "(");
	ifNode->condition = parseExpression();
	expect(TokenType::OPERATOR, ")");

	if (match(TokenType::OPERATOR, "{")) {
		ifNode->thenBlock = parseBlock();
	}
	else {
		error("Expected '{' after 'if' condition");
	}

	if (match(TokenType::KEYWORD, "else")) {
		if (match(TokenType::OPERATOR, "{")) {
			ifNode->elseBlock = parseBlock();
		}
		else if (match(TokenType::KEYWORD, "if")) {
			auto elseIfBlock = std::make_unique<BlockNode>();
			elseIfBlock->statements.push_back(parseIfStatement());
			ifNode->elseBlock = std::move(elseIfBlock);
		}
		else {
			error("Expected '{' or 'if' after 'else'");
		}
	}

	return ifNode;
}

std::unique_ptr<ASTNode> Parser::parseForStatement() {
	auto forNode = std::make_unique<ForStatementNode>();

	expect(TokenType::OPERATOR, "(");

	if (match(TokenType::IDENTIFIER)) {
		forNode->variable = previous().value;
		if (match(TokenType::OPERATOR, ",")) {
			forNode->tokenValue = forNode->variable;
			expect(TokenType::IDENTIFIER);
			forNode->variable = previous().value;
		}

		if (match(TokenType::OPERATOR, "<-")) {
			// Token-stream iteration sugar.  It preserves the ordinary range-loop
			// representation so existing loop lowering remains the single backend.
			if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::OPERATOR && peek(1).value == "." &&
				peek(2).type == TokenType::IDENTIFIER && peek(2).value == "between") {
				advance();
				forNode->tokenStream = std::make_unique<IdentifierNode>(previous().value);
				expect(TokenType::OPERATOR, ".");
				expect(TokenType::IDENTIFIER, "between");
				expect(TokenType::OPERATOR, "(");
				auto startExpr = parseExpression();
				expect(TokenType::OPERATOR, ",");
				auto endExpr = parseExpression();
				expect(TokenType::OPERATOR, ")");
				if (match(TokenType::OPERATOR, ".")) {
					expect(TokenType::IDENTIFIER, "withoutTrivia");
					expect(TokenType::OPERATOR, "(");
					expect(TokenType::OPERATOR, ")");
					forNode->skipTrivia = true;
				}
				if (forNode->tokenValue.empty()) {
					error("Token iteration requires both text and ordinal names");
				}
				forNode->isRange = true;
				forNode->isInclusive = false;
				auto rangeExpr = std::make_unique<RangeExpressionNode>();
				rangeExpr->startExpr = std::move(startExpr);
				rangeExpr->endExpr = std::move(endExpr);
				rangeExpr->isInclusive = false;
				forNode->iterableExpr = std::move(rangeExpr);
			}
			else {
			auto startExpr = parseExpression();

			if (match(TokenType::KEYWORD, "to") || match(TokenType::KEYWORD, "until")) {
				forNode->isRange = true;
				forNode->isInclusive = previous().value == "to";

				auto endExpr = parseExpression();

				auto rangeExpr = std::make_unique<RangeExpressionNode>();
				rangeExpr->startExpr = std::move(startExpr);
				rangeExpr->endExpr = std::move(endExpr);
				rangeExpr->isInclusive = forNode->isInclusive;

				forNode->iterableExpr = std::move(rangeExpr);
			}
			else {
				forNode->iterableExpr = std::move(startExpr);
			}
			}
		}
		else {
			error("Expected '<-' in for comprehension");
		}
	}
	else {
		error("Expected identifier in for comprehension");
	}

	expect(TokenType::OPERATOR, ")");
	parseContractClauses(*forNode, false);

	if (match(TokenType::OPERATOR, "{")) {
		forNode->body = parseBlock();
	}
	else {
		error("Expected '{' after 'for' comprehension");
	}

	return forNode;
}

std::unique_ptr<ASTNode> Parser::parseWhileStatement() {
	auto whileNode = std::make_unique<WhileStatementNode>();

	expect(TokenType::OPERATOR, "(");
	whileNode->condition = parseExpression();
	expect(TokenType::OPERATOR, ")");
	parseContractClauses(*whileNode, false);

	if (match(TokenType::OPERATOR, "{")) {
		whileNode->body = parseBlock();
	}
	else {
		error("Expected '{' after 'while' condition");
	}

	return whileNode;
}

std::unique_ptr<ASTNode> Parser::parseMapStatement() {
	auto mapNode = std::make_unique<MapStatementNode>();

	expect(TokenType::OPERATOR, "(");

	if (match(TokenType::IDENTIFIER)) {
		mapNode->variable = previous().value;

		if (match(TokenType::OPERATOR, "<-")) {
			mapNode->iterableExpr = parseExpression();
		}
		else {
			error("Expected '<-' in map statement");
		}
	}
	else {
		error("Expected identifier in map statement");
	}

	expect(TokenType::OPERATOR, ")");

	if (match(TokenType::OPERATOR, "{")) {
		mapNode->body = parseBlock();
	}
	else {
		error("Expected '{' after 'map' statement");
	}

	return mapNode;
}

std::unique_ptr<ASTNode> Parser::parsePMapStatement() {
	auto pmapNode = std::make_unique<PMapStatementNode>();

	expect(TokenType::OPERATOR, "(");

    if ((check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) &&
        peek(1).type == TokenType::OPERATOR && peek(1).value == ",") {
        pmapNode->backend = advance().value;
        expect(TokenType::OPERATOR, ",");
    }

	if (match(TokenType::IDENTIFIER)) {
		pmapNode->variable = previous().value;

		if (match(TokenType::OPERATOR, "<-")) {
			pmapNode->iterableExpr = parseExpression();
		}
		else {
			error("Expected '<-' in pmap statement");
		}
	}
	else {
		error("Expected identifier in pmap statement");
	}

	expect(TokenType::OPERATOR, ")");

	if (match(TokenType::OPERATOR, "{")) {
		pmapNode->body = parseBlock();
	}
	else {
		error("Expected '{' after 'pmap' statement");
	}

	return pmapNode;
}

std::unique_ptr<ASTNode> Parser::parseReduceStatement() {
	auto reduceNode = std::make_unique<ReduceStatementNode>();

	expect(TokenType::OPERATOR, "(");

	if (match(TokenType::IDENTIFIER)) {
		reduceNode->variable = previous().value;

		if (match(TokenType::OPERATOR, "<-")) {
			reduceNode->iterableExpr = parseExpression();
		}
		else {
			error("Expected '<-' in reduce statement");
		}
	}
	else {
		error("Expected identifier in reduce statement");
	}

	// Parse required initial value: reduce(x <- arr, initVal) { body }
	if (!match(TokenType::OPERATOR, ",")) {
		error("Expected ',' and initial value in reduce statement");
	}
	reduceNode->initialValue = parseExpression();

	expect(TokenType::OPERATOR, ")");

	if (match(TokenType::OPERATOR, "{")) {
		reduceNode->body = parseBlock();
	}
	else {
		error("Expected '{' after 'reduce' statement");
	}

	return reduceNode;
}

std::unique_ptr<ASTNode> Parser::parsePReduceStatement() {
	auto reduceNode = std::make_unique<ReduceStatementNode>();
	reduceNode->isParallel = true;

	expect(TokenType::OPERATOR, "(");

	if (match(TokenType::IDENTIFIER) || match(TokenType::KEYWORD)) {
		reduceNode->backend = previous().value;
	}
	else {
		error("Expected backend policy in preduce statement");
	}
	expect(TokenType::OPERATOR, ",");

	if (match(TokenType::IDENTIFIER)) {
		reduceNode->accumulatorVariable = previous().value;
	}
	else {
		error("Expected accumulator identifier in preduce statement");
	}
	expect(TokenType::OPERATOR, ",");

	if (match(TokenType::IDENTIFIER)) {
		reduceNode->variable = previous().value;

		if (match(TokenType::OPERATOR, "<-")) {
			reduceNode->iterableExpr = parseExpression();
		}
		else {
			error("Expected '<-' in preduce statement");
		}
	}
	else {
		error("Expected element identifier in preduce statement");
	}

	if (!match(TokenType::OPERATOR, ",")) {
		error("Expected ',' and initial value in preduce statement");
	}
	reduceNode->initialValue = parseExpression();

	expect(TokenType::OPERATOR, ")");

	if (match(TokenType::OPERATOR, "{")) {
		reduceNode->body = parseBlock();
	}
	else {
		error("Expected '{' after 'preduce' statement");
	}

	return reduceNode;
}

std::unique_ptr<ASTNode> Parser::parseFilterStatement() {
	auto filterNode = std::make_unique<FilterStatementNode>();

	expect(TokenType::OPERATOR, "(");

	if (match(TokenType::IDENTIFIER)) {
		filterNode->variable = previous().value;

		if (match(TokenType::OPERATOR, "<-")) {
			filterNode->iterableExpr = parseExpression();
		}
		else {
			error("Expected '<-' in filter statement");
		}
	}
	else {
		error("Expected identifier in filter statement");
	}

	expect(TokenType::OPERATOR, ")");

	if (match(TokenType::OPERATOR, "{")) {
		filterNode->body = parseBlock();
	}
	else {
		error("Expected '{' after 'filter' statement");
	}

	return filterNode;
}

#include "LatexMathEqParser.h"

#include "BinaryExpressionNode.h"
#include "FunctionCallNode.h"
#include "IdentifierNode.h"
#include "ForStatementNode.h"
#include "ValueNode.h"
#include "UnitNode.h"
#include "UnaryExpressionNode.h"
#include "RangeExpressionNode.h"
#include "ArrayLiteralNode.h"

#include <iostream>

std::unique_ptr<ASTNode> LatexMathEqParser::parse() {
	if (isAtEnd()) {
		throw std::runtime_error("Unexpected end of input while parsing LaTeX expression.");
	}

	return parseExpr();
}

bool LatexMathEqParser::isAtEnd() const {
	return *position >= tokens->size() || (*tokens)[*position].type == TokenType::END_OF_FILE;
}

const Token& LatexMathEqParser::peek(int pos) const {
	if (pos < 0)
		return (*tokens)[*position];
	else if (*position + static_cast<size_t>(pos) < tokens->size())
		return (*tokens)[*position + pos];
	else {
		return (*tokens)[tokens->size() - 1];
	}
}

const Token& LatexMathEqParser::advance() {
	if (!isAtEnd()) (*position)++;
	return previous();
}

const Token& LatexMathEqParser::previous() const {
	if (*position == 0) return (*tokens)[0];
	return (*tokens)[*position - 1];
}

bool LatexMathEqParser::check(TokenType type, const std::string& value) const {
	if (isAtEnd()) return false;
	if ((*tokens)[*position].type != type) return false;
	if (!value.empty() && (*tokens)[*position].value != value) return false;
	return true;
}

bool LatexMathEqParser::match(TokenType type, const std::string& value) {
	while (!isAtEnd() && (*tokens)[*position].type == TokenType::COMMENT) {
		advance();
	}

	if (check(type, value)) {
		advance();
		return true;
	}

	return false;
}

// ===== Operator Symbol Tables =====
const char* operatorSymbolTable[] = {
	"+", "-", "*", "/", "^",
	"=", "<", ">", "<=", ">=", "!=",
	"times", "div", "pm", "mp",
	"cdot", "leq", "geq", "neq"
};

// ===== Constant Symbol Tables =====
const char* constantSymbolTable[] = {
	"pi", "e", "i", "phi", "gamma", "inf"
};

// ===== Greek Letter Symbol Tables =====
const char* greekLetterSymbolTable[] = {
	"alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
	"iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi",
	"rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega",
	"Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta",
	"Iota", "Kappa", "Lambda", "Mu", "Nu", "Xi", "Omicron", "Pi",
	"Rho", "Sigma", "Tau", "Upsilon", "Phi", "Chi", "Psi", "Omega"
};

// ===== Aggregate Function Symbol Tables =====
const char* aggregateFunctionSymbolTable[] = {
	"sum", "int", "biguplus", "bigoplus", "bigvee", "prod", "oint", "bigcap", "bigotimes", "bigwedge", "coprod", "iint", "bigcup", "bigodot", "bigsqcup"
};

// ===== Function No-Arg Symbol Tables =====
const char* functionNoArgSymbolTable[] = {
	"inf", "NaN", "emptyset", "infty"
};

// ===== Function One-Arg Symbol Tables =====
const char* functionOneArgSymbolTable[] = {
	"arccos", "arcsin", "arctan", "arg",
	"cos", "cosh", "cot", "coth",
	"csc", "deg", "det", "dim",
	"exp", "gcd", "hom",
	"ker", "lg", "lim", "liminf",
	"limsup", "ln", "log", "max",
	"min", "Pr", "sec", "sin",
	"sinh", "sup", "tan", "tanh",
	"sqrt", "overline", "overrightarrow", "underline", "overleftarrow", "widehat", "overbrace", "widetilde", "underbrace"
};

// ===== Function Two-Arg Symbol Tables =====
const char* functionTwoArgSymbolTable[] = {
	"arctan2", "binom", "gcd", "lg", "log", "max", "min", "pow", "frac"
};

// ===== Relational Operators (관계 연산자) =====
// https://www.cmor-faculty.rice.edu/~heinken/latex/symbols.pdf 참고
const char* relationalOperatorSymbolTable[] = {
	"=", "<", ">", "leq", "geq", "neq",
	"approx", "sim", "simeq", "equiv", "cong",
	"ll", "gg", "doteq", "asymp", "propto",
	"perp", "parallel", "models", "mid", "vert"
};

// ===== Binary Operators (이항 연산자) =====
const char* binaryOperatorSymbolTable[] = {
	"+", "-", "*", "/", "^",
	"times", "div", "pm", "mp", "cdot",
	"ast", "star", "cap", "cup", "subseteq",
	"subset", "supseteq", "supset", "in", "notin",
	"bigcap", "bigcup", "circ", "bullet", "oplus",
	"ominus", "otimes", "oslash", "odot", "vee",
	"wedge", "setminus", "backslash", "wr", "sqcap",
	"sqcup", "triangleleft", "triangleright", "lhd", "rhd"
};

// ===== Arrows (화살표) =====
const char* arrowSymbolTable[] = {
	"rightarrow", "leftarrow", "leftrightarrow",
	"Rightarrow", "Leftarrow", "Leftrightarrow",
	"to", "gets", "mapsto", "longmapsto",
	"updownarrow", "Updownarrow", "uparrow", "downarrow",
	"nrightarrow", "nleftarrow", "dashrightarrow", "dashleftarrow",
	"leftharpoonup", "leftharpoondown", "rightharpoonup", "rightharpoondown",
	"leftrightharpoons", "rightleftharpoons", "nearrow", "searrow",
	"swarrow", "nwarrow", "Rrightarrow", "Lleftarrow"
};

// ===== Miscellaneous Symbols (기타 심볼) =====
const char* miscSymbolSymbolTable[] = {
	"infty", "emptyset", "varnothing", "partial", "nabla",
	"hbar", "ell", "imath", "jmath", "Re", "Im",
	"wp", "Bbbk", "oslash", "mho", "dagger", "ddagger",
	"heartsuit", "diamondsuit", "clubsuit", "spadesuit", "sharp",
	"flat", "natural", "smile", "frown", "wr",
	"dag", "ddag", "lozenge", "circledS", "top", "bot",
	"prime", "forall", "exists", "neg", "vert", "Vert",
	"angle", "therefore", "because", "checkmark", "square"
};

// ===== Logical Operators (논리 연산자) =====
const char* logicalOperatorSymbolTable[] = {
	"wedge", "vee", "neg", "forall", "exists",
	"therefore", "because", "Rightarrow", "Leftarrow",
	"Leftrightarrow"
};

// ===== Calligraphic Fonts (캘리그래피 폰트) =====
const char* calligraphicFontSymbolTable[] = {
	"mathcal", "mathbf", "mathit", "mathtt",
	"mathsf", "mathfrak", "mathbb", "mathscr",
	"textit", "textbf", "texttt", "textsf"
};

// ===== 행렬 환경 (Matrix Environments) =====
const char* matrixEnvironmentTable[] = {
	"matrix", "pmatrix", "bmatrix", "vmatrix", "Bmatrix", "Vmatrix",
	"smallmatrix"
};

// ===== LaTeX 명령어 미리보기 =====
// 현재 위치가 '\' + IDENTIFIER 형태인지 확인하고, 명령어 이름을 반환
std::string LatexMathEqParser::tryPeekLatexCommand() const {
	if (*position < tokens->size() &&
		(*tokens)[*position].type == TokenType::OPERATOR &&
		(*tokens)[*position].value == "\\" &&
		*position + 1 < tokens->size() &&
		(*tokens)[*position + 1].type == TokenType::IDENTIFIER) {
		return (*tokens)[*position + 1].value;
	}
	return "";
}

// ===== 연산자 우선순위 기반 파싱 =====
//
// 우선순위 (낮음 → 높음):
//   parseExpr        : 최상위 진입점
//   parseRelational  : =, <, >, <=, >=, !=, \leq, \geq, \neq, ...
//   parseAdditive    : +, -, \pm, \mp
//   parseMultiplicative : *, /, \times, \div, \cdot
//   parsePostfix     : ^(거듭제곱), _(첨자)
//   parseSimpleExpr  : 단항 연산자, 괄호, 중괄호
//   parseTerminal    : 숫자, 식별자, \명령어

std::unique_ptr<ASTNode> LatexMathEqParser::parseExpr() {
	return parseRelational();
}

// ===== 관계 연산자: =, <, >, <=, >=, !=, \leq, \geq, \neq 등 =====
std::unique_ptr<ASTNode> LatexMathEqParser::parseRelational() {
	auto left = parseAdditive();

	while (true) {
		if (isAtEnd() || check(TokenType::OPERATOR, "$") || check(TokenType::OPERATOR, "$$") ||
			check(TokenType::OPERATOR, ")") || check(TokenType::OPERATOR, "}") ||
			check(TokenType::OPERATOR, "&")) {
			break;
		}
		// 행 구분자 \\ 또는 \end 확인
		{
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "end" || cmd == "\\") break;
			if (check(TokenType::OPERATOR, "\\") && *position + 1 < tokens->size() &&
				(*tokens)[*position + 1].type == TokenType::OPERATOR && (*tokens)[*position + 1].value == "\\") {
				break;
			}
		}

		std::string op;

		// 단일 토큰 관계 연산자
		if (check(TokenType::OPERATOR, "=") || check(TokenType::OPERATOR, "<") ||
			check(TokenType::OPERATOR, ">") || check(TokenType::OPERATOR, "<=") ||
			check(TokenType::OPERATOR, ">=") || check(TokenType::OPERATOR, "!=")) {
			op = advance().value;
		}
		// LaTeX 명령어 관계 연산자 (\leq, \geq, \neq, \approx 등)
		else {
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "leq" || cmd == "geq" || cmd == "neq" ||
				cmd == "approx" || cmd == "sim" || cmd == "simeq" ||
				cmd == "equiv" || cmd == "cong" || cmd == "ll" || cmd == "gg" ||
				cmd == "doteq" || cmd == "asymp" || cmd == "propto") {
				advance(); // consume '\'
				op = advance().value; // consume command name
			}
			else {
				break;
			}
		}

		auto right = parseAdditive();
		left = std::make_unique<BinaryExpressionNode>(std::move(left), op, std::move(right));
	}

	return left;
}

// ===== 덧셈/뺄셈: +, -, \pm, \mp =====
std::unique_ptr<ASTNode> LatexMathEqParser::parseAdditive() {
	auto left = parseMultiplicative();

	while (true) {
		if (isAtEnd() || check(TokenType::OPERATOR, "$") || check(TokenType::OPERATOR, "$$") ||
			check(TokenType::OPERATOR, ")") || check(TokenType::OPERATOR, "}") ||
			check(TokenType::OPERATOR, "&")) {
			break;
		}
		// 행 구분자 \\ 또는 \end 확인
		{
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "end" || cmd == "\\") break;
			if (check(TokenType::OPERATOR, "\\") && *position + 1 < tokens->size() &&
				(*tokens)[*position + 1].type == TokenType::OPERATOR && (*tokens)[*position + 1].value == "\\") {
				break;
			}
		}

		std::string op;

		if (check(TokenType::OPERATOR, "+") || check(TokenType::OPERATOR, "-")) {
			op = advance().value;
		}
		else {
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "pm" || cmd == "mp") {
				advance(); // consume '\'
				op = advance().value;
			}
			else {
				break;
			}
		}

		auto right = parseMultiplicative();
		left = std::make_unique<BinaryExpressionNode>(std::move(left), op, std::move(right));
	}

	return left;
}

// ===== 곱셈/나눗셈: *, /, \times, \div, \cdot =====
std::unique_ptr<ASTNode> LatexMathEqParser::parseMultiplicative() {
	auto left = parsePostfix();

	while (true) {
		if (isAtEnd() || check(TokenType::OPERATOR, "$") || check(TokenType::OPERATOR, "$$") ||
			check(TokenType::OPERATOR, ")") || check(TokenType::OPERATOR, "}") ||
			check(TokenType::OPERATOR, "&")) {
			break;
		}
		// 행 구분자 \\ 또는 \end 확인
		{
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "end" || cmd == "\\") break;
			if (check(TokenType::OPERATOR, "\\") && *position + 1 < tokens->size() &&
				(*tokens)[*position + 1].type == TokenType::OPERATOR && (*tokens)[*position + 1].value == "\\") {
				break;
			}
		}

		std::string op;

		if (check(TokenType::OPERATOR, "*") || check(TokenType::OPERATOR, "/")) {
			op = advance().value;
		}
		else {
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "times" || cmd == "div" || cmd == "cdot") {
				advance(); // consume '\'
				op = advance().value; // "times" / "div" / "cdot"
				// In math these are ordinary arithmetic: \cdot and \times multiply, \div divides.
				// Emit the standard operators so codegen evaluates them (the raw command names have
				// no codegen handler and would evaluate to 0).
				if (op == "cdot" || op == "times") {
					op = "*";
				}
				else if (op == "div") {
					op = "/";
				}
			}
			else {
				break;
			}
		}

		auto right = parsePostfix();
		left = std::make_unique<BinaryExpressionNode>(std::move(left), op, std::move(right));
	}

	return left;
}

// ===== 후위 연산자: ^(거듭제곱), _(첨자) =====
std::unique_ptr<ASTNode> LatexMathEqParser::parsePostfix() {
	auto node = parseSimpleExpr();

	// ^와 _는 동일 항에 모두 붙을 수 있음: x_i^2, x^2_i
	while (true) {
		if (check(TokenType::OPERATOR, "^")) {
			advance(); // consume '^'
			auto exponent = parseArg();
			// In math `^` is exponentiation; emit the power operator `**` (CSEC's `^` is bitwise XOR,
			// which returns 0 on the floating-point operands typical of a formula).
			node = std::make_unique<BinaryExpressionNode>(std::move(node), "**", std::move(exponent));
		}
		else if (check(TokenType::OPERATOR, "_")) {
			advance(); // consume '_'
			auto subscript = parseArg();

			// 식별자에 첨자가 붙으면 이름에 합침: x_i → "x_i"
			if (node && node->nodeType == ASTNodeType::IDENTIFIER &&
				subscript && subscript->nodeType == ASTNodeType::IDENTIFIER) {
				auto* idNode = dynamic_cast<IdentifierNode*>(node.get());
				auto* subNode = dynamic_cast<IdentifierNode*>(subscript.get());
				if (idNode && subNode) {
					node = std::make_unique<IdentifierNode>(idNode->value + "_" + subNode->value);
				}
			}
			else if (node && node->nodeType == ASTNodeType::IDENTIFIER &&
				subscript && subscript->nodeType == ASTNodeType::VALUE) {
				auto* idNode = dynamic_cast<IdentifierNode*>(node.get());
				auto* valNode = dynamic_cast<ValueNode*>(subscript.get());
				if (idNode && valNode) {
					node = std::make_unique<IdentifierNode>(idNode->value + "_" + valNode->value);
				}
			}
			else {
				// 일반적인 경우: 이항 연산으로 처리
				node = std::make_unique<BinaryExpressionNode>(std::move(node), "_", std::move(subscript));
			}
		}
		else {
			break;
		}
	}

	return node;
}

std::unique_ptr<ASTNode> LatexMathEqParser::parseTerminal() {
	if (match(TokenType::OPERATOR, "\\")) {
		if (check(TokenType::IDENTIFIER)) {
			std::string command = advance().value;

			// 명령어가 '_'로 끝나는 경우, 명령어에서 제거하고, 다음 token을 첨자 인자로 처리
			if (!command.empty() && command.back() == '_') {
				command = command.substr(0, command.size() - 1);
				(*tokens)[*position - 1].value = command;
				tokens->insert(tokens->begin() + *position, Token{ TokenType::OPERATOR, "_", peek().line, peek().column });
			}

			// \left 구분자 처리
			if (command == "left") {
				// \left 뒤의 구분자 소비 (, [, {, |, .
				if (!isAtEnd()) {
					advance(); // consume delimiter
				}
				auto expr = parseExpr();
				// \right 소비
				if (check(TokenType::OPERATOR, "\\")) {
					std::string nextCmd = tryPeekLatexCommand();
					if (nextCmd == "right") {
						advance(); // consume '\'
						advance(); // consume 'right'
						if (!isAtEnd()) {
							advance(); // consume delimiter ), ], }, |, .
						}
					}
				}
				return expr;
			}

			// \begin{...} 환경 처리
			if (command == "begin") {
				if (!match(TokenType::OPERATOR, "{")) {
					throw std::runtime_error("Expected '{' after \\begin");
				}
				if (!check(TokenType::IDENTIFIER)) {
					throw std::runtime_error("Expected environment name after \\begin{");
				}
				std::string envName = advance().value;
				if (!match(TokenType::OPERATOR, "}")) {
					throw std::runtime_error("Expected '}' after environment name");
				}
				if (isMatrixEnvironment(envName)) {
					return parseMatrixEnvironment(envName);
				}
				// 알 수 없는 환경은 식별자로 처리
				return std::make_unique<IdentifierNode>("begin_" + envName);
			}

			// 폰트 명령어 처리 (예: \mathbb, \mathcal, \mathbf)
			if (isCalligraphicFont(command)) {
				auto arg = parseArg(true);

				// 인자가 식별자인 경우 특수 심볼로 매핑
				if (arg && arg->nodeType == ASTNodeType::IDENTIFIER) {
					auto* idArg = dynamic_cast<IdentifierNode*>(arg.get());
					if (!idArg) return std::move(arg);
					std::string letter = idArg->value;

					// \mathbb{X} → 수학적 수 집합 매핑
					if (command == "mathbb") {
						if (letter == "R") return std::make_unique<IdentifierNode>("Real");
						if (letter == "C") return std::make_unique<IdentifierNode>("Complex");
						if (letter == "N") return std::make_unique<IdentifierNode>("Natural");
						if (letter == "Z") return std::make_unique<IdentifierNode>("Integer");
						if (letter == "Q") return std::make_unique<IdentifierNode>("Rational");
						if (letter == "P") return std::make_unique<IdentifierNode>("Prime");
						if (letter == "F") return std::make_unique<IdentifierNode>("Field");
						if (letter == "H") return std::make_unique<IdentifierNode>("Quaternion");
						// 알 수 없는 문자는 접두사 형태로
						return std::make_unique<IdentifierNode>("mathbb_" + letter);
					}

					// \mathcal{X} → 캘리그래피 심볼 매핑
					if (command == "mathcal") {
						if (letter == "L") return std::make_unique<IdentifierNode>("Lagrangian");
						if (letter == "F") return std::make_unique<IdentifierNode>("Fourier");
						if (letter == "O") return std::make_unique<IdentifierNode>("BigO");
						if (letter == "P") return std::make_unique<IdentifierNode>("PowerSet");
						if (letter == "H") return std::make_unique<IdentifierNode>("Hilbert");
						if (letter == "B") return std::make_unique<IdentifierNode>("Borel");
						return std::make_unique<IdentifierNode>("mathcal_" + letter);
					}

					// \mathbf{X} → 볼드 (벡터/행렬)
					if (command == "mathbf") {
						return std::make_unique<IdentifierNode>("bf_" + letter);
					}

					// \mathit{X} → 이탤릭
					if (command == "mathit") {
						return std::make_unique<IdentifierNode>("it_" + letter);
					}

					// \mathfrak{X} → 프락투르 심볼 매핑
					if (command == "mathfrak") {
						if (letter == "g") return std::make_unique<IdentifierNode>("LieAlgebra_g");
						if (letter == "h") return std::make_unique<IdentifierNode>("LieAlgebra_h");
						if (letter == "p") return std::make_unique<IdentifierNode>("PrimeIdeal");
						if (letter == "m") return std::make_unique<IdentifierNode>("MaximalIdeal");
						return std::make_unique<IdentifierNode>("mathfrak_" + letter);
					}

					// \mathscr{X} → 스크립트
					if (command == "mathscr") {
						return std::make_unique<IdentifierNode>("mathscr_" + letter);
					}

					// 기타 폰트 명령어: 접두사_문자 형태로
					return std::make_unique<IdentifierNode>(command + "_" + letter);
				}

				// 인자가 식별자가 아닌 경우 (예: \mathbf{x+y}) → 함수 호출로 처리
				auto node = std::make_unique<FunctionCallNode>();
				node->functionName = command;
				node->arguments.push_back(std::move(arg));
				return node;
			}

			// Limit-like operators accept optional lower/upper bounds before the body:
			// \lim_{x=0} f(x), \limsup^n a_n, etc.
			if (command == "lim" || command == "liminf" || command == "limsup") {
				auto node = std::make_unique<FunctionCallNode>();
				node->functionName = command;

				std::unique_ptr<ASTNode> lower = nullptr;
				std::unique_ptr<ASTNode> upper = nullptr;
				while (check(TokenType::OPERATOR, "^") || check(TokenType::OPERATOR, "_")) {
					if (match(TokenType::OPERATOR, "^")) {
						upper = parseArg(true);
					}
					else if (match(TokenType::OPERATOR, "_")) {
						lower = parseArg(true);
					}
				}

				node->arguments.push_back(lower ? std::move(lower) : std::make_unique<UnitNode>());
				node->arguments.push_back(upper ? std::move(upper) : std::make_unique<UnitNode>());

				if (!isAtEnd() &&
					!check(TokenType::OPERATOR, "$") &&
					!check(TokenType::OPERATOR, "$$") &&
					!check(TokenType::OPERATOR, ")") &&
					!check(TokenType::OPERATOR, "}") &&
					!check(TokenType::OPERATOR, "&")) {
					auto body = parseSimpleExpr();
					node->arguments.push_back(body ? std::move(body) : std::make_unique<UnitNode>());
				}
				else {
					node->arguments.push_back(std::make_unique<UnitNode>());
				}

				return node;
			}

			// 두 개 인자 함수 처리
			for (int i = 0; i < sizeof(functionTwoArgSymbolTable) / sizeof(functionTwoArgSymbolTable[0]); ++i) {
				if (command == functionTwoArgSymbolTable[i]) {
					auto firstArg = parseArg();
					auto secondArg = parseArg();

					auto node = std::make_unique<FunctionCallNode>();
					node->functionName = command;
					node->arguments.push_back(firstArg ? std::move(firstArg) : std::make_unique<UnitNode>());
					node->arguments.push_back(secondArg ? std::move(secondArg) : std::make_unique<UnitNode>());
					return node;
				}
			}

			// 한 개 인자 함수 처리
			for (int i = 0; i < sizeof(functionOneArgSymbolTable) / sizeof(functionOneArgSymbolTable[0]); ++i) {
				if (command == functionOneArgSymbolTable[i]) {
					auto arg = parseArg();

					auto node = std::make_unique<FunctionCallNode>();
					node->functionName = command;
					node->arguments.push_back(arg ? std::move(arg) : std::make_unique<UnitNode>());
					return node;
				}
			}

			// 그리스 문자 처리
			for (int i = 0; i < sizeof(greekLetterSymbolTable) / sizeof(greekLetterSymbolTable[0]); ++i) {
				if (command == greekLetterSymbolTable[i]) {
					return std::make_unique<IdentifierNode>(command);
				}
			}

			// 상수 처리
			for (int i = 0; i < sizeof(constantSymbolTable) / sizeof(constantSymbolTable[0]); ++i) {
				if (command == constantSymbolTable[i]) {
					return std::make_unique<IdentifierNode>(command);
				}
			}

			// 관계 연산자 처리 (단독 출현 시 식별자로)
			if (isRelationalOperator(command)) {
				return std::make_unique<IdentifierNode>(command);
			}

			// 이항 연산자 처리
			if (isBinaryOperator(command)) {
				return std::make_unique<IdentifierNode>(command);
			}

			// 화살표 처리
			if (isArrow(command)) {
				return std::make_unique<IdentifierNode>(command);
			}

			// 논리 연산자 처리
			if (isLogicalOperator(command)) {
				return std::make_unique<IdentifierNode>(command);
			}

			// 기타 심볼 처리 (infty, emptyset, partial, nabla 등)
			if (isMiscSymbol(command)) {
				return std::make_unique<IdentifierNode>(command);
			}

			// 집계 함수 처리 (sum, int, prod 등)
			for (int i = 0; i < sizeof(aggregateFunctionSymbolTable) / sizeof(aggregateFunctionSymbolTable[0]); ++i) {
				if (command == aggregateFunctionSymbolTable[i]) {
					if (command == "sum") {
						auto node = std::make_unique<ForStatementNode>();

						// upper / lower limit 처리
						std::unordered_map<std::string, std::unique_ptr<ASTNode>> argsMap;
						argsMap["upper"] = nullptr;
						argsMap["lower"] = nullptr;

						while (check(TokenType::OPERATOR, "^") || check(TokenType::OPERATOR, "_")) {
							if (match(TokenType::OPERATOR, "^")) {
								argsMap["upper"] = parseArg(true);
							}
							else if (match(TokenType::OPERATOR, "_")) {
								argsMap["lower"] = parseArg(true);
							}
						}

						// lower limit에서 변수와 시작값 추출
						std::unique_ptr<ASTNode> startExpr = nullptr;

						if (argsMap["lower"]) {
							if (argsMap["lower"]->nodeType == ASTNodeType::BINARY_EXPRESSION) {
								auto* binExpr = dynamic_cast<BinaryExpressionNode*>(argsMap["lower"].get());
								if (!binExpr) {
									throw std::runtime_error("Failed to cast lower limit to BinaryExpressionNode.");
								}
								if (!binExpr->left || !binExpr->right) {
									throw std::runtime_error("Expected 'variable = start' in lower limit.");
								}
								if (binExpr->op == "=") {
									if (binExpr->left->nodeType == ASTNodeType::IDENTIFIER) {
										auto* idNode = dynamic_cast<IdentifierNode*>(binExpr->left.get());
										if (!idNode) {
											throw std::runtime_error("Failed to cast left side of '=' to IdentifierNode.");
										}
										node->variable = idNode->value;
										startExpr = std::move(binExpr->right);
									}
									else {
										throw std::runtime_error("Expected identifier on the left side of '=' in lower limit.");
									}
								}
								else {
									throw std::runtime_error("Expected '=' operator in lower limit.");
								}
							}
							else if (argsMap["lower"]->nodeType == ASTNodeType::FOR_STATEMENT) {
								auto* forStmt = dynamic_cast<ForStatementNode*>(argsMap["lower"].get());
								if (!forStmt) {
									throw std::runtime_error("Failed to cast lower limit to ForStatementNode.");
								}
								node->variable = forStmt->variable;
								startExpr = std::move(forStmt->iterableExpr);
							}
							else {
								throw std::runtime_error("Expected binary expression in lower limit.");
							}
						}
						else {
							node->variable = "_";
							startExpr = std::make_unique<UnitNode>();
						}

						// upper + lower를 RangeExpressionNode로 합침
						if (argsMap["upper"] && startExpr) {
							node->iterableExpr = std::make_unique<RangeExpressionNode>(
								std::move(startExpr), std::move(argsMap["upper"]), true);
							node->isRange = true;
						}
						else if (argsMap["upper"]) {
							node->iterableExpr = std::move(argsMap["upper"]);
						}
						else if (startExpr) {
							node->iterableExpr = std::move(startExpr);
						}
						else {
							node->iterableExpr = std::make_unique<UnitNode>();
						}

						auto blockNode = std::make_unique<BlockNode>();
						blockNode->statements.push_back(parseSimpleExpr());
						node->body = std::move(blockNode);

						return node;
					}
					else {
						auto node = std::make_unique<FunctionCallNode>();

						node->functionName = command;

						// upper / lower limit 처리
						std::unordered_map<std::string, std::unique_ptr<ASTNode>> argsMap;
						argsMap["upper"] = nullptr;
						argsMap["lower"] = nullptr;

						while (check(TokenType::OPERATOR, "^") || check(TokenType::OPERATOR, "_")) {
							if (match(TokenType::OPERATOR, "^")) {
								argsMap["upper"] = parseArg(true);
							}
							else if (match(TokenType::OPERATOR, "_")) {
								argsMap["lower"] = parseArg(true);
							}
						}

						if (argsMap["lower"]) {
							node->arguments.push_back(std::move(argsMap["lower"]));
						}
						else {
							node->arguments.push_back(std::make_unique<UnitNode>());
						}

						if (argsMap["upper"]) {
							node->arguments.push_back(std::move(argsMap["upper"]));
						}
						else {
							node->arguments.push_back(std::make_unique<UnitNode>());
						}

						node->arguments.push_back(parseSimpleExpr());

						return node;
					}
				}
			}

			// 인식할 수 없는 명령어는 식별자로 처리
			return std::make_unique<IdentifierNode>(command);
		}
		else {
			throw std::runtime_error("Expected command name after '\\'");
		}
	}
	else if (match(TokenType::INTEGER_LITERAL) || match(TokenType::FLOAT_LITERAL) ||
		match(TokenType::EXPONENTIAL_LITERAL) || match(TokenType::HEX_LITERAL) ||
		match(TokenType::BINARY_LITERAL) || match(TokenType::OCTAL_LITERAL)) {
		// 숫자 리터럴 처리
		double v = 0.0;

		switch (previous().type) {
		case TokenType::INTEGER_LITERAL:
			v = (double)std::stoi(previous().value);
			break;
		case TokenType::FLOAT_LITERAL:
			v = std::stod(previous().value);
			break;
		case TokenType::EXPONENTIAL_LITERAL:
			v = std::stod(previous().value);
			break;
		case TokenType::HEX_LITERAL:
			v = (double)std::stoul(previous().value, nullptr, 16);
			break;
		case TokenType::BINARY_LITERAL:
			v = (double)std::stoul(previous().value, nullptr, 2);
			break;
		case TokenType::OCTAL_LITERAL:
			v = (double)std::stoul(previous().value, nullptr, 8);
			break;
		default:
			break;
		}

		std::string value = std::to_string(v);
		return std::make_unique<ValueNode>(value, TokenType::FLOAT_LITERAL);
	}
	else if (match(TokenType::IDENTIFIER)) {
		// 식별자 처리
		return std::make_unique<IdentifierNode>(previous().value);
	}
	return nullptr;
}

std::unique_ptr<ASTNode> LatexMathEqParser::parseSimpleExpr() {
	// 괄호 그룹: ( expr )
	if (match(TokenType::OPERATOR, "(")) {
		auto expr = parseExpr();
		match(TokenType::OPERATOR, ")");
		return expr;
	}
	// 중괄호 그룹: { expr }
	else if (match(TokenType::OPERATOR, "{")) {
		auto expr = parseExpr();
		match(TokenType::OPERATOR, "}");
		return expr;
	}
	// 단항 연산자: -, +
	else if (check(TokenType::OPERATOR, "-") || check(TokenType::OPERATOR, "+")) {
		std::string op = advance().value; // 연산자를 먼저 저장
		auto operand = parsePostfix();
		return std::make_unique<UnaryExpressionNode>(op, std::move(operand));
	}

	auto terminal = parseTerminal();
	if (!terminal) {
		throw std::runtime_error("Expected an operand in LaTeX expression.");
	}
	return terminal;
}

std::unique_ptr<ASTNode> LatexMathEqParser::parseArg(bool isBrace) {
	// 파라미터 필수 함수 (중괄호 필수)
	if (isBrace && !check(TokenType::OPERATOR, "{")) {
		throw std::runtime_error("Expected '{' to start argument.");
	}

	if (match(TokenType::OPERATOR, "{")) {
		auto expr = parseExpr();
		match(TokenType::OPERATOR, "}");
		return expr;
	}
	else if (match(TokenType::OPERATOR, "(")) {
		auto expr = parseExpr();
		match(TokenType::OPERATOR, ")");
		return expr;
	}
	else {
		auto terminal = parseTerminal();
		return terminal ? std::move(terminal) : std::make_unique<UnitNode>();
	}

	return nullptr;
}

// ===== LaTeX 심볼 인식 함수들 =====

bool LatexMathEqParser::isRelationalOperator(const std::string& symbol) const {
	for (int i = 0; i < sizeof(relationalOperatorSymbolTable) / sizeof(relationalOperatorSymbolTable[0]); ++i) {
		if (symbol == relationalOperatorSymbolTable[i]) {
			return true;
		}
	}
	return false;
}

bool LatexMathEqParser::isBinaryOperator(const std::string& symbol) const {
	for (int i = 0; i < sizeof(binaryOperatorSymbolTable) / sizeof(binaryOperatorSymbolTable[0]); ++i) {
		if (symbol == binaryOperatorSymbolTable[i]) {
			return true;
		}
	}
	return false;
}

bool LatexMathEqParser::isArrow(const std::string& symbol) const {
	for (int i = 0; i < sizeof(arrowSymbolTable) / sizeof(arrowSymbolTable[0]); ++i) {
		if (symbol == arrowSymbolTable[i]) {
			return true;
		}
	}
	return false;
}

bool LatexMathEqParser::isMiscSymbol(const std::string& symbol) const {
	for (int i = 0; i < sizeof(miscSymbolSymbolTable) / sizeof(miscSymbolSymbolTable[0]); ++i) {
		if (symbol == miscSymbolSymbolTable[i]) {
			return true;
		}
	}
	return false;
}

bool LatexMathEqParser::isLogicalOperator(const std::string& symbol) const {
	for (int i = 0; i < sizeof(logicalOperatorSymbolTable) / sizeof(logicalOperatorSymbolTable[0]); ++i) {
		if (symbol == logicalOperatorSymbolTable[i]) {
			return true;
		}
	}
	return false;
}

bool LatexMathEqParser::isCalligraphicFont(const std::string& symbol) const {
	for (int i = 0; i < sizeof(calligraphicFontSymbolTable) / sizeof(calligraphicFontSymbolTable[0]); ++i) {
		if (symbol == calligraphicFontSymbolTable[i]) {
			return true;
		}
	}
	return false;
}

bool LatexMathEqParser::isMatrixEnvironment(const std::string& name) const {
	for (int i = 0; i < sizeof(matrixEnvironmentTable) / sizeof(matrixEnvironmentTable[0]); ++i) {
		if (name == matrixEnvironmentTable[i]) {
			return true;
		}
	}
	return false;
}

std::unique_ptr<ASTNode> LatexMathEqParser::parseMatrixEnvironment(const std::string& envType) {
	// 행렬 환경의 행과 열을 파싱
	// 행 구분자: \\ (연속 백슬래시 두 개)
	// 열 구분자: &
	// 종료: \end{envType}

	auto matrix = std::make_unique<ArrayLiteralNode>();

	while (true) {
		if (isAtEnd()) {
			throw std::runtime_error("Unexpected end of input inside matrix environment '" + envType + "'");
		}

		// \end{envType} 확인
		{
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "end") {
				advance(); // consume '\'
				advance(); // consume 'end'
				if (!match(TokenType::OPERATOR, "{")) {
					throw std::runtime_error("Expected '{' after \\end");
				}
				if (!check(TokenType::IDENTIFIER) || peek().value != envType) {
					throw std::runtime_error("Expected \\end{" + envType + "}");
				}
				advance(); // consume envType
				if (!match(TokenType::OPERATOR, "}")) {
					throw std::runtime_error("Expected '}' after \\end{" + envType);
				}
				break;
			}
		}

		// 한 행 파싱
		auto row = std::make_unique<ArrayLiteralNode>();

		// 첫 번째 셀 파싱
		auto cell = parseExpr();
		if (!cell) {
			throw std::runtime_error("Expected matrix cell expression in \\begin{" + envType + "}");
		}
		row->elements.push_back(std::move(cell));

		// 나머지 셀 파싱 (& 구분자)
		while (match(TokenType::OPERATOR, "&")) {
			cell = parseExpr();
			if (!cell) {
				throw std::runtime_error("Expected matrix cell expression after '&' in \\begin{" + envType + "}");
			}
			row->elements.push_back(std::move(cell));
		}

		if (row->elements.empty()) {
			throw std::runtime_error("Empty matrix row in \\begin{" + envType + "}");
		}

		matrix->elements.push_back(std::move(row));

		// 행 구분자 \\ 확인 (연속 OPERATOR '\' 토큰 두 개)
		if (check(TokenType::OPERATOR, "\\") && *position + 1 < tokens->size() &&
			(*tokens)[*position + 1].type == TokenType::OPERATOR && (*tokens)[*position + 1].value == "\\") {
			advance(); // consume first '\'
			advance(); // consume second '\'
			continue;
		}

		// \\ 가 LaTeX 명령어로 처리되는 경우
		{
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "\\") {
				advance(); // consume '\'
				advance(); // consume '\\'
				continue;
			}
		}

		// \end 확인 (행 구분자 없이 마지막 행)
		{
			std::string cmd = tryPeekLatexCommand();
			if (cmd == "end") {
				continue; // 루프 상단에서 \end 처리
			}
		}

		throw std::runtime_error("Expected '\\\\' or \\end{" + envType + "} in matrix environment");
	}

	return matrix;
}

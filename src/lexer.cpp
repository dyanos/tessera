// lexer.cpp
#include "lexer.h"
#include <cctype>

namespace {
bool isAsciiIdentifierStart(unsigned char ch) {
    return (ch == '_') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

bool isAsciiIdentifierContinue(unsigned char ch) {
    return isAsciiIdentifierStart(ch) || (ch >= '0' && ch <= '9');
}

int utf8SequenceLength(unsigned char lead) {
    if ((lead & 0x80) == 0x00) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 0;
}

bool isUtf8Continuation(unsigned char ch) {
    return (ch & 0xC0) == 0x80;
}

bool consumeUtf8CodePoint(const std::string& text, size_t pos, int* outLen) {
    if (pos >= text.size()) {
        return false;
    }

    unsigned char lead = static_cast<unsigned char>(text[pos]);
    int len = utf8SequenceLength(lead);
    if (len <= 1) {
        return false;
    }
    if (pos + static_cast<size_t>(len) > text.size()) {
        return false;
    }
    for (int i = 1; i < len; ++i) {
        unsigned char cont = static_cast<unsigned char>(text[pos + i]);
        if (!isUtf8Continuation(cont)) {
            return false;
        }
    }
    *outLen = len;
    return true;
}

std::string decodeStringEscapes(const std::string& text) {
    std::string decoded;
    decoded.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '\\' || i + 1 >= text.size()) {
            decoded.push_back(text[i]);
            continue;
        }

        char escaped = text[++i];
        switch (escaped) {
        case 'n': decoded.push_back('\n'); break;
        case 'r': decoded.push_back('\r'); break;
        case 't': decoded.push_back('\t'); break;
        case '\\': decoded.push_back('\\'); break;
        case '"': decoded.push_back('"'); break;
        case '0': decoded.push_back('\0'); break;
        default:
            decoded.push_back(escaped);
            break;
        }
    }
    return decoded;
}
}

Lexer::Lexer(const std::string& source)
    : source(source), position(0), line(1), column(1) {
    initializeKeywords();
    initializeOperators();
}

void Lexer::initializeKeywords() {
    std::string kws[] = {
        // Core language keywords actually used by the grammar. (Removed a block of Scala-heritage
        // words -- abstract/catch/do/final/finally/forSome/implicit/lazy/package/private/protected/
        // sealed/throw/trait/try/type/with/yield -- that were reserved but unused, so common names
        // like `type`, `do`, `final`, and `private` are now usable as identifiers.)
        "case", "class", "def", "else", "extends",
        "false", "for", "if",
        "import", "match", "new", "null", "object", "override",
        "operator", "return", "super",
        "this", "true", "val", "var",
		"while", "to", "until", "and", "or", "xor", "map", "pmap", "reduce", "preduce", "filter", "external", "inline", "constexpr",
		"template", "typename", "mut", "box", "unsafe", "unatomic", "struct",
        "async", "await", "go", "build", "break", "continue", "enum",
        // Physics-DSL entry keywords (dispatch a simulation block from statement position). The body
        // words that appear only inside these blocks -- atom/bond/at/steps/dt/temperature/grid/
        // viscosity/velocity/euler/from/step/lattice/spacing/chain/mcmc -- are NOT reserved; the DSL
        // parser matches them contextually (Parser::matchWord), so they remain ordinary identifiers.
        "molecule", "cfd", "ode", "protein"
    };
    keywords.insert(kws, kws + sizeof(kws) / sizeof(kws[0]));
}

void Lexer::initializeOperators() {
    // Canonical operator set (see docs/grammar.md). The self-host lexer keeps a matching multi-character
    // list in native_runtime/lexer_runtime.inc (csec_operator_length); keep the two in sync when adding
    // or removing an operator. Matched maximal-munch (longest operator first) in matchOperator().
    std::string ops[] = {
		"=", "==", "!=", "+", "-", "*", "/", "%", "$", "$$", "\\", "<", ">", "<=", ">=", "<<", ">>", "<<|", ">>|",
		"!", "&", "|", "^", "~", ":", ".", ",", ";", "(", ")", "{", "}", "[", "]", "[@", "<-", "->",
		"++", "--", "+=", "-=", "*=", "/=", "%=", "**", "_", "=>", "<:", "<%", ">:", "#", "@", "'", "?"
    };
    operators.insert(ops, ops + sizeof(ops) / sizeof(ops[0]));
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (position < source.length()) {
        if (matchComment(tokens)) continue;
        if (matchWhitespace(tokens)) continue;
        if (matchNewline(tokens)) continue;
        if (matchPrefixedStringLiteral(tokens)) continue;
        if (matchStringLiteral(tokens)) continue;
        if (matchCharLiteral(tokens)) continue;
        if (matchNumberLiteral(tokens)) continue;
        if (matchIdentifierOrKeyword(tokens)) continue;
        if (matchOperator(tokens)) continue;

        tokens.push_back(Token{ TokenType::UNKNOWN, std::string(1, source[position]), line, column });
        advance();
    }

    tokens.push_back(Token{ TokenType::END_OF_FILE, "", line, column });
    return tokens;
}

void Lexer::advance(int steps) {
    for (int i = 0; i < steps && position < source.size(); ++i) {
        if (source[position] == '\n') {
            ++line;
            column = 1;
        }
        else {
            ++column;
        }
        ++position;
    }
}

char Lexer::peek(int offset) const {
    if (position + offset < source.length()) {
        return source[position + offset];
    }
    else {
        return '\0';
    }
}

bool Lexer::matchComment(std::vector<Token>& tokens) {
    if (peek() == '/' && peek(1) == '/') {
        size_t start = position;
        while (peek() != '\n' && peek() != '\0') {
            advance();
        }
        std::string comment = source.substr(start, position - start);
        tokens.push_back(Token{ TokenType::COMMENT, comment, line, column });
        return true;
    }
    else if (peek() == '/' && peek(1) == '*') {
        size_t start = position;
        advance(2);
        while (!(peek() == '*' && peek(1) == '/')) {
            if (peek() == '\0') {
                break;
            }
            advance();
        }
        if (peek() == '*' && peek(1) == '/') {
            advance(2);
        }
        std::string comment = source.substr(start, position - start);
        tokens.push_back(Token{ TokenType::COMMENT, comment, line, column });
        return true;
    }
    return false;
}

bool Lexer::matchWhitespace(std::vector<Token>& tokens) {
    if (std::isspace(static_cast<unsigned char>(peek())) && peek() != '\n') {
        //size_t start = position;
        while (std::isspace(static_cast<unsigned char>(peek())) && peek() != '\n') {
            advance();
        }
        //std::string whitespace = source.substr(start, position - start);
        //tokens.push_back(Token{ TokenType::WHITESPACE, whitespace, line, column });
        return true;
    }
    return false;
}

bool Lexer::matchNewline(std::vector<Token>& tokens) {
    if (peek() == '\n') {
        while (peek() == '\n') {
            advance();
        }
        //tokens.push_back(Token{ TokenType::NEWLINE, "\n", line, column });
        //advance();
        return true;
    }
    return false;
}

bool Lexer::matchStringLiteral(std::vector<Token>& tokens) {
    if (peek() == '"') {
        size_t start = position;
        advance();
        while (peek() != '"' && peek() != '\0') {
            if (peek() == '\\') {
                advance(2);  // ?좎떛?숈삕?좎룞?쇿뜝?숈삕?좎룞???좎룞?쇿뜝?숈삕 ?좎룞?숉궢
            }
            else {
                advance();
            }
        }
        advance();  // ?좎뙠?먯삕 ?좎룞?쇿뜝?숈삕???좎룞?숉궢
        std::string strLiteral = decodeStringEscapes(source.substr(start + 1, position - start - 2));
        // `${name}` is deliberately lowered in the lexer into ordinary string addition.  This keeps
        // interpolation usable anywhere a string expression is accepted and lets the existing
        // concat-chain codegen coalesce all fragments into one allocation.
        size_t fragmentStart = 0;
        bool emittedInterpolation = false;
        for (size_t i = 0; i + 3 <= strLiteral.size(); ++i) {
            if (strLiteral[i] != '$' || strLiteral[i + 1] != '{') continue;
            size_t nameStart = i + 2;
            if (nameStart >= strLiteral.size() || !isAsciiIdentifierStart(static_cast<unsigned char>(strLiteral[nameStart]))) continue;
            size_t nameEnd = nameStart + 1;
            while (nameEnd < strLiteral.size() && isAsciiIdentifierContinue(static_cast<unsigned char>(strLiteral[nameEnd]))) ++nameEnd;
            if (nameEnd >= strLiteral.size() || strLiteral[nameEnd] != '}') continue;
            if (i > fragmentStart) tokens.push_back(Token{ TokenType::STRING_LITERAL, strLiteral.substr(fragmentStart, i - fragmentStart), line, column });
            if (i > fragmentStart) tokens.push_back(Token{ TokenType::OPERATOR, "+", line, column });
            tokens.push_back(Token{ TokenType::IDENTIFIER, strLiteral.substr(nameStart, nameEnd - nameStart), line, column });
            tokens.push_back(Token{ TokenType::OPERATOR, "+", line, column });
            fragmentStart = nameEnd + 1;
            i = nameEnd;
            emittedInterpolation = true;
        }
        if (emittedInterpolation) {
            tokens.push_back(Token{ TokenType::STRING_LITERAL, strLiteral.substr(fragmentStart), line, column });
        }
        else {
            tokens.push_back(Token{ TokenType::STRING_LITERAL, strLiteral, line, column });
        }
        return true;
    }
    return false;
}

bool Lexer::matchPrefixedStringLiteral(std::vector<Token>& tokens) {
    char prefix = peek();
    if ((prefix != 'r' && prefix != 'R' && prefix != 'u' && prefix != 'U') || peek(1) != '"') {
        return false;
    }

    const bool raw = prefix == 'r' || prefix == 'R';
    size_t start = position;
    advance(2);
    while (peek() != '"' && peek() != '\0') {
        if (!raw && peek() == '\\') {
            advance(2);
        }
        else {
            advance();
        }
    }

    if (peek() == '"') {
        advance();
    }

    std::string literal = source.substr(start + 2, position - start - 3);
    if (!raw) {
        literal = decodeStringEscapes(literal);
    }
    tokens.push_back(Token{ raw ? TokenType::REGEX_LITERAL : TokenType::STRING_LITERAL, literal, line, column });
    return true;
}

bool Lexer::matchCharLiteral(std::vector<Token>& tokens) {
    if (peek() == '\'') {
        size_t savedPos = position;
        int savedLine = line;
        int savedCol = column;
        advance();
        if (peek() == '\\') {
            advance(2);  // ?좎떛?숈삕?좎룞?쇿뜝?숈삕?좎룞???좎룞?쇿뜝?숈삕
        }
        else {
            advance();
        }
        if (peek() == '\'') {
            advance();
            std::string charLiteral = source.substr(savedPos, position - savedPos);
            tokens.push_back(Token{ TokenType::CHAR_LITERAL, charLiteral, line, column });
            return true;
        }
        position = savedPos;
        line = savedLine;
        column = savedCol;
        // ?좎룞?쇿뜝?숈삕 ?좎룞?쇿뜝?숈삕 泥섇뜝?숈삕
    }
    return false;
}

bool Lexer::matchNumberLiteral(std::vector<Token>& tokens) {
    if (std::isdigit(static_cast<unsigned char>(peek()))) {
        size_t start = position;
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
        // ?좎룞?쇿뜝?숈삕 ?좎룞??16?좎룞?쇿뜝?숈삕, 2?좎룞?쇿뜝?숈삕, 8?좎룞?쇿뜝?숈삕 ?좎룞?쇿뜝?띕쨪??泥섇뜝?숈삕
		bool singleZero = (position - start == 1) && (source[start] == '0');
		if (singleZero && (peek() == 'x' || peek() == 'X')) {
			advance();
			while (std::isxdigit(static_cast<unsigned char>(peek()))) {
				advance();
			}

			std::string value = source.substr(start, position - start);
			tokens.push_back(Token{ TokenType::HEX_LITERAL, value, line, column });
		}
		else if (singleZero && (peek() == 'b' || peek() == 'B')) {
			advance();
			while (peek() == '0' || peek() == '1') {
				advance();
			}

            std::string value = source.substr(start, position - start);
            tokens.push_back(Token{ TokenType::BINARY_LITERAL, value, line, column });
		}
		else if (singleZero && (peek() == 'o' || peek() == 'O')) {
			advance();
			while (std::isdigit(static_cast<unsigned char>(peek())) && peek() < '8') {
				advance();
			}

            std::string value = source.substr(start, position - start);
            tokens.push_back(Token{ TokenType::OCTAL_LITERAL, value, line, column });
		}
		else if (peek() == 'e' || peek() == 'E') {
            size_t savedPosition = position;
            int savedColumn = column;
			advance();
			if (peek() == '+' || peek() == '-') {
				advance();
			}
            bool hasExponentDigits = false;
			while (std::isdigit(static_cast<unsigned char>(peek()))) {
                hasExponentDigits = true;
				advance();
			}

            if (hasExponentDigits) {
                if (peek() == 'f' || peek() == 'F') {  // `1e3f`: a Float literal
                    advance();
                    tokens.push_back(Token{ TokenType::FLOAT_LITERAL, source.substr(start, position - start), line, column });
                    return true;
                }
                std::string value = source.substr(start, position - start);
			    tokens.push_back(Token{ TokenType::EXPONENTIAL_LITERAL, value, line, column });
            }
            else {
                position = savedPosition;
                column = savedColumn;
                std::string intLiteral = source.substr(start, position - start);
                tokens.push_back(Token{ TokenType::INTEGER_LITERAL, intLiteral, line, column });
            }
		}
        else if (peek() == '.') {
            advance();
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
            if (peek() == 'e' || peek() == 'E') {
                advance();
                if (peek() == '+' || peek() == '-') {
                    advance();
                }
                while (std::isdigit(static_cast<unsigned char>(peek()))) {
                    advance();
                }
                if (peek() == 'f' || peek() == 'F') {  // `1.5e3f`: a Float literal
                    advance();
                    tokens.push_back(Token{ TokenType::FLOAT_LITERAL, source.substr(start, position - start), line, column });
                    return true;
                }
                std::string expLiteral = source.substr(start, position - start);
                tokens.push_back(Token{ TokenType::EXPONENTIAL_LITERAL, expLiteral, line, column });
            }
            else {
                // A plain decimal literal is a Double; the `f` suffix (`1.5f`) makes it a Float.
                if (peek() == 'f' || peek() == 'F') {
                    advance();
                    tokens.push_back(Token{ TokenType::FLOAT_LITERAL, source.substr(start, position - start), line, column });
                }
                else {
                    std::string doubleLiteral = source.substr(start, position - start);
                    tokens.push_back(Token{ TokenType::EXPONENTIAL_LITERAL, doubleLiteral, line, column });
                }
            }
        }
        else {
            std::string intLiteral = source.substr(start, position - start);
            tokens.push_back(Token{ TokenType::INTEGER_LITERAL, intLiteral, line, column });
        }
        return true;
    }
    return false;
}

bool Lexer::matchIdentifierOrKeyword(std::vector<Token>& tokens) {
    unsigned char first = static_cast<unsigned char>(peek());
    bool startsWithAsciiIdentifier = isAsciiIdentifierStart(first);
    bool startsWithUtf8 = first >= 0x80;

    if (!startsWithAsciiIdentifier && !startsWithUtf8) {
        return false;
    }

    size_t start = position;
    if (startsWithAsciiIdentifier) {
        advance();
    }
    else {
        int len = 0;
        if (!consumeUtf8CodePoint(source, position, &len)) {
            return false;
        }
        advance(len);
    }

    while (position < source.length()) {
        unsigned char ch = static_cast<unsigned char>(peek());
        if (isAsciiIdentifierContinue(ch)) {
            advance();
            continue;
        }
        if (ch >= 0x80) {
            int len = 0;
            if (!consumeUtf8CodePoint(source, position, &len)) {
                break;
            }
            advance(len);
            continue;
        }
        break;
    }

    std::string identifier = source.substr(start, position - start);
    if (keywords.find(identifier) != keywords.end()) {
        if (identifier == "true" || identifier == "false") {
            tokens.push_back(Token{ TokenType::BOOLEAN_LITERAL, identifier, line, column });
        }
        else {
            tokens.push_back(Token{ TokenType::KEYWORD, identifier, line, column });
        }
    }
    else {
        tokens.push_back(Token{ TokenType::IDENTIFIER, identifier, line, column });
    }
    return true;
}

bool Lexer::matchOperator(std::vector<Token>& tokens) {
    // ?좎룞?쇿뜝?숈삕 ?좎룞?쇿뜝?숈삕 ?좎룞?쇿뜝?숈삕?좎룞??泥섇뜝?숈삕
    // `&&` and `||` are spellings of the `and` / `or` keywords.
    const std::string pair = source.substr(position, 2);
    if (pair == "&&" || pair == "||") {
        tokens.push_back(Token{ TokenType::KEYWORD, pair == "&&" ? "and" : "or", line, column });
        advance(2);
        return true;
    }
    std::string op = "";
    size_t maxOpLength = 3;  // ?좎뙇?먯삕 ?좎룞?쇿뜝?숈삕?좎룞???좎룞?쇿뜝?숈삕 ?좎룞?쇿뜝?숈삕
    for (size_t len = maxOpLength; len > 0; --len) {
        op = source.substr(position, len);
        if (operators.find(op) != operators.end()) {
            tokens.push_back(Token{ TokenType::OPERATOR, op, line, column });
            advance(static_cast<int>(len));
            return true;
        }
    }
    return false;
}


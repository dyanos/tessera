#include "codegen.h"

#include "ValueNode.h"
#include "ASTVisitor.h"

#include "token.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>

#include <llvm/IR/Constants.h>

void ValueNode::accept(ASTVisitor& visitor) {
	visitor.visit(*this);
}

llvm::Value* ValueNode::codegen() {
	auto& cg = CodeGenerator::getInstance();

	try {
		if (valueType == TokenType::INTEGER_LITERAL) {
			// strtoll never throws (std::stoll does, on a literal beyond 2^63). A literal too large for
			// i64 is only meaningful when the target is Nat, where coerceToNat() reparses this node's
			// exact text via from_decimal and ignores this placeholder.
			errno = 0;
			const long long parsed = std::strtoll(value.c_str(), nullptr, 0);
			const bool requiresLong = errno == ERANGE ||
				parsed < std::numeric_limits<int>::min() ||
				parsed > std::numeric_limits<int>::max();
			return llvm::ConstantInt::get(requiresLong ? llvm::Type::getInt64Ty(cg.context)
				: llvm::Type::getInt32Ty(cg.context), parsed, true);
		}
		else if (valueType == TokenType::FLOAT_LITERAL) {
			return llvm::ConstantFP::get(llvm::Type::getFloatTy(cg.context), std::stof(value));
		}
		else if (valueType == TokenType::EXPONENTIAL_LITERAL) {
			// A decimal literal is a Double unless the type checker narrowed it to Float (`x * 0.5`
			// with a Float `x`, `val f: Float = 0.5`).
			if (type && type->isFloatTy()) {
				return llvm::ConstantFP::get(llvm::Type::getFloatTy(cg.context), std::stof(value));
			}
			return llvm::ConstantFP::get(llvm::Type::getDoubleTy(cg.context), std::stod(value));
		}
		else if (valueType == TokenType::BINARY_LITERAL) {
			return llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), std::stoi(value.substr(2), nullptr, 2));
		}
		else if (valueType == TokenType::HEX_LITERAL) {
			return llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), std::stoi(value.substr(2), nullptr, 16));
		}
		else if (valueType == TokenType::OCTAL_LITERAL) {
			return llvm::ConstantInt::get(llvm::Type::getInt32Ty(cg.context), std::stoi(value.substr(2), nullptr, 8));
		}
		else if (valueType == TokenType::STRING_LITERAL) {
			return cg.builder.CreateGlobalString(value);
		}
        else if (valueType == TokenType::REGEX_LITERAL) {
            return cg.builder.CreateGlobalString(value);
        }
        else if (valueType == TokenType::CHAR_LITERAL) {
            char ch = '\0';
            if (value.size() >= 3) {
                if (value[1] == '\\' && value.size() >= 4) {
                    switch (value[2]) {
                    case 'n': ch = '\n'; break;
                    case 't': ch = '\t'; break;
                    case 'r': ch = '\r'; break;
                    case '0': ch = '\0'; break;
                    case '\\': ch = '\\'; break;
                    case '\'': ch = '\''; break;
                    default: ch = value[2]; break;
                    }
                }
                else {
                    ch = value[1];
                }
            }
            return llvm::ConstantInt::get(llvm::Type::getInt8Ty(cg.context), static_cast<unsigned char>(ch));
        }
		else if (valueType == TokenType::BOOLEAN_LITERAL) {
			if (value == "true") {
				return llvm::ConstantInt::get(llvm::Type::getInt1Ty(cg.context), 1);
			}
			else {
				return llvm::ConstantInt::get(llvm::Type::getInt1Ty(cg.context), 0);
			}
		}
		else {
			std::cerr << "Invalid value type: " << value << std::endl;
			return nullptr;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error: Failed to parse value '" << value << "': " << e.what() << std::endl;
		return nullptr;
	}
}

std::unique_ptr<Type> ValueNode::getType() {
	if (type) return type->clone();

	switch (valueType) {
	case TokenType::INTEGER_LITERAL:
	case TokenType::HEX_LITERAL:
	case TokenType::BINARY_LITERAL:
	case TokenType::OCTAL_LITERAL:
		try {
			const long long parsed = std::stoll(value, nullptr, 0);
			type = std::make_unique<BasicType>(parsed < std::numeric_limits<int>::min() ||
				parsed > std::numeric_limits<int>::max() ? "Long" : "Int");
		}
		catch (const std::exception&) {
			type = std::make_unique<BasicType>(std::string("Int"));
		}
		break;
	case TokenType::FLOAT_LITERAL:
		type = std::make_unique<BasicType>(std::string("Float"));
		break;
	case TokenType::EXPONENTIAL_LITERAL:
		type = std::make_unique<BasicType>(std::string("Double"));
		break;
	case TokenType::BOOLEAN_LITERAL:
		type = std::make_unique<BasicType>(std::string("Boolean"));
		break;
    case TokenType::CHAR_LITERAL:
        type = std::make_unique<BasicType>(std::string("Char"));
        break;
	case TokenType::STRING_LITERAL:
    case TokenType::REGEX_LITERAL:
		type = std::make_unique<BasicType>(std::string("String"));
		break;
	default:
		type = std::make_unique<UnknownType>();
		break;
	}

	return type->clone();
}

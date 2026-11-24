// type.h

#pragma once

#include <string>
#include <vector>
#include <memory>


class Type {
public:
    // VARIABLE????댁꽌??寃???꾩슂
    enum Kind { UNKNOWN=-1, BASIC=0, POINTER, ARRAY, STRUCT, CLASS, FUNCTION, GENERIC, VARIABLE, BOX, BORROW, MUTABLE_BORROW, UNSAFE_POINTER, TUPLE, TENSOR_AXIS };

private:
    Kind kind;

    std::vector<std::unique_ptr<Type>> baseTypes; // ?ㅼ쨷 ?곸냽 吏?먯쓣 ?꾪븳 湲곕컲 ??낅뱾
    // ?? BASIC?대㈃ ?대쫫("int","float" ??
    std::string name;
    // STRUCT/CLASS?대㈃ 硫ㅻ쾭 ?꾨뱶留?
    std::vector<std::pair<std::string, std::unique_ptr<Type>>> fields;
    // FUNCTION?대㈃ 諛섑솚???+ 留ㅺ컻???由ъ뒪??
    std::unique_ptr<Type> returnType;
    std::vector<std::unique_ptr<Type>> paramTypes;
    // 諛곗뿴?대㈃ elementType + size
    std::unique_ptr<Type> elementType;
    int arraySize = 0;
    // 湲고? ?쒕꽕由?씠硫???낆씤??由ъ뒪????
    std::vector<std::unique_ptr<Type>> typeArgs;

public:
	Type() : Type(Kind::UNKNOWN, "unknown") {}
    Type(Kind kind, const std::string& name)
		: kind(kind), name(name) {
	}
    Type(Kind kind, const std::string& name, const std::unique_ptr<Type>& parentType)
        : kind(kind), name(name) {
        if (parentType) {
            this->baseTypes.push_back(parentType->clone());
        }
    }
    Type(Kind kind,
        const std::string& name,
        const std::unique_ptr<Type>& returnType,
        const std::vector<std::unique_ptr<Type>>& paramTypes) {
        this->kind = kind;
        this->name = name;
        this->returnType = returnType ? std::unique_ptr<Type>(returnType->clone()) : nullptr;
        for (const auto& p : paramTypes) {
            this->paramTypes.push_back(p ? p->clone() : nullptr);
        }
    }
    Type(const Type& type)
        : kind(type.kind), name(type.name),
          returnType(type.returnType ? type.returnType->clone() : nullptr),
          elementType(type.elementType ? type.elementType->clone() : nullptr),
          arraySize(type.arraySize) {
        for (const auto& b : type.baseTypes) {
            this->baseTypes.push_back(b ? b->clone() : nullptr);
        }
        for (const auto& f : type.fields) {
            this->fields.push_back({ f.first, f.second ? f.second->clone() : nullptr });
        }
        for (const auto& p : type.paramTypes) {
            this->paramTypes.push_back(p ? p->clone() : nullptr);
        }
        for (const auto& t : type.typeArgs) {
            this->typeArgs.push_back(t ? t->clone() : nullptr);
        }
    }

    virtual ~Type() = default;

    // ?앹꽦?먮뱾 諛??묎렐??
    static std::unique_ptr<Type> makeBasic(const std::string& name);
    static std::unique_ptr<Type> makePointer(std::unique_ptr<Type> base);
    static std::unique_ptr<Type> makeArray(std::unique_ptr<Type> base, int size);
    static std::unique_ptr<Type> makeStruct(const std::string& name,
        const std::vector<std::pair<std::string, std::unique_ptr<Type>>>& fields);
    static std::unique_ptr<Type> makeFunction(const std::unique_ptr<Type>& ret, const std::vector<std::unique_ptr<Type>>& params);

	Kind getKind() const { return kind; }
	std::string getName() const { return name; }
    bool operator==(const Type& other) const {
        return this->kind == other.getKind() && this->name == other.getName();
    }

    bool operator == (const std::unique_ptr<Type>& other) const {
        return other && (*this == *other);
    }

    // 타占쏙옙 占쏟교몌옙 占쏙옙占쏙옙 占쌉쇽옙
    virtual bool equals(const Type& other) const {
        return *this == other;
    }
    virtual bool equals(const std::unique_ptr<Type>& other) const {
        return other && equals(*other);
    }

    bool isTopType() const {
        // ?? Any ??낆씠 理쒖긽????낆씤 寃쎌슦
        return getName() == "Any";
	}

    bool hasSuperClass(const Type& other) const {
        for (auto& base : this->baseTypes) {
            if (*base == other) return true;
            if (base->hasSuperClass(other)) return true;
        }
        return false;
    }
    bool hasSuperClass(const std::unique_ptr<Type>& other) const {
        return other && hasSuperClass(*other);
    }

    bool structuralCheck(const Type& superType) const {
        if (superType.fields.empty()) return false;
        for (const auto& superField : superType.fields) {
            bool found = false;
            for (const auto& myField : this->fields) {
                if (myField.first == superField.first &&
                    myField.second && superField.second &&
                    myField.second->equals(*superField.second)) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    bool structuralCheck(const std::unique_ptr<Type>& superType) const {
        return superType && structuralCheck(*superType);
    }

    bool checkFunctionSubtype(const Type& other) const;
    bool checkFunctionSubtype(const std::unique_ptr<Type>& other) const;

    bool checkGenericSubtype(const Type& other) const {
        return false;
    }
    bool checkGenericSubtype(const std::unique_ptr<Type>& other) const {
        return other && checkGenericSubtype(*other);
    }

    bool checkBasicConversion(const Type& toType) const {
        if (this->isIntegerTy() && toType.isIntegerTy()) return true;
        if ((this->getName() == "Boolean" && toType.getName() == "Bool") ||
            (this->getName() == "Bool" && toType.getName() == "Boolean")) return true;
        if ((this->getName() == "Unit" && toType.getName() == "Void") ||
            (this->getName() == "Void" && toType.getName() == "Unit")) return true;
        if ((this->getName() == "Double" && toType.getName() == "Real") ||
            (this->getName() == "Real" && toType.getName() == "Double")) return true;
        if (this->getName() == "Int" && toType.getName() == "Float") return true;
        if (this->getName() == "Int" && toType.getName() == "Double") return true;
        if (this->getName() == "Float" && toType.getName() == "Double") return true;
        return false;
    }
    bool checkBasicConversion(const std::unique_ptr<Type>& toType) const {
        return toType && checkBasicConversion(*toType);
    }

    // 占쏙옙占쏙옙타占쏙옙 占싯삼옙 占쌉쇽옙
    virtual bool isSubtypeOf(const std::vector<std::unique_ptr<Type>>& others) const {
        for (const auto& other : others) {
            if (isSubtypeOf(other)) return true;
        }
        return false;
	}

    virtual bool isSubtypeOf(const Type& other) const;
    virtual bool isSubtypeOf(const std::unique_ptr<Type>& other) const;

	// int, float, double, char, string, unit?뺤씤吏 泥댄겕?섎뒗 ?⑥닔?낅땲??
	bool isBasicType() const {
		return kind == Kind::BASIC || kind == Kind::GENERIC;
	}

    // class?뺤씤吏 泥댄겕?섎뒗 ?⑥닔?낅땲??
	bool isClassType() const {
		return kind == Kind::CLASS;
	}

	// function?뺤씤吏 泥댄겕?섎뒗 ?⑥닔?낅땲??
	bool isFunctionType() const {
		return kind == Kind::FUNCTION;
	}

    bool isIntegerTy() const {
		return this->isBasicType() && (name == "Int" || name == "Byte" || name == "Short" || name == "Long" || name == "Natural" || name == "Integer");
    }

	bool isCharTy() const {
		return this->isBasicType() && name == "Char";
	}

    bool isFloatTy() const {
        return this->isBasicType() && name == "Float";
    }

	bool isDoubleTy() const {
		return this->isBasicType() && (name == "Double" || name == "Real");
	}

	bool isStringTy() const {
        // string? basic type?멸??
		return this->isBasicType() && name == "String";
	}

	bool isVoidTy() const {
		return this->isBasicType() && (name == "Unit" || name == "Void");
	}

    bool isOwnershipManagedTy() const {
        return kind == Kind::BOX || kind == Kind::BORROW || kind == Kind::MUTABLE_BORROW || kind == Kind::UNSAFE_POINTER;
    }

    virtual std::unique_ptr<Type> clone() = 0;
};

// Primitive and composite type specializations
class BasicType : public Type {
public:
    BasicType() : Type(Kind::BASIC, "basic") {}
    BasicType(const std::string& name)
        : Type(Kind::BASIC, name) {}
    BasicType(const std::string& name, const std::unique_ptr<Type>& parentType)
        : Type(Kind::BASIC, name, parentType) {}

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::BASIC) return false;
        if ((getName() == "Boolean" && other.getName() == "Bool") ||
            (getName() == "Bool" && other.getName() == "Boolean")) return true;
        if ((getName() == "Unit" && other.getName() == "Void") ||
            (getName() == "Void" && other.getName() == "Unit")) return true;
        if ((getName() == "Double" && other.getName() == "Real") ||
            (getName() == "Real" && other.getName() == "Double")) return true;
        return getName() == other.getName();
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<BasicType>(*this);
    }
};

class ClassType : public Type {
public:
    ClassType() : Type(Kind::CLASS, "class") {}
    ClassType(const std::string& name)
        : Type(Kind::CLASS, name) {}
    ClassType(const std::string& name, const std::unique_ptr<Type>& parentType)
        : Type(Kind::CLASS, name, parentType) {}

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::CLASS) return false;
        return getName() == other.getName();
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<ClassType>(*this);
    }
};

class StructType : public Type {
public:
    StructType() : Type(Kind::STRUCT, "struct") {}
    StructType(const std::string& name)
        : Type(Kind::STRUCT, name) {}
    StructType(const std::string& name, const std::unique_ptr<Type>& parentType)
        : Type(Kind::STRUCT, name, parentType) {}

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::STRUCT) return false;
        return getName() == other.getName();
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<StructType>(*this);
    }
};

class FunctionType : public Type {
public:
    std::vector<std::unique_ptr<Type>> parameterTypes;
    std::unique_ptr<Type> returnType;

    FunctionType() : Type(Kind::FUNCTION, "function") {}
    FunctionType(std::string& name) : Type(Kind::FUNCTION, name) {}
    FunctionType(const std::vector<std::unique_ptr<Type>>& parameterTypes, const std::unique_ptr<Type>& returnType)
        : Type(Kind::FUNCTION, "function") {
        this->returnType = returnType ? returnType->clone() : nullptr;
        for (const auto& pt : parameterTypes) {
            this->parameterTypes.push_back(pt->clone());
        }
    }
    FunctionType(const FunctionType& other) : Type(Kind::FUNCTION, "function") {
        this->returnType = other.returnType ? other.returnType->clone() : nullptr;
        for (const auto& pt : other.parameterTypes) {
            this->parameterTypes.push_back(pt->clone());
        }
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::FUNCTION) return false;
        auto otherFuncType = dynamic_cast<const FunctionType*>(&other);
        if (!otherFuncType) return false;
        if (!returnType || !otherFuncType->returnType) return false;
        if (!returnType->equals(*otherFuncType->returnType)) return false;
        if (parameterTypes.size() != otherFuncType->parameterTypes.size()) return false;
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (!parameterTypes[i]->equals(*otherFuncType->parameterTypes[i])) return false;
        }
        return true;
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<FunctionType>(*this);
    }
};

class UnknownType : public Type {
public:
    UnknownType()
        : Type(Kind::UNKNOWN, "unknown") {}

    bool equals(const Type& other) const override {
        return false;
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<UnknownType>(*this);
    }
};

class GenericType : public Type {
public:
    std::unique_ptr<Type> baseType;
    std::vector<std::unique_ptr<Type>> typeArguments;

    GenericType() : Type(Kind::GENERIC, "generic") {}
    GenericType(const std::unique_ptr<Type>& baseType, const std::vector<std::unique_ptr<Type>>& typeArguments)
        : Type(Kind::GENERIC, baseType->getName()) {
        this->baseType = baseType ? baseType->clone() : nullptr;
        for (const auto& pt : typeArguments) {
            this->typeArguments.push_back(pt->clone());
        }
    }
    GenericType(const GenericType& other)
        : Type(Kind::GENERIC, other.baseType ? other.baseType->getName() : "generic") {
        this->baseType = other.baseType ? other.baseType->clone() : nullptr;
        for (const auto& arg : other.typeArguments) {
            this->typeArguments.push_back(arg->clone());
        }
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::GENERIC) return false;
        auto otherGeneric = dynamic_cast<const GenericType*>(&other);
        if (!otherGeneric) return false;
        if (!baseType || !otherGeneric->baseType) return false;
        if (!baseType->equals(*otherGeneric->baseType)) {
            // Vector and Array are interchangeable spellings of the flat array type, so a
            // Vector[T] and an Array[T] are the same type for equality and overload resolution.
            const std::string a = baseType->getName();
            const std::string b = otherGeneric->baseType->getName();
            const bool bothFlat = (a == "Array" || a == "Vector") && (b == "Array" || b == "Vector");
            if (!bothFlat) return false;
        }
        if (typeArguments.size() != otherGeneric->typeArguments.size()) return false;
        for (size_t i = 0; i < typeArguments.size(); ++i) {
            if (!typeArguments[i]->equals(*otherGeneric->typeArguments[i])) return false;
        }
        return true;
    }

    bool isSubtypeOf(const Type& other) const override {
        if (equals(other)) return true;
        return false;
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<GenericType>(*this);
    }
};

class ArrayType : public Type {
public:
    std::unique_ptr<Type> elementType;
    int size;

    ArrayType() : Type(Kind::ARRAY, "array"), size(0) {}
    ArrayType(const std::unique_ptr<Type>& elementType, int size)
        : Type(Kind::ARRAY, "array") {
        this->elementType = elementType->clone();
        this->size = size;
    }
    ArrayType(const ArrayType& other)
        : Type(Kind::ARRAY, "array") {
        this->elementType = other.elementType ? other.elementType->clone() : nullptr;
        this->size = other.size;
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::ARRAY) return false;
        auto otherArray = dynamic_cast<const ArrayType*>(&other);
        if (!otherArray) return false;
        return elementType->equals(*otherArray->elementType) && size == otherArray->size;
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<ArrayType>(*this);
    }
};

class PointerType : public Type {
public:
    std::unique_ptr<Type> baseType;

    PointerType() : Type(Kind::POINTER, "pointer") {}
    PointerType(const std::unique_ptr<Type>& baseType)
        : Type(Kind::POINTER, "pointer") {
        this->baseType = baseType->clone();
    }
    PointerType(const PointerType& other)
        : Type(Kind::POINTER, "pointer") {
        this->baseType = other.baseType ? other.baseType->clone() : nullptr;
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::POINTER) return false;
        auto otherPtr = dynamic_cast<const PointerType*>(&other);
        if (!otherPtr) return false;
        return baseType->equals(*otherPtr->baseType);
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<PointerType>(*this);
    }
};

class BoxType : public Type {
public:
    std::unique_ptr<Type> baseType;

    BoxType() : Type(Kind::BOX, "box") {}
    BoxType(const std::unique_ptr<Type>& baseType)
        : Type(Kind::BOX, "box") {
        this->baseType = baseType ? baseType->clone() : nullptr;
    }
    BoxType(const BoxType& other)
        : Type(Kind::BOX, "box") {
        this->baseType = other.baseType ? other.baseType->clone() : nullptr;
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::BOX) return false;
        auto otherBox = dynamic_cast<const BoxType*>(&other);
        if (!otherBox || !baseType || !otherBox->baseType) return false;
        return baseType->equals(*otherBox->baseType);
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<BoxType>(*this);
    }
};

class BorrowType : public Type {
public:
    std::unique_ptr<Type> baseType;
    bool isMutableBorrow = false;

    BorrowType() : Type(Kind::BORROW, "borrow") {}
    BorrowType(const std::unique_ptr<Type>& baseType, bool isMutableBorrow)
        : Type(isMutableBorrow ? Kind::MUTABLE_BORROW : Kind::BORROW, isMutableBorrow ? "&mut" : "&"),
          isMutableBorrow(isMutableBorrow) {
        this->baseType = baseType ? baseType->clone() : nullptr;
    }
    BorrowType(const BorrowType& other)
        : Type(other.isMutableBorrow ? Kind::MUTABLE_BORROW : Kind::BORROW, other.isMutableBorrow ? "&mut" : "&"),
          isMutableBorrow(other.isMutableBorrow) {
        this->baseType = other.baseType ? other.baseType->clone() : nullptr;
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != getKind()) return false;
        auto otherBorrow = dynamic_cast<const BorrowType*>(&other);
        if (!otherBorrow || !baseType || !otherBorrow->baseType) return false;
        return baseType->equals(*otherBorrow->baseType);
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<BorrowType>(*this);
    }
};

// An anonymous ordered product type `(T1, …, Tn)` used for multiple return values. Ownership of
// each owned element transfers with the tuple; destructuring moves each element out into a binding.
class TupleType : public Type {
public:
    std::vector<std::unique_ptr<Type>> elementTypes;

    TupleType() : Type(Kind::TUPLE, "tuple") {}
    explicit TupleType(std::vector<std::unique_ptr<Type>> elements)
        : Type(Kind::TUPLE, "tuple") {
        elementTypes = std::move(elements);
    }
    TupleType(const TupleType& other) : Type(Kind::TUPLE, "tuple") {
        for (const auto& e : other.elementTypes) {
            elementTypes.push_back(e ? e->clone() : nullptr);
        }
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::TUPLE) return false;
        auto* o = dynamic_cast<const TupleType*>(&other);
        if (!o || o->elementTypes.size() != elementTypes.size()) return false;
        for (size_t i = 0; i < elementTypes.size(); ++i) {
            if (!elementTypes[i] || !o->elementTypes[i] || !elementTypes[i]->equals(*o->elementTypes[i])) {
                return false;
            }
        }
        return true;
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<TupleType>(*this);
    }
};

class UnsafePointerType : public Type {
public:
    std::unique_ptr<Type> baseType;

    UnsafePointerType() : Type(Kind::UNSAFE_POINTER, "unsafe pointer") {}
    UnsafePointerType(const std::unique_ptr<Type>& baseType)
        : Type(Kind::UNSAFE_POINTER, "unsafe pointer") {
        this->baseType = baseType ? baseType->clone() : nullptr;
    }
    UnsafePointerType(const UnsafePointerType& other)
        : Type(Kind::UNSAFE_POINTER, "unsafe pointer") {
        this->baseType = other.baseType ? other.baseType->clone() : nullptr;
    }

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::UNSAFE_POINTER) return false;
        auto otherPtr = dynamic_cast<const UnsafePointerType*>(&other);
        if (!otherPtr || !baseType || !otherPtr->baseType) return false;
        return baseType->equals(*otherPtr->baseType);
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<UnsafePointerType>(*this);
    }
};

class TypeVariableType : public Type {
public:
    TypeVariableType() : Type(Kind::VARIABLE, "T") {}
    TypeVariableType(const std::string& name) : Type(Kind::VARIABLE, name) {}

    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::VARIABLE) return false;
        return getName() == other.getName();
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<TypeVariableType>(getName());
    }
};

class TensorAxisType final : public Type {
public:
    enum class Form { Symbol, RuntimeReference, RuntimeValue };

    const Form form;

    TensorAxisType(Form form, const std::string& name) : Type(Kind::TENSOR_AXIS, name), form(form) {}

    bool equals(const Type& other) const override {
        const auto* axis = dynamic_cast<const TensorAxisType*>(&other);
        return axis && axis->form == form && axis->getName() == getName();
    }

    bool isSubtypeOf(const Type& other) const override {
        return equals(other);
    }

    std::unique_ptr<Type> clone() override {
        return std::make_unique<TensorAxisType>(*this);
    }
};


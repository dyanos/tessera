// TensorScript grammar sketch.
//
// This PEG file documents the current hand-written C++ parser. It is not used
// to generate parser.cpp; keep parser.cpp, lexer.cpp, and this file in sync.

Start
  = _ Program _

Program
  = TopStatement*

TopStatement
  = EnumDeclaration
  / InterfaceDeclaration
  / Attribute? TemplateDeclaration
  / Attribute? Modifier* ImportDeclaration
  / Attribute? Modifier* ClassDeclaration
  / Attribute? Modifier* ObjectDeclaration
  / Attribute? Modifier* FunctionDeclaration
  / Statement

Modifier
  = "constexpr" __
  / "external" __
  / "unsafe" __

Attribute
  = "[@" _ AttributeExpression _ "]" _

AttributeExpression
  = Identifier CallArguments?
  / StringLiteral
  / IntegerLiteral

ImportDeclaration
  = "import" __ QualifiedName (_ "=" _ Identifier)? StatementEnd?

TemplateDeclaration
  = "template" _ "<" _ TemplateParameter (_ "," _ TemplateParameter)* _ ">" _
    ("def" __ FunctionRest / "class" __ ClassRest / "struct" __ ClassRest)

TemplateParameter
  = ("typename" / "class") __ Identifier (_ ":" _ Identifier (_ "+" _ Identifier)*)?   // interface bounds
  / IntegralType __ Identifier

EnumDeclaration
  = "enum" __ Identifier _ ("<" _ Identifier (_ "," _ Identifier)* _ ">" _)? Implements?
    "{" _ EnumVariant (_ ("," / ";") _ EnumVariant)* _ (","/";")? _ FunctionDeclaration* _ "}"

EnumVariant
  = Identifier _ ParameterList?

InterfaceDeclaration
  = "interface" __ Identifier _ "{" _ (InterfaceMethod _)* "}"   // `interface` is contextual

InterfaceMethod
  = "def" __ MemberName _ "(" _ Parameters? _ ")" _ (":" _ Type)? _ (FunctionBody / StatementEnd)?

Implements
  = "implements" __ Identifier (_ "," _ Identifier)* _

ClassDeclaration
  = "class" __ ClassRest

ClassRest
  = Identifier _ ParameterList? _ ("extends" __ Identifier _)? Implements? ClassBody?

ClassBody
  = "{" _ (FunctionDeclaration / VariableDeclaration StatementEnd?)* _ "}"

ObjectDeclaration
  = "object" __ Identifier _ ("{" _ Statement* _ "}" / "=" _ ("{" _ Statement* _ "}" / Expression))

FunctionDeclaration
  = "def" __ FunctionRest

FunctionRest
  = MemberName _ "(" _ Parameters? _ ")" _ (":" _ Type)? _ ((ContractClause / FunctionSpecBlock) _)*
    ("=" _ Expression / "{" _ Statement* _ "}")?

ContractClause
  = ("requires" / "ensures" / "decreases") __ (Identifier _ ":" _)? Expression

FunctionSpecBlock
  = "[%" _ ((ContractClause / "def" __ FunctionRest) _ (";" _)?)* "%]"

SpecificationDeclarations
  = "[%" _ (("def" / "lemma") __ FunctionRest _ (";" _)?)* "%]"

GhostBlock
  = "[%" _ Statement* _ "%]"

LoopSpec
  = ("invariant" / "decreases") __ (Identifier _ ":" _)? Expression
  / "[%" _ (("invariant" / "decreases") __ (Identifier _ ":" _)? Expression _ (";" _)?)* "%]"

Parameters
  = Parameter (_ "," _ Parameter)*

Parameter
  = Identifier _ ":" _ Type (_ "=" _ Expression)?   // default value

VariableDeclaration
  = ("val" / "var") __ (Identifier / "$" _ LatexCommand _ "$") _ (":" _ Type)? _ "=" _ Expression

Statement
  = "constexpr" __ Statement
  / "unsafe" _ ("{" _ Statement* _ "}" / FunctionDeclaration)
  / "unatomic" _ "{" _ Statement* _ "}"
  / VariableDeclaration StatementEnd?
  / IfStatement
  / ForStatement
  / WhileStatement
  / FunctionDeclaration
  / ReturnStatement
  / LoopControl
  / MapStatement
  / PMapStatement
  / ReduceStatement
  / PReduceStatement
  / FilterStatement
  / ObjectDeclaration
  / Expression StatementEnd?

IfStatement
  = "if" _ "constexpr"? _ "(" _ Expression _ ")" _ Block (_ "else" _ (Block / IfStatement))?

ForStatement
  = "for" _ "(" _ Identifier _ "<-" _ Expression (_ ("to" / "until") __ Expression)? _ ")" _ (LoopSpec _)* Block

WhileStatement
  = "while" _ "(" _ Expression _ ")" _ (LoopSpec _)* Block

MapStatement
  = "map" _ "(" _ Identifier _ "<-" _ Expression _ ")" _ Block

PMapStatement
  = "pmap" _ "(" _ (PMapBackend _ "," _)? Identifier _ "<-" _ Expression _ ")" _ Block

PMapBackend
  = "cpu" / "openmp" / "gpu" / "simd"

ReduceStatement
  = "reduce" _ "(" _ Identifier _ "<-" _ Expression _ "," _ Expression _ ")" _ Block

PReduceStatement
  = "preduce" _ "(" _ PMapBackend _ "," _ Identifier _ "," _ Identifier _ "<-" _ Expression _ "," _ Expression _ ")" _ Block

FilterStatement
  = "filter" _ "(" _ Identifier _ "<-" _ Expression _ ")" _ Block

ReturnStatement
  = "return" (__ Expression)? StatementEnd?

LoopControl
  = ("break" / "continue") ![A-Za-z0-9_] StatementEnd?

Block
  = "{" _ Statement* _ "}"

Expression
  = MatchExpression
  / AssignmentExpression

MatchExpression
  = PipelineExpression (_ "match" _ "{" _ MatchCase* _ "}")?

MatchCase
  = "case" __ Pattern (_ "|" _ Pattern)* (_ "if" __ Expression)? _ "=>" _ Expression

Pattern
  = "_"
  / "-"? Literal
  / (Identifier _ "." _)? Identifier (_ "(" _ Pattern (_ "," _ Pattern)* _ ")")?

AssignmentExpression
  = PipelineExpression (_ ("=" / "<-" / "+=" / "-=" / "*=" / "/=" / "%=") _ Expression)?

PipelineExpression
  = OrExpression (_ (">>|" / "<<|") _ OrExpression)*

OrExpression
  = AndExpression (_ "or" __ AndExpression)*

AndExpression
  = BitwiseOrExpression (_ "and" __ BitwiseOrExpression)*

BitwiseOrExpression
  = XorExpression (_ "|" _ XorExpression)*

XorExpression
  = BitwiseAndExpression (_ ("xor" / "^") _ BitwiseAndExpression)*

BitwiseAndExpression
  = EqualityExpression (_ "&" _ EqualityExpression)*

EqualityExpression
  = ComparisonExpression (_ ("==" / "!=") _ ComparisonExpression)*

ComparisonExpression
  = ShiftExpression (_ ("<=" / ">=" / "<" / ">") _ ShiftExpression)*

ShiftExpression
  = AdditiveExpression (_ ("<<" / ">>") _ AdditiveExpression)*

AdditiveExpression
  = MultiplicativeExpression (_ ("+" / "-") _ MultiplicativeExpression)*

MultiplicativeExpression
  = PrefixExpression (_ ("*" / "/" / "%" / "@" / "inner" / "outer" / "tensor") _ PrefixExpression)*

PrefixExpression
  = ("++" / "--" / "<-" / "&mut" / "&" / "+" / "-" / "!" / "~" / "*") _ PrefixExpression
  / "await" ![A-Za-z0-9_] _ PrefixExpression
  / ("async" / "go") ![A-Za-z0-9_] _ PostfixExpression   // operand must be a call f(...)
  / PostfixExpression

PostfixExpression
  = PrimaryExpression (_ ("++" / "--" / "?" / IndexSuffix))*   // `e?` unwraps an Option/Result

IndexSuffix
  = "[" _ Expression (_ "," _ Expression)* _ "]"

PrimaryExpression
  = IfStatement
  / MapStatement
  / PMapStatement
  / ReduceStatement
  / PReduceStatement
  / FilterStatement
  / LambdaExpression
  / NewExpression
  / ArrayLiteral
  / DictLiteral
  / SetLiteral
  / Block
  / "(" _ Expression (_ "," _ Expression)+ _ ")"
  / "(" _ Expression _ ")"
  / QualifiedName TemplateArguments? CallArguments?
  / Literal
  / InlineMath
  / BlockMath
  / "_"

LambdaExpression
  = "[" _ ("&" / "=" / IdentifierList?) _ "]" _ "(" _ LambdaParameters? _ ")" _ "->" _ Block

LambdaParameters
  = LambdaParameter (_ "," _ LambdaParameter)*

LambdaParameter
  = Identifier (_ ":" _ Type)?

NewExpression
  = "new" __ Identifier TemplateArguments? (_ "[" _ Expression (_ "," _ Expression)* _ "]" / CallArguments)?

ArrayLiteral
  = "[" _ (Expression (_ "," _ Expression)*)? _ "]"

DictLiteral
  = "{" _ DictEntry (_ "," _ DictEntry)* (_ ",")? _ "}"
  / "{" _ "}"

DictEntry
  = Expression _ ":" _ Expression

SetLiteral
  = "{" _ Expression (_ "," _ Expression)* _ "," _ "}"
  / "{" _ Expression (_ "," _ Expression)+ _ "}"

CallArguments
  = "(" _ (CallArgument (_ "," _ CallArgument)*)? _ ")"

CallArgument
  = Identifier _ "=" !"=" _ Expression   // named argument
  / Expression

TemplateArguments
  = "<" _ TemplateArgument (_ "," _ TemplateArgument)* _ ">"

TemplateArgument
  = Type
  / Literal

Type
  = "box" __ Type
  / "&" _ "mut"? _ Type
  / "unsafe" _ "*" _ Type
  / FunctionType
  / Identifier TemplateArguments?

FunctionType
  = "(" _ (Type (_ "," _ Type)*)? _ ")" _ "=>" _ Type

Literal
  = BooleanLiteral
  / CharLiteral
  / RegexLiteral
  / StringLiteral
  / HexLiteral
  / BinaryLiteral
  / OctalLiteral
  / FloatLiteral
  / IntegerLiteral

InlineMath
  = "$" (!"$" .)* "$"

BlockMath
  = "$$" (!"$$" .)* "$$"

IdentifierList
  = Identifier (_ "," _ Identifier)*

QualifiedName
  = Identifier (_ "." _ MemberName)*

// After '.' (and as a method name) keywords are ordinary names: frame.filter(...), xs.map(...).
MemberName
  = [A-Za-z_\u0080-\uFFFF] [A-Za-z0-9_\u0080-\uFFFF]*

Identifier
  = !ReservedWord [A-Za-z_\u0080-\uFFFF] [A-Za-z0-9_\u0080-\uFFFF]*
  / "box"

ReservedWord
  = ("abstract" / "case" / "catch" / "class" / "def" / "do" / "else" /
     "extends" / "external" / "false" / "final" / "finally" / "for" /
     "if" / "import" / "match" / "mut" / "new" / "null" / "object" /
     "return" / "super" / "template" / "this" / "true" / "typename" /
     "unsafe" / "unatomic" / "val" / "var" / "while" / "preduce" /
     "async" / "await" / "go" / "break" / "continue" / "enum") ![A-Za-z0-9_]

IntegralType
  = "Int" / "Long" / "Short" / "Byte" / "Boolean" / "Bool"

BooleanLiteral
  = "true" / "false"

StringLiteral
  = [uU]? "\"" ("\\" . / !"\"" .)* "\""

RegexLiteral
  = [rR] "\"" (!"\"" .)* "\""

CharLiteral
  = "'" ("\\" . / !"'" .) "'"

HexLiteral
  = "0" [xX] [0-9a-fA-F]+

BinaryLiteral
  = "0" [bB] [01]+

OctalLiteral
  = "0" [oO] [0-7]+

FloatLiteral
  = [0-9]+ "." [0-9]* ([eE] [+-]? [0-9]+)?
  / [0-9]+ [eE] [+-]? [0-9]+

IntegerLiteral
  = [0-9]+

StatementEnd
  = _ ";" _

_
  = ([ \t\r\n] / Comment)*

__
  = ([ \t\r\n] / Comment)+

Comment
  = "//" (![\r\n] .)*
  / "/*" (!"*/" .)* "*/"

LatexCommand
  = "\\" [A-Za-z]+ ("{" [A-Za-z]+ "}")? / Identifier

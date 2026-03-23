# tinycompiler

A compiler frontend for a small JavaScript-like language "tinyjs". It includes a lexer,
a recursive descent parser, semantic analysis, and an AST built
with LLVM-style RTTI.

---

## Language Specification

The language is a simplified subset of JavaScript. It supports functions, integer
and boolean literals, basic arithmetic, comparisons, variables, if/else,
while loops, return statements, and function calls.

### Grammar (EBNF)

EBNF notation used here:
- `{ x }` = zero or more repetitions of x
- `[ x ]` = x is optional
- `( x | y )` = x or y
- Quoted strings are terminals. ALL_CAPS names are token categories.

```
program         = { function_decl } EOF .

function_decl   = "function" IDENTIFIER "(" [ param_list ] ")" block .

param_list      = IDENTIFIER { "," IDENTIFIER } .

block           = "{" { statement } "}" .

statement       = var_decl
                | assign_or_call
                | if_stmt
                | while_stmt
                | return_stmt .

var_decl        = "var" IDENTIFIER [ "=" expr ] ";" .

assign_or_call  = IDENTIFIER ( "=" expr | "(" [ arg_list ] ")" ) ";" .

if_stmt         = "if" "(" expr ")" block [ "else" block ] .

while_stmt      = "while" "(" expr ")" block .

return_stmt     = "return" [ expr ] ";" .

arg_list        = expr { "," expr } .

expr            = comparison .

comparison      = term { ( "==" | "!=" | "<" | ">" ) term } .

term            = factor { ( "+" | "-" ) factor } .

factor          = primary { ( "*" | "/" | "%" ) primary } .

primary         = INTEGER_LITERAL
                | "true"
                | "false"
                | "(" expr ")"
                | IDENTIFIER [ "(" [ arg_list ] ")" ] .
```

### Tokens

**Keywords**

| Token         | Spelling   |
|---------------|------------|
| `kw_var`      | `var`      |
| `kw_function` | `function` |
| `kw_if`       | `if`       |
| `kw_else`     | `else`     |
| `kw_while`    | `while`    |
| `kw_return`   | `return`   |
| `kw_true`     | `true`     |
| `kw_false`    | `false`    |

**Punctuators**

| Token        | Spelling |
|--------------|----------|
| `plus`       | `+`      |
| `minus`      | `-`      |
| `star`       | `*`      |
| `slash`      | `/`      |
| `percent`    | `%`      |
| `equalequal` | `==`     |
| `notequal`   | `!=`     |
| `less`       | `<`      |
| `greater`    | `>`      |
| `equal`      | `=`      |
| `lparen`     | `(`      |
| `rparen`     | `)`      |
| `lbrace`     | `{`      |
| `rbrace`     | `}`      |
| `semi`       | `;`      |
| `comma`      | `,`      |

**Literals**

```
INTEGER_LITERAL = DIGIT { DIGIT } .
IDENTIFIER      = ( LETTER | "_" ) { LETTER | DIGIT | "_" } .
DIGIT           = "0" | "1" | ... | "9" .
LETTER          = "a" | ... | "z" | "A" | ... | "Z" .
```

### LL(1) Compatibility

The grammar is mostly LL(1), meaning the parser only needs one token of lookahead to
make decisions:

- Each statement type starts with a unique keyword (`var`, `if`, `while`,
  `return`) or an identifier, so the parser always knows which rule to apply.
- The one exception is `assign_or_call`: after consuming the identifier, the
  parser looks at the next token (`=` for assignment, `(` for a call). This
  requires two tokens of lookahead from the start of the construct, but is
  handled naturally by the recursive descent parser.
- The expression rules (`comparison`, `term`, `factor`) use repetition (`{ }`)
  instead of left recursion, so there is no left recursion anywhere in the grammar.

## Usage

Build the compiler:

```sh
cmake --build cmake-build-debug
```

Run it on a source file. Each run produces two output files: an AST dump and LLVM IR.

```sh
./cmake-build-debug/tinycompiler test1.js
# AST written to test1-output.ast
# IR written to test1.ll

./cmake-build-debug/tinycompiler test2.js
# AST written to test2-output.ast
# IR written to test2.ll
```
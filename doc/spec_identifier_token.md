# Header File `lex/identifier_token.hpp`

The file `identifier_token.hpp` provides the keyword and identifier token.

```cpp
namespace lex
{
    // token specification
    template <class Derived, class TokenSpec>
    struct identifier_token;
    
    template <char ... Char> 
    struct keyword_token;
    
    #define FOONATHAN_LEX_KEYWORD(String)
    
    // traits
    template <class Token>
    struct is_identifier_token;
    
    template <class Token>
    struct is_non_identifier_rule_token;
    
    template <class Token>
    struct is_keyword_token;
    
    template <class Token>
    struct is_non_keyword_literal_token;
}
```

## Token Specification

```cpp
template <class Derived, class TokenSpec>
struct identifier_token
: lex::rule_token<Derived, TokenSpec>
{
    static constexpr const char* name = "<identifier">; 
};
```

A class derived from `lex::identifier_token` is a special [rule token](spec_rule_token.md#token-specification), an identifier token.
It behaves just like a regular rule token derived from `lex::rule_token`, except for the keyword interaction described below.
A token specification must only contain one identifier token.

```cpp
template <char ... Literal> 
struct keyword_token
: lex::literal_token<Literal...>
{};
```

A class derived from `lex::keyword_token` is a special [literal token](spec_literal_token.md#token-specification), a keyword token.
It behaves just like a regular literal token, except for the keyword interaction described below.
If a token specification contains a keyword token, it must also contain the identifier token.

The tokenizer does not try and match keyword tokens on the input, only the identifier token is matched like a regular rule token.
If the identifier token matched, all keywords token try to match its spelling.
If any keyword matched, it will be returned instead, otherwise it is a normal identifier.

```cpp
#define FOONATHAN_LEX_KEYWORD(String)
```

The macro `FOONATHAN_LEX_KEYWORD(String)` is equivalent to `lex::keyword_token<String[0], String[1], …>`.
All null characters are ignored.

## Traits

The traits all derive from either `std::true_type` or `std::false_type`,
depending on the result of the condition.

* `is_identifier_token<Token>`: whether or not `Token` is an identifier token.
* `is_non_identifier_rule_token<Token>`: whether or not `Token` is a rule token but not an identifier.
* `is_keyword_token<Token>`: whether or not `Token` is a keyword token.
* `is_non_keyword_literal_token<Token>`: whether or not `Token` is a literal token but not a keyword.

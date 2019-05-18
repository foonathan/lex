# Header File `lex/literal_token.hpp`

The file `literal_token.hpp` provides the literal token.

```cpp
namespace lex
{
    // token specification
    template <char ... Literal>
    struct literal_token;
    
    #define FOONATHAN_LEX_LITERAL(String)
   
    // traits 
    template <class Token> 
    struct is_literal_token;
}
```

## Token Specification

```cpp
template <char ... Literal>
struct literal_token
{
    static constexpr char name[] = { Literal..., '\0' };
};
```

A class derived from `lex::literal_token` is a literal token.

It creates a token that matches the specified character sequence.
If there are multiple literal tokens sharing a common prefix, the longest literal token is selected.
If a [rule token](spec_rule_token.md#token-specification) and a literal token would both match at the current input,
the literal token will be created unless the rule token has specified it as conflicting.

```cpp
#define FOONATHAN_LEX_LITERAL(String)
```

The macro `FOONATHAN_LEX_LITERAL(String)` is equivalent to `lex::literal_token<String[0], String[1], …>`.
All null characters are ignored.

## Traits

The traits all derive from either `std::true_type` or `std::false_type`,
depending on the result of the condition.

`is_literal_token<Token>`: whether or not `Token` is a literal token.


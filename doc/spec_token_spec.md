# Header File `lex/token_spec.hpp`

The file `token_spec.hpp` contains the token specification and base tokens.

```cpp
namespace lex
{
    // token specification
    template <class ... Tokens>
    struct token_spec {};
     
    struct error_token {}; 
    struct eof_token {};
    
    // traits
    template <typename T>
    struct is_token;
}
```

## Token Specification

The tokens in a grammar are specified by creating a class that inherits from `lex::token_spec` passing it all the token types.

More details can be found in the [tutorial]().

The special token types `lex::error_token`, representing an invalid character (sequence), and `lex::eof_token`, representing the end of the input, is always included.

## Traits

The traits all derive from either `std::true_type` or `std::false_type`,
depending on the result of the condition.

`is_token<T>`: whether or not `T` is a token type.

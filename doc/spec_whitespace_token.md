# Header File `lex/whitespace_token.hpp`

The file `whitespace_token.hpp` defines the whitespace token.

```cpp
namespace lex
{
    struct whitespace_token {};
    
    // traits
    template <class Token>
    struct is_whitespace_token;
}
```

A token that inherits from `lex::whitespace_token` in addition to one of the other base classes is a whitespace token.
When tokenizing, whitespace tokens will be automatically skipped and not created.

## Traits

The traits all derive from either `std::true_type` or `std::false_type`,
depending on the result of the condition.

`is_whitespace_token<Token>`: whether or not `Token` is a whitespace token.

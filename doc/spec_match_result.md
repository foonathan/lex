# Header File `lex/match_result.hpp`

The file `match_result.hpp` defines the class `lex::match_result`, which is the result of a token match operation.

```cpp
template <class TokenSpec>
struct match_result
{
    token_kind<TokenSpec> kind;
    std::size_t           bump;
    
    static match_result unmatched();
    static match_result error(std::size_t bump);
    static match_result success(token_kind<TokenSpec> kind, std::size_t bump);
    static match_result eof();
    
    bool is_unmatched() const; 
    bool is_error() const;
    bool is_success() const;
    bool is_eof() const;
    bool is_matched() const;
};
```

All member functions are `constexpr` and `noexcept`.

Creation of a `match_result` is only possible using the named functions which put it in one of four states:

1. `unmatched()`: no token was matched.
   Then `kind` is be `lex::error_token` and `bump` is `0`.
   `is_unmatched()` returns `true`, all others return `false`.
   
2. `error()`: the input is an error.

   Then `kind` is be `lex::error_token` and `bump` is the number of characters to skip, which must be `> 0`.
   
   `is_error()` and `is_matched()` return `true`, all others return `false`.
   
3. `success()`: the input matched a token.

   Then `kind` and `bump` are as specified; `bump` must be greater than `0` and `kind` not `lex::error_token` or `lex::eof_token`.
   
   `is_success()` and `is_matched()` return `true`, all others return `false`.
   
4. `eof()`: the end of the input was reached.

   Then `kind` is `lex::eof_token` and `bump` is `0`.
   
   `is_eof()` and `is_matched()` return `true`, all others return `false`.


# Header File `lex/token.hpp`

The file `token.hpp` contains the class `lex::token` and `lex::static_token`.

```cpp
namespace lex
{
    template <class TokenSpec>
    class token;
    
    template <class Token, class Payload = void>
    class static_token;
}
```

## Token

```cpp
template <class TokenSpec>
class token
{
    token_kind<TokenSpec> kind;
    token_spelling        spelling;

public:
    // creation
    token();
    
    // kind 
    token_kind<TokenSpec> kind(); 
    
    explicit operator bool() const;
    
    template <class Token>
    bool is(Token = {}) const;
    
    template <template <typename> class Category>
    bool is_category() const;
    
    const char* name() const;
    
    // spelling
    token_spelling spelling() const;
    
    std::size_t offset(const tokenizer<TokenSpec>& tokenizer) const;
};
```

The class `lex::token` is the value of a token in the input.
It is a lightweight pair of kind and spelling, i.e. view in the input.

All functions are `constexpr` and `noexcept`.

### Creation

```cpp
token();
```

The default constructor creates an invalid, partially-formed token that may not be used.

Valid tokens can only be created by the tokenizer.

### Kind

```cpp
token_kind<TokenSpec> kind(); 
```

Returns what kind of token it is, i.e. the runtime identifier of the token type.

The other functions just forward to the corresponding [`lex::token_kind`](spec_token_kind.md) member function.

### Spelling

```cpp
token_spelling spelling() const;
```

Returns the spelling of the token, i.e. the view into the input.

```cpp
std::size_t offset(const tokenizer<TokenSpec>& tokenizer) const;
```

Returns the offset of the token in the input, when passed the [`lex::tokenizer`](spec_tokenizer.md) that was used to create it.

The offset of a token is simply the difference between the spelling pointer and the start pointer of the input.
Line and column information can be obtained by iterating over the input and keeping track of newlines.

## Static Token

```cpp
template <class Token, class Payload = void>
class static_token
{
    token_spelling spelling;
    Payload        payload;

public:
    // creation
    template <class TokenSpec, class Payload>
    explicit static_token(const token<TokenSpec>& token, Payload payload);
    
    // static information
    operator Token() const;
    
    Payload& value() &;
    const Payload& value() const&;
    Payload&& value() &&;
    const Payload&& value() const&&;
    
    // kind 
    template <class TokenSpec>
    token_kind<TokenSpec> kind(); 
    
    explicit operator bool() const;
    
    template <class Token>
    bool is(Token = {}) const;
    
    template <template <typename> class Category>
    bool is_category() const;
    
    const char* name() const;
    
    // spelling
    token_spelling spelling() const;
    
    template <class TokenSpec>
    std::size_t offset(const tokenizer<TokenSpec>& tokenizer) const;
};
```

The class `lex::static_token` is a `lex::token` where the [`lex::token_kind`](spec_token_kind.md) is statically known.
It can optionally have an additional `Payload`, e.g. the integer value of an integer token.
If the `Payload` is `void` (the default), all `Payload` functions are not available.

A `static_token<Token, Payload>` is convertible to `static_token<Token, void>` by discarding the payload.

All member functions are `constexpr` and `noexcept`, except the `Payload` constructor which is not `noexcept`.

> This class is used with the parsing interface.

### Creation

```cpp
template <class TokenSpec, class Payload>
explicit static_token(const token<TokenSpec>& token, Payload payload);
```

Creates it from a regular `lex::token`, whose kind must refer the specified type, and the payload.

If the `Payload` is `void`, there is no `Payload` argument.

### Static Information

```cpp
operator Token() const;
```

Returns `Token{}`.

> This is used for a production callback that does not care about the spelling of the token, only the kind.
    
```cpp
Payload& value() &;
const Payload& value() const&;
Payload&& value() &&;
const Payload&& value() const&&;
```

Returns a reference to the payload.

If `Payload` is `void`, they are not available.

### Kind

```cpp
token_kind<TokenSpec> kind(); 
```

Returns `token_kind<TokenSpec>::of<Token>()`, i.e. the runtime identifier of the token type.
The `TokenSpec` has to be given as it is not known from the token type alone.

The other functions return the same as the corresponding [`lex::token_kind`](spec_token_kind.md) member function for the specified token type.

### Spelling

The spelling functions are the same as for `lex::token`.

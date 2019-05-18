# Header File `lex/token_kind.hpp`

The file `token_kind.hpp` contains the class `lex::token_kind`.

```cpp
template <class TokenSpec>
class token_kind
{
    Integer id;

public:
    // creation
    static token_kind from_id(std::size_t id);
    
    template <class Token>
    static token_kind of();
    
    token_kind();
    template <class Token>
    token_kind(Token token);
    
    // access
    explicit operator bool() const;
    
    template <class Token>
    bool is(Token = {}) const;
    
    template <template <typename> class Category>
    bool is_category() const;
    
    const char* name() const;
    
    Integral get() const;
};

// comparison
bool operator==(token_kind lhs, token_kind rhs);
bool operator!=(token_kind lhs, token_kind rhs);
```

Tokens are identified by their index in the [`lex::token_spec`](spec_token_spec.md#token-specification),
`lex::token_kind` is just a strongly-typed wrapper over that index.
The type of the index is the smallest `unsigned` integer that can hold all required values.

All functions are `constexpr` and `noexcept`.

## Creation

```cpp
token_kind();
```

The default constructor creates a token kind corresponding to `lex::error_token`.

```cpp
template <class Token>
static token_kind of();

template <class Token>
token_kind(Token token)
```

Creates a token kind corresponding to the id of the specified token type.

`Token` must be one of the tokens of the `TokenSpec`, including `lex::error_token` or `lex::eof_token`.

As tokens are stateless, they can just be default constructed and passed to the constructor.
When that is not possible because the token type is incomplete, the `static` function can be used.

```cpp
static token_kind from_id(std::size_t id);
```

Creates a token kind from the internal numeric id.
This should only be used passing it the result of `get()`.

## Access

```cpp
explicit operator bool() const;
```

Returns `true` if the kind refers to a token other than `lex::error_token`,
returns `false` if it does refer to `lex::error_token`.

```cpp
template <class Token>
bool is(Token = {}) const;
```

Returns `true` if the kind refers to the specified `Token`.

It can either be invoked as `kind.is(my_token{})` or `kind.is<my_token>()`.

```cpp
template <template <typename> class Category>
bool is_category() const;
```

Let `T` be the token the kind refers to.
If `Category<T>::value` is `true`, returns `true`.
Otherwise, returns `false`.

This can be used with the token traits, for example `kind.is_category<literal_token>()`.


```cpp
const char* name() const;
```

Returns the name of the token the kind refers to.

If this function is used, all tokens must have a `static const char* name` member, which is the name that is returned.
For all tokens except those that inherit from [`lex::basic_rule_token`](spec_rule_token.md#token-specification) or [`lex::rule_token`](spec_rule_token.md#token-rule-dsl) it is automatically provided.
The automatic name can of course be overriden by hiding it in the derived class.
    
```cpp
Integral get() const;
```

Returns the integral id of the token.

## Comparison

```cpp
bool operator==(token_kind lhs, token_kind rhs);
bool operator!=(token_kind lhs, token_kind rhs);
```

Compares the kind by comparing the underlying id.

As a `token_kind` can be implicitly created from a token type, comparisons of the form `kind == my_token{}` are well-formed and do the same as `kind.is<my_token>()`.

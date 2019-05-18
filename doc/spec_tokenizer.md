# Header File `lex/tokenizer.hpp`

The file `tokenizer.hpp` contains the `lex::tokenizer` class, which implements the tokenization algorithm.

```cpp
template <class TokenSpec>
class tokenizer
{
public:
    // constructors
    explicit tokenizer(const char* ptr, std::size_t size);
    explicit tokenizer(const char* begin, const char* end);
    template <std::size_t N>
    explicit tokenizer(const char (&array)[N]);
    
    // tokenization
    token<TokenSpec> peek() const;
    
    void bump();
    
    bool is_done() const;
    
    token<TokenSpec> get(); 
    
    void reset(const char* position);
    
    // getters
    const char* begin_ptr() const;
    const char* current_ptr() const;
    const char* end_ptr() const;
};
```

The tokenizer tokenizes the character range given to it in the constructor using the tokens specified in the token specification.
It will only store the current token in memory.
This makes it lightweight, but parsing grammars with lookahead requires resetting to an earlier position and re-tokenizing again.

All member functions are `constexpr` and `noexcept`.

## Constructor

After initializing the range, the tokenizer will immediately tokenize the first input.

```cpp
explicit tokenizer(const char* ptr, std::size_t size);
```

Tokenizes the range `[ptr, ptr + size)`.

```cpp
explicit tokenizer(const char* begin, const char* end);
```

Tokenizes the range `[begin, end)`.

```cpp
template <std::size_t N>
explicit tokenizer(const char (&array)[N]);
```

Tokenizes the range `[array, array + N - 1)`.

This constructor is meant for null-terminated string literals, which have that type.

## Tokenization

```cpp
token<TokenSpec> peek() const;
```

Returns the current [`lex::token`](spec_token.md#token).

This can be called multiple times and it will always return the same token.

```cpp
void bump();
```

Advances the internal state to the next token in the input,
this works as follows:

1. Consume the characters of the last token, advancing the current input position.

2. If the input is at the end, do nothing.
   Then `is_done()` returns `true` and `peek()` returns `lex::eof_token`.
   As the EOF token has no characters, calling `bump()` repeatedly has no effect.
 
3. Otherwise, try to tokenize the current input:
   1. Try all literal tokens using the trie, selecting the longest literal that matches.
      If this literal token is specified as being in conflict with a rule,
      try if the rule matches and return it instead.
   2. If no literal matched, try the (non-identifier) rule tokens in specification order.
   3. If no rule matched, try the identifier token.
      If it matched, try all keyword literals, returning them if they matched.
      Otherwise, return the identifier token.
   If this process results in a token, `peek()` will return it.
   Otherwise, `peek()` will return a `lex::error_token` consisting of the next character.
   
If during that process, `peek()` would return a token marked as `lex::whitespace_token`, the process is repeated until it does not.


```cpp
bool is_done() const;
```

Returns `true` if the tokenizer reached the end of the input, `false` otherwise.

If this returns `true`, `peek()` returns `lex::eof_token` and `current_ptr() == end_ptr()`.

```cpp
token<TokenSpec> get();
```

Remembers the current token for the return value, and then advances by calling `bump()`.

```cpp
void reset(const char* position);
```

Resets the tokenizer to an arbitrary position in the specified range.
`peek()` will return the token at that position.

## Getters

```cpp
const char* begin_ptr() const;
```

Returns the beginning of the input range, which was given in the constructor.

```cpp
const char* current_ptr() const;
```

Returns the current position of the input range.

The token returned by `peek()` will start at that position.

```cpp
const char* end_ptr() const;
```

Returns the end of the input range, which was given in the constructor.

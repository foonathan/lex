# Header File `lex/rule_token.hpp`

The file `rule_token.hpp` provides the rule token.

```cpp
namespace lex
{
    // token specification
    struct null_token;
    
    template <class Derived, class TokenSpec>
    struct basic_rule_token;
    
    // token rule DSL
    namespace token_rule { … }
    
    template <class TokenSpec>
    class rule_matcher;
    
    template <class Derived, class TokenSpec>
    struct rule_token;
    
    // traits
    template <class Token>
    struct is_null_token;
    
    template <class Token>
    struct is_rule_token; 
}
```

## Token Specification

```cpp
struct null_token;
```

A class derived from `lex::null_token` is a null token.
They are never created by the tokenizer, this has to be done by rule tokens.

---

```cpp
template <class Derived, class TokenSpec>
struct basic_rule_token
{
    using spec          = TokenSpec;
    using token_kind    = lex::token_kind<TokenSpec>;
    using token         = lex::token<TokenSpec>;
    using match_result  = lex::match_result<TokenSpec>;
    using rule_matcher  = lex::rule_matcher<TokenSpec>;
    
    static match_result unmatched();
    static match_result error(std::size_t bump);
    static match_result success(std::size_t bump);
    template <class Token>
    static match_result success(std::size_t bump);
};
```

A class derived from `lex::basic_rule_token` is a rule token.
It has to implement a function `try_match()` with the following signature:

```cpp
static match_result try_match(const char* cur, const char* end) noexcept;
```

The tokenizer will first try to create literals token from the input, if that fails, it will try each rule token in arbitrary order.
This is done by invoking `try_match()`, passing it the current input as half-open range.
If this returns a [`match_result`](spec_match_result.md) where `is_matched() == true`, the resulting token will be created.
Otherwise, the next rule is tried.

For convenience, the base class provides typedefs and creation function that just forward to the `match_result` one.
All the functions are `constexpr` and `noexcept`.

If a [literal token](spec_literal_token.md#token-specification) is a prefix of a rule token, the rule token will never be created.
To prevent this, the derived class can implement a function `is_conflicting_literal()` with the following signature:

```cpp
static bool is_conflicting_literal(token_kind kind) noexcept;
```

If `kind` is a literal token that is a prefix, it shall return `true`.
Otherwise, it shall return `false`.

## Token Rule DSL

```cpp
namespace token_rule { … }
```

The token rule DSL for declarative token specification is described [here](spec_token_rules.md).

---

```cpp
template <class TokenSpec>
class rule_matcher
{
    const char* cur;
    const char* end;
    std::size_t bumped;

public:     
    explicit rule_matcher(const char* cur, const char* end);
    
    template <class Rule>
    bool peek(Rule rule) const;
    
    template <class Rule>
    bool match(Rule rule) const;
    
    match_result<TokenSpec> finish(token_kind<TokenSpec> kind);
    
    template <class Rule>
    match_result<TokenSpec> finish(token_kind<TokenSpec> kind, Rule rule);
};
```

The class `rule_matcher` is used to apply a token rule DSL on an input.
All functions are `constexpr` and `noexcept`.

```cpp
explicit rule_matcher(const char* cur, const char* end);
```

The constructor creates a matcher of the input `[cur, end)`.
`bumped` is initialized to `0`.

```cpp
template <class Rule>
bool peek(Rule rule) const;
```

If `rule` matches the input `[cur, end)`, returns `true`.
Otherwise, returns `false`.

Unlike `match()`, the internal position is not advanced.

```cpp
template <class Rule>
bool match(Rule rule) const;
```

If `rule` matches the input `[cur, end)` consuming `n` characters, it will advance `cur` and `bumped` by `n`, and returns `true`.
Otherwise, returns `false`.

Unlike `peek()`, the internal position is advanced.

```cpp
match_result<TokenSpec> finish(token_kind<TokenSpec> kind);
```

If `bumped == 0`, returns `match_result::unmatched()`.
Otherwise, returns `match_result::success(kind, bumped)`.

```cpp
template <class Rule>
match_result<TokenSpec> finish(token_kind<TokenSpec> kind, Rule rule);
```

Matches the rule like `match()`.

If the rule matched and `bumped > 0`, returns `match_result::success(kind, bumped)`.
If the rule did not match but `bumped > 0`, returns `match_result::error(bumped)`.
If the rule did not match and `bumped == 0`, returns `match_result::unmatched()`.

---

```cpp
template <class Derived, class TokenSpec>
struct rule_token
: lex::basic_rule_token<Derived, TokenSpec>
{
    static match_result try_match(const char* str, const char* end);
};
```

A class derived from `lex::rule_token` is also a rule token.
Instead of manually implementing the matching algorithm, it is done automatically using a specified token rule.
For that, the derived class has to implement a function `rule()`:

```cpp
static constexpr auto rule() noexcept
{
    return /* some token rule DSL expression */;
}
```

The resulting rule will then be matched using the `rule_matcher`.

## Traits

The traits all derive from either `std::true_type` or `std::false_type`,
depending on the result of the condition.

* `is_null_token<Token>`: whether or not `Token` is a null token.
* `is_rule_token<Token>`: whether or not `Token` is a rule token, both `lex::basic_rule_token` and `lex::rule_token`


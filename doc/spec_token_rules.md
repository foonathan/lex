# Token Rules DSL

Token rules are used in combination with the [`lex::rule_matcher`](spec_rule_token.md#token-rule-dsl) or [`lex::rule_token`](spec_rule_token.md#token-rule-dsl).
They allow a compact definition of complex token.

No token (except EOF) can span zero characters.
If rules are specified that would match a zero character sequence, they are considered unmatched.
This means that a single, top-level `tr::opt(r)` is equivalent to `r` and a single, top-level `tr::star(r)` is equivalent to `tr::plus(r)`, etc.

All rules are defined in namespace `lex::token_rule` in the header [`rule_token.hpp`](spec_rule_token.md).
The following rule descriptions assumes the namespace alias `tr = lex::token_rule`.

## Primitive Rules

**Character**

Every character is a primitive rule.
It matches if the next character of the input is the specified character.
It then consumes one character.

```cpp
constexpr auto rule()
{
    return 'x';
}
```

* `"xyz"` → `"yz"`
* `"x"` → `""`
* `"abc"` → `"abc"` (unmatched)
* `""` → `""` (unmatched)

**String**

Every string is a primitive rule.
It matches if the current input starts with the specified string.
It then consumes `strlen()` characters.

```cpp
constexpr auto rule()
{
    return "hello";
}
```

* `"hello world"` → `" world"`
* `"helloworld"` → `"world"`
* `"hello"` → `""`
* `"hell"` → `"hell"` (unmatched)
* `""` → `""` (unmatched)

**Predicate**

Every function with the signature `bool(char)` is a primitive rule.
It is called passing it the next character of the input and matches if the function returns `true`.
It then consumes one character.

Every function in the `lex::ascii` namespace has that signature.

```cpp
bool is_valid(char c) { return c == 'a'; }

constexpr auto rule()
{
    return is_valid; 
}
```

* `"abc"` → `"bc"`
* `"a" → `""`
* `"xyz"` → `"xyz"` (unmatched)
* `""` → `""` (unmatched)

**Function**

Every function with the signature `std::size_t(const char*, const char*)` is a primitive rule.
It is called passing it the input.
If the function returns zero, the rule does not match.
Otherwise, the rule matches and the specified amount of characters are consumed.

```cpp
std::size_t match(const char* begin, const char* end){ … }

constexpr auto rule()
{
    return match;
}
```

**Wrapper `tr::r(rule)`**

C++ does not allow operator overloading for built-in types.
As all primitive rules are built-in types, they can't be used with the operator overloads.
For that, they can be wrapped in `tr::r()`.
The result is a type that has the operator overloads.

Redundant calls to `tr::r()` are ignored.

```cpp
constexpr auto rule()
{
    // `'#' + lex::ascii::is_alpha` would not work
    return tr::r('#') + lex::ascii::is_alpha;
}
```

## Atomic Rules

**Any `tr::any`**

The any rule matches if the input has characters left.
It then consumes one character.

```cpp
constexpr auto rule()
{
    return tr::any;
}
```

* `"xyz"` → `"yz"`
* `""` → `""` (unmatched)

**Skip `tr::skip<N>`**

The skip rule matches if the input has `N` character left.
It then consumes `N` characters.

`tr::any` is an alias for `tr::skip<1>`.

```cpp
constexpr auto rule()
{
    return tr::skip<2>;
}
```

* `"xyz"` → `"z"`
* `"x"` → `"x"` (unmatched)
* `""` → `""` (unmatched)

**EOF `tr::eof`**

The eof rule matches if the input does not have any characters left.
It is the only rule that consumes nothing *if* it matched.

It is useful if certain tokens must be at the end of a file.

```cpp
constexpr auto rule()
{
    return tr::eof;
}
```

* `"xyz"` → `"xyz"` (unmatched)
* `""` → `""` (matched!)

**Fail `tr::fail`**

The fail rule never matches.

It is useful as part of combinators.

```cpp
constexpr auto rule()
{
    return tr::fail;
}
```

* `"xyz"` → `"xyz"` (unmatched)
* `""` → `""` (unmatched)

## Combinator Rules

Combinator rules combine existing rules to create more advanced onces.
They are the classical PEG combinators.

**Sequence `+`**

The operator `+` can be used to create a sequence.
`a + b` matches if `a` matched, and then `b` matched on the new input position.
It then consumes what `a` and what `b` consumed.

```cpp
constexpr auto rule()
{
    // equivalent to `"abc"`
    return tr::r('a') + 'b' + 'c';
}
```

**Ordered Choice `/`**

The operator `/` can be used to create an ordered choice.
`a / b` matches if `a` or `b` matched, consuming what `a` or `b` consumed.
The alternatives are tried in the specified order.

```cpp
constexpr auto rule()
{
    return tr::r("ab") / "a" / "b";
}
```

* `"abc"` → `"c"` (first alternative)
* `"ac"` → `"c"` (second alternative)
* `"bc"` → `"c"` (third alternative)
* `"c"` → `"c"` (unmatched)

```cpp
constexpr auto rule()
{
    return tr::r("a") / "ab" / "b";
}
```

* `"abc"` → `"bc"` (first alternative!)
* `"ac"` → `"c"` (first alternative)
* `"bc"` → `"c"` (third alternative)
* `"c"` → `"c"` (unmatched)

Here `"ab"` is never considered as `"a"` would already match.

**Optional `tr::opt(rule)`**

The optional rule always matches.
If `rule` matches, it consumes it.
Otherwise, it consumes nothing.

```cpp
constexpr auto rule()
{
    return tr::opt('a') + 'b';
}
```

* `"abc"` → `"c"` (optional taken)
* `"bc"` → `"c"` (optional not taken)
* `"c"` → `"c"` (unmatched, but because of `b`)

**Zero-or-more `tr::star(rule)`**

The zero-or-more rule always matches.
It tries to consume `rule` as often as possible.

```cpp
constexpr auto rule()
{
    return tr::star('a') + 'b';
}
```


* `"abc"` → `"c"` (one)
* `"aaabc"` → `"c"` (three)
* `"bc"` → `"c"` (zero, still matched)
* `"c"` → `"c"` (unmatched, but because of `b`)

**One-or-more `tr::plus(rule)`**

Equivalent to: `rule + tr::star(rule)`

The one-or-more rule matches if `rule` matches.
If matched, it consumes `rule` and then consumes it as often as possible.

```cpp
constexpr auto rule()
{
    return tr::plus('a');
}
```

* `"abc"` → `"bc"` (one)
* `"aaabc"` → `"bc"` (three)
* `"bc"` → `"bc"` (unmatched)
* `"c"` → `"c"` (unmatched)
* `""` → `""` (unmatched)

```cpp
constexpr auto rule()
{
    return tr::plus('a') + 'b';
}
```


* `"abc"` → `"c"` (one)
* `"aaabc"` → `"c"` (three)
* `"bc"` → `"c"` (unmatched)

**Lookahead `tr::lookahead(rule)`**

Alternative spelling: `&rule` (address-of operator)

The lookahead rule matches if `rule` matches but does not consume anything.
It can be used to guide complex choices.

```cpp
constexpr auto rule()
{
    return tr::lookahead("ab") + "a";
}
```

* `"abc"` → `"bc"`
* `"ac"` → `"ac"` (unmatched)

**Negative Lookahead `tr::neg_lookahead(rule)`**

Alternative spelling: `!rule`

The negative lookahead rule matches if `rule` does not match.
It never consumes anything.
It can be used to guide complex choices.

```cpp
constexpr auto rule()
{
    return tr::neg_lookahead("ab") + "a";
}
```

* `"abc"` → `"abc"` (unmatched)
* `"ac"` → `"c"` (matched)

## Convenience Rules

**Minus `tr::minus(rule, subtrahend)`**

The minus rule does a set-minus operation.
It matches if `rule` matched and `subtrahend` didn't match the part of the input `rule` consumed.
If matched, it consumes what `rule` consumed.

```cpp
constexpr auto rule()
{
    // non-zero digit
    return tr::minus(lex::ascii::is_digit, '0');
}
```

* `"12"` → `"2"`
* `"09"` → `"09"` (unmatched)

**If-then-else `tr::if_then_else(condition, then, else)`**

Equivalent to: `(condition + then) / (!condition + else)`

If the input matches `condition`, matches `then` in sequence.
If the input does not match `condition`, matches `else` in sequence.

```cpp
constexpr auto rule()
{
    return tr::if_then_else('a', 'b', 'c');
}
```

* `"abxyz"` → `"xyz"` (first branch)
* `"cxyz"` → `"xyz"` (second branch)
* `"axyz"` → `"axyz"` (unmatched, `Then` not after `Condition`)
* `"acxyz` → `"acxyz"` (unmatched, `Then` not after `Condition`)

**Loop `tr::until(end, rule)`

Equivalent to: `tr::star(!end + rule) + end`

Matches `rule` repeatedly, until `end` is matched.
`rule` defaults to `tr::any`.

```cpp
constexpr auto rule()
{
    return tr::until('a', 'b');
}
```

* `"bbbbaxyz"` → `"xyz"`
* `"axyz"` → `"xyz"`
* `"bca"` → `"bca"` (unmatched)

```cpp
constexpr auto rule()
{
    return tr::until('a');
}
```

* `"xyzabc"` → `"bc"`
* `"abc"` → `"bc"`
* `"bc"` → `"bc"` (unmatched)

**Excluding Loop `tr::until_excluding(end, rule)`**

Equivalent to: `tr::star(!end + rule) + &rule`

Matches `rule` repeatedly, until `end` is matched, but does not consume `end`.
`rule` defaults to `tr::any`.

```cpp
constexpr auto rule()
{
    return tr::until_excluding('a', 'b');
}
```

* `"bbbbaxyz"` → `"axyz"`
* `"axyz"` → `"axyz"`
* `"bca"` → `"bca"` (unmatched)

```cpp
constexpr auto rule()
{
    return tr::until_excluding('a');
}
```

* `"xyzabc"` → `"abc"`
* `"abc"` → `"abc"`
* `"bc"` → `"bc"` (unmatched)

**List `tr::list(element, separator)` / `tr::list_trailing(element, separator)`**

Equivalent to: `element + tr::star(separator + element)` (first version)

Equivalent to: `tr::list(element, separator) + tr::opt(separator)` (second version)

Matches a non-empty list of `element` separated by `separator`.
`tr::list_trailing()` then consumes an optional trailing separator at the end as well.

```cpp
constexpr auto rule()
{
    return tr::list('a', ',');
}
```

* `"a,a"` → `""`
* `"a"` → `""`
* `""` → `""` (unmatched)

**Padded `tr::opt_padded(left, rule, right)` / `tr::padded(left, rule, right)`**

Equivalent to: `tr::star(left) + rule + tr::star(right)` (first version)

Equivalent to: `(tr::plus(left) + rule + tr::star(right)) / (rule + tr::plus(right))` (second version)

Matches `rule` with arbitrary amount of padding left and right.
For `tr::padded()` there must be padding on at least one side at least once.

```cpp
constexpr auto rule()
{
    return tr::opt_padded('l', 'a', 'r');
}
```

* `"a"` → `""`
* `"llllarrrr"` -> `""`

**Repetition `tr::repeated<min, max>(rule)`**

Matches if `rule` matched at least `min` times but at most `max` times.

```cpp
constexpr auto rule()
{
    return tr::repeated<1, 3>('a');
}
```

* `"a"` → `""`
* `"aa"` → `""`
* `"aaa"` → `""`
* `""` → `""` (unmatched)
* `"aaaa"` → `"aaaa"` (unmatched)

There are aliases for the edge cases:

* `tr::times<n>(rule)` is equivalent to `repeated<n, n>(rule)` — matches if `rule` matches `n` times (but not more)
* `tr::at_most<n>(rule)` is equivalent to `repeated<0, n>(rule)` — matches if `rule` matches `n` times or less (but not more)
* `tr::at_least<n>(rule)` is equivalent to `repeated<n, ∞>`(rule)` — matches if `rule` matches `n` times or more (but not less)


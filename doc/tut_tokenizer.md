# Token Specification and Tokenization

Before parsing the library has to turn a sequence of characters into a sequence of tokens.

Tokens are the atomic units of the parser.
They can be multiple characters but are treated as just one thing.
For example, in C++ tokens are keywords like `int`, identifiers like `main`, literals like `42` or punctuation like `+=`.

In foonathan/lex there are three representation of a token:

* the token type: this is a user-defined class that defines what the token is
* the [`lex::token_kind`](spec_token_kind.md): this is a runtime identifier of a token type
* the [`lex::token`](spec_token.md#token): this is the runtime token value, i.e. the token kind and exact spelling

## Token Specification

Tokens are created by creating new types, the token types:
You define a new class and inherit from one of the token base types, like [`lex::literal_token`](spec_literal_token.md#token-specification), passing the base type some template arguments and optionally adding some required member functions.

```cpp
struct my_token : lex::literal_token<…>
{
    // potentially some member functions
};
```

Due to the lack of reflection, you have to manually list all the tokens of the grammar manually.
This is done by creating yet another type that inherits from [`lex::token_spec`](spec_token_spec.md#token-specification) passing it all your token types.
The token specification has to be defined *before* any tokens are defined, which means that the types have to be forward declared.

It usually looks like this:

```cpp
struct my_token_spec
: lex::token_spec<struct my_token_a, struct my_token_b, …>
{};

struct my_token_a : lex::…
{
    …
};

struct my_token_b : lex::…
{
    …
};

… // more tokens
```

Each token specification implicitly contains two special tokens:

* `lex::eof_token` is a token that represents the end of the input.

* `lex::error_token` is a token that represents a character that could not match to any of the other tokens.
For example, C++ does not use the character `@`, it would thus generate `lex::error_token` instead.
Error tokens always consist of only a single character.

### Literal Tokens

The most basic token is a plain constant string.
It is usually used for punctuation and operators.
To specify a literal token, inherit from [`lex::literal_token`](spec_literal_token.md#token-specification) passing it the characters of your string.

**Note**: You should not use it for keywords, [see below](#keyword-and-identifier-tokens).

For example, a calculator would use a literal token for the operators:

```cpp
struct plus
: lex::literal_token<'+'> {};
struct minus 
: lex::literal_token<'-'> {};
struct star
: lex::literal_token<'*'> {};
struct slash
: lex::literal_token<'/'> {};
```

The input `+` will be turned into a `plus` token, `*/` into `star` followed by `slash` etc.

If multiple literals share a common prefix, the longest literal wins (maximal munch):

```cpp
struct plus
: lex::literal_token<'+'> {};
struct plus_plus
: lex::literal_token<'+', '+'> {};
struct plus_plus_equal
: lex::literal_token<'+', '='> {};
```

The input `++` will be `plus_plus` and not two `plus`,
the input `++=` will be `plus_plus` and then `lex::error_token` as `=` is not a valid token.
Tokenization is "dumb" and does not realize that turning it into `plus` followed by `plus_equal` would work.

This is consistent with the way many programming languages work.
For example, in C++ `1--2` is a syntax error as it is read as `1-- 2` and not `1 - (-2)`.

If you think that it is annoying to write `literal_token<'s', 'o', 'm', 'e', 'l', 'i', 't', 'e', 'r', 'a', 'l'>`, you can use the macro `FOONATHAN_LEX_LITERAL`:

```cpp
struct some_literal_annoying
: lex::literal_token<'s', 'o', 'm', 'e', 'l', 'i', 't', 'e', 'r', 'a', 'l'> {};
struct some_literal_easy
: FOONATHAN_LEX_LITERAL("someliteral") {};
```

The two tokens are identical thanks to preprocessor magic.

**Note**: Actually having two identical literals is an error.

### Rule Tokens

Rule tokens are used for tokens that have dynamic contents as opposed to static literal strings.
They are used for things like integer literals or comments.

They can be specified in one of two ways.
For the manual version, inherit from [`lex::basic_rule_token`](spec_rule_token.md#token-specification).
This is a [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) base class taking two parameters:
The first is the derived class and the second is the token spec.
The derived class must implement a `static` member function `try_match()` that takes the character range and parses the token:

```cpp
struct my_token_spec
: lex::token_spec<struct my_rule_token, …> {};

struct my_rule_token
: lex::basic_rule_token<my_rule_token, my_token_spec>
{
    static match_result try_match(const char* begin, const char* end)
    {
        // parse the token
    }
};
```

The result of `try_match()` is [`lex::match_result<my_token_spec>`](spec_match_result.md).
It can have one of three states:

* *unmatched*: the input did not match the token. Nothing in the input is consumed and the next token is tried.

* *error*: the input did not match *any* token. Then a non-zero amount of characters are consumed, a `lex::error_token` created and tokenization tries again with the new position of the input.

* *success*: the input did match the token. Then a non-zero amount of characters are consumed, the token created and tokenization continues with the new position of the input.

The job of `try_match()` is to look at the input and return a correct `match_result.`
If the input matches the token we're trying to parse, it returns a success result.
Otherwise, if the input looks like the token we're trying to parse, but has some syntax error (e.g. a string literal without a closing `"`), it returns an error result.
Otherwise, it returns unmatched, and tokenization will try a different token.

Inheriting from `lex::basic_rule_token` gives access to some convenience functions and types to make writing the `try_match()` function easier.
Like shown above, you don't need to write `lex::match_result<my_token_spec>`
but have access to the type alias `match_result`.
Similar, there is `token` instead of `lex::token<my_token_spec>` and `token_kind` instead of `lex::token_kind<my_token_spec>`, and `rule_matcher` instead of `lex::rule_matcher<my_token_spec>`.
To create a `match_result` you can use `unmatched()`, `error()` and `success()`.
The last two require the (non-zero) amount of characters that should be consumed by the token as argument.

With that we can write a rule token that parses a Python comment (`#` followed by any character until newline):

```cpp
struct python_comment
: lex::basic_rule_token<python_comment, my_token_spec>
{
    static match_result try_match(const char* begin, const char* end)
    {
        auto start = begin;
        if (begin == end)
            // no characters left, so not a comment
            return unmatched();
            
        if (*begin != '#')
            // must start with '#'
            return unmatched();
        ++begin;
        
        // skip characters until we've reached a newline
        while (begin != end && !lex::ascii::is_newline(*begin))
            ++begin;
            
        // we've successfully consumed `begin - start` characters
        return success(begin - start);
    }
};
```

Specifying a token in such a way is verbose and error-prone, so there is a way to automate the definition.

#### Token Rules

To create rule tokens in a more declaritive way, you can use *token rules*.
They are based on [parsing expression grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar) and allow specification of tokens similar to e.g. EBNF.

For example, we can also define the `python_comment` like so:

```cpp
struct python_comment
: lex::basic_rule_token<python_comment, my_token_spec>
{
    static match_result try_match(const char* begin, const char* end)
    {
        namespace tr = lex::token_rule;
        
        // '#' followed by anything until newline
        auto rule = '#' + tr::until(lex::ascii::is_newline, tr::any);
        
        // if the rule matched, we want to create a `python_comment`
        rule_matcher matcher(begin, end);
        return matcher.finish(python_comment{}, rule);
    }
};
```

For a full specification of the token rule DSL, [see here](spec_token_rules.md).
Just note how the rule specification reads a lot like the literal description.

Because this boilerplate would be very common, you can also use [`lex::rule_token`](spec_rule_token.md#token-rule-dsl) instead of `lex::basic_rule_token`.
It takes the same two arguments, but instead of `try_match()` you need to provide `rule()`:

```cpp
struct python_comment
: lex::rule_token<python_comment, my_token_spec>
{
    static constexpr auto rule()
    {
        namespace tr = lex::token_rule;
        
        // '#' followed by anything until newline
        return '#' + tr::until(lex::ascii::is_newline, tr::any);
    }
};
```

`lex::rule_token` already has a `try_match()` function which looks like the one we've implemented above,
but instead of hard-coding the rule it invokes the function from the derived class.
Note that `rule()` can (and must) be `constexpr`.
This ensures that it will disappear at runtime.

##### Handling Conflicts

Matching literal tokens is very fast, so when tokenization, they are tried first.
But this can be problematic.

Consider a C-style language with `//` comments and a division operator:

```cpp
struct comment
: lex::rule_token<comment, my_token_spec>
{
    static constexpr auto rule() 
    {
        namespace tr = lex::token_rule;
        return "//" + tr::until(lex::ascii::is_newline);
    }
};

struct div
: lex::literal_token<'/'>
{};
```

When tokenizing `// hello`, the tokenizer will first try all literal tokens.
It sees that `/` matches, so this token is used.
As the tokenizer does no backtracking, it means that a `comment` token will never be created, as `div` takes priority!

To prevent that, implement the `is_conflicting_literal()` method.
It takes a `token_kind` and should return `true` if that token is a literal token that would conflict with the rule token, i.e. it starts with the same character sequence.
Then it is checked whether the rule would match after the literal token matched.
And if it does, the rule is created instead.

This means that we actually need to write `comment` like this:

```cpp
struct comment
: lex::rule_token<comment, my_token_spec>
{
    // new: handle conflict
    static constexpr bool is_conflicting_literal(token_kind kind) 
    {
        return kind.is<div>();
    }

    // as before
    static constexpr auto rule() 
    {
        namespace tr = lex::token_rule;
        return "//" + tr::until(lex::ascii::is_newline);
    }
};
```

Note that `is_conflicting_literal()` has to be `constexpr`, as it is used when generating the tokenization algorithm at compile-time.

We've used `lex::rule_token` here for convenience but this of course also applies to `lex::basic_rule_token`.

#### Matching Multiple Tokens at Once

Some tokens have overlap.
For example, both integer and floating point literals start with a sequence of digits.
We could parse them like so:

```cpp
struct integer_literal
: lex::rule_token<integer_literal, my_token_spec>
{
    static constexpr auto rule()
    {
        namespace tr = lex::token_rule;
        // non-zero amount of digits followed by no '.'
        return tr::plus(lex::ascii::is_digit) + !tr::r('.');
    }
};

struct float_literal
: lex::rule_token<float_literal, my_token_spec>
{
    static constexpr auto rule()
    {
        namespace tr = lex::token_rule;
        // non-zero amount of digits followed by '.' and more digits
        return tr::plus(lex::ascii::is_digit) + '.' + tr::star(lex::ascii::is_digit);
    }
};
```

> Note that we've deliberately avoided ambiguity by specifying that no `.` must follow an integer literal.
> Otherwise, whether or not `123.4` is a float literal or an integer literal, then a `.`, then another integer literal, would be unspecified.
> By saying that an integer literal only matches if the next token isn't `.`, this is avoided.

However, writing it that way is less efficient as it could be.
When parsing `123…9.1` where we have a long sequence of numbers, it first reads all the digits trying to do an integer literal, sees the `.` and aborts, and then reads them all again trying to do a floating point literal.

This can be avoided by handling both integer and floating point literal at once.

The `try_match()` function of `lex::basic_rule_token` can return *which* token was matched:

```cpp
struct int_literal
: lex::null_token
{};

struct float_literal
: lex::basic_rule_token<float_literal, my_token_spec>
{
    static match_result try_match(const char* begin, const char* end)
    {
        auto start = begin;
        // parse sequence of digits 
        
        if (begin != end && *begin == '.')
        {
            ++begin;
            // parse remainder of float
            
            return success<float_literal>(begin - start); // match float
        }
        else
            // not a float
            return succes<int_literal>(begin - start); // match int
    }
};
```

We've used `lex::null_token` for the integer literal.
This just means that it is a token, but the tokenizer isn't responsible for its matching.
The `try_match()` function of the floating literal parses the common code, and then decides which token to create.

We can still use the token rules with this method as well, using the `rule_matcher`:

```cpp
struct int_literal
: lex::null_token
{};

struct float_literal
: lex::basic_rule_token<float_literal, my_token_spec>
{
    static match_result try_match(const char* begin, const char* end)
    {
        namespace tr = lex::token_rule;
        rule_matcher matcher(begin, end);
        
        // first match digits
        if (!matcher.match(tr::plus(lex::ascii::is_digit))
            return unmatched();
            
        if (matcher.match('.'))
            // match float with additional digits
            return matcher.finish(float_literal{}, tr::star(lex::ascii::is_digit));
        else
            // matched an int
            return matcher.finish(int_literal{});
    }
};
```

We construct a matcher over the input.
With `.match()` we can ask whether a rule would match at the current position.
If it does match, it also advances the internal input position.
With `.finish()` it returns the appropriate `match_result`, after optionally matching an additional rule.

### Keyword and Identifier Tokens

An identifier in a C-like language can be defined like this:

```cpp
struct identifier
: lex::rule_token<identifier, my_token_spec>
{
    static constexpr auto rule() noexcept
    {
        namespace tr = lex::token_rule;
        // start followed by zero-or-more rest characters
        return is_identifier_start + tr::star(is_identifier_rest);
    }

    static constexpr bool is_identifier_start(char c) noexcept
    {
        return lex::ascii::is_alpha(c) || c == '_';
    }

    static constexpr bool is_identifier_rest(char c) noexcept
    {
        return lex::ascii::is_alnum(c) || c == '_';
    }
};
```

Some identifiers are reserved, they're keywords.
For example, we can define the `int` keywords using a literal token:

```cpp
struct kw_int
: FOONATHAN_LEX_LITERAL("int")
{};
```

However, this causes conflicts when parsing `"integer"`, for example.
This is an identifier, but the tokenizer will prefer the literal `int`, and consume it, and then create the identifier `eger`.

Specifying every keyword as conflicting in the literal is tedious, so there is a better alternative.
Instead of designating `identifier` as `lex::rule_token` and keywords as `lex::literal_token`, use [`lex::identifier_token`](spec_identifier_token.md#token-specification) and [`lex::keyword_token`](spec_identifier_token.md#token-specification) instead:

```cpp
struct identifier
: lex::identifier_token<identifier, my_token_spec>
{
    static constexpr auto rule() noexcept
    {
        // as before
    }
};

struct kw_int
: FOONATHAN_LEX_KEYWORD("int")
// or lex::keyword_token<'i', 'n', 't'>
{}:
```

Now the tokenization logic is changed:
The tokenizer will not consider keywords when tokenizing, only the identifier.
If the identifier matches, it will then check the result for keywords.
If a keyword matches, that is returned, otherwise the identifier is.

So when tokenizing `"integer"`, it is identified as an identifier.
Then the whole string `"integer"` is checked for keywords.
It isn't a keyword, so it is a single `identifier`.

**Note:** Every token specification must contain at most one `lex::identifier_token`.
If it contains any `lex::keyword_token`, then it must contain exactly one `lex::identifier_token`.

### Whitespace Tokens

Most programming languages allow whitespace in arbitrary places.
Specifying it in the grammar would be tedious.

Instead you can use [`lex::whitespace_token`](spec_whitespace_token.md).
Inherit from it *in addition* to one of the other base classes.
The resulting token will be matched, but transparently skipped.

For example:

```cpp
struct whitespace
: lex::rule_token<whitespace, my_token_spec>, lex::whitespace_token
{
    static constexpr auto rule()
    {
        return lex::token_rule::star(lex::ascii::is_whitespace);
    }
};
```

Now tokenization of e.g. `a b c` will not mention any `whitespace` but just whatever the rest is.

Note that a `lex::whitespace_token` does not need to be "whitespace".
You can choose to ignore any token.
It is also commonly used for comments.

## Tokenization

Once all tokens are specified, input can be tokenized using [`lex::tokenizer`](spec_tokenizer.md).
It is a template that needs the token specification and uses the compile-time information to construct a trie for efficient processing.

The basic use case looks like this:

```cpp
lex::tokenizer<my_token_spec> tokenizer(begin, end); // tokenize [begin, end)
while (!tokenizer.is_done()) // while we have not reached EOF
{
    lex::token<my_token_spec> cur = tokenizer.get(); // tokenize the next input
    …
}
```

### `lex::tokenizer`

The tokenizer itself is very lightweight, as tokenization is done lazily.
It just stores the current input position and current token.
`tokenizer.peek()` returns the current token and `tokenizer.bump()` consumes the current token and advances the input by the characters it consumed.
`tokenizer.get()` combines the two.

As tokenization is done lazily, it does not support token look-ahead.
But `lex::tokenizer` is lightweight, so it can be copied and the old state restored.
It can also be reset to an arbitrary position in the input using `tokenizer.reset()`.

### `lex::token`

The result of `tokenizer.peek()` is [`lex::token<my_token_spec>`](spec_token.md#token).
It is the lightweight representation of a single token,
basically just a [`lex::token_kind<my_token_spec>`](spec_token_kind.md) and a string view.

The string view is the range of characters occupied by the tokens.
As this is a view into the input, it can also be used to calculate the file position of a token.
For example, the offset into the input is simply the difference between the start pointer of the token and the start of the input.

`lex::token_kind<my_token_spec>` is basically just the index of the token in the token specification.
It uniquely identifies one of the token types and provides accessors to check whether the token is a specified type.
For convenience, `lex::token` provides all the member functions of `lex::token_kind` as well.

Note that the resulting tokens are always strings: the characters the token occupies.
Turning for example an `integer_literal` into an actual integer value has to be done during parsing.

### Named Tokens

Both `lex::token` and `lex::token_kind` have a `.name()` member function.
It returns a human readable name of the *kind* of token it is.

If this function is used it requires support from certain token types:

* The special `lex::eof_token` and `lex::error_token` have the name `<eof>` and `<error>`, respectively.
* The name of a literal token is the literal itself.
* The name of the identifier token is `<identifier>`.
* The name of a keyword token is the keyword itself.
* The name of a rule token (basic or automated) has to be provided:
  For that, add a `static const char* name = …;` member to the token type.

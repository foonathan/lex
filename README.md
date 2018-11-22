# foonathan/lex

[![Build Status](https://dev.azure.com/foonathan/lex/_apis/build/status/foonathan.lex)](https://dev.azure.com/foonathan/lex/_build/latest?definitionId=2)

> Note: This project is currently WIP, no guarantees are made until an 0.1 release.

This library is a C++14 `constexpr` tokenization and (in the future) parsing library.
The tokens are specified in the type system so they are available at compile-time.
With this information a [trie](https://en.wikipedia.org/wiki/Trie) is constructed that efficiently matches the input.

## Basic Example

The tokens for a simple calculator:

```cpp
using tokens = lex::token_spec<struct variable, struct plus, struct minus, …>;

struct variable : lex::rule_token<variable, tokens>
{
    static constexpr auto rule() const noexcept
    {
        // variables consists of one or more characters
        return lex::token_rule::plus(lex::ascii::is_alpha);
    }
};

struct plus : lex::literal_token<'+'>
{};

struct minus : lex::literal_token<'-'>
{};
```

See [example/ctokenizer.cpp](example/ctokenizer.cpp) for an annotated example and tutorial.

## Features

* Declarative token specification: No need to worry about ordering or implementing lexing by hand.
* Fast: Performance is comparable or faster to a handwritten state machine, see benchmarks.
* Lightweight: No memory allocation, tokens are just string views into the input.
* Lazy: The `lex::tokenizer` will just tokenize the next token in the input.
* Fully `constexpr`: The entire lexing can happen at compile-time.
* Flexible error handling: On invalid input, a `lex::error_token` is created consuming one characters.
The parser can then decide how an error should be handled.

## FAQ

**Q: Isn't the name [lex](https://en.wikipedia.org/wiki/Lex_(software)) already taken?**

A: It is. That's why the library is called `foonathan/lex`.
In my defense, naming is hard.
I could come up with some cute name, but then its not really descriptive.
If you know `foonathan/lex`, you know what the project is about.

**Q: Sounds great, but what about compile-time?**

A: Compiling the `foonathan_lex_ctokenizer` target, which contains an implementation of a tokenizer for C (modulo some details),
takes under three seconds.
Just including `<iostream>` takes about half a second, including `<iostream>` and `<regex>` takes about two seconds.
So the compile time is noticeable, but as a tokenizer will not be used in a lot of files of the project and rarely changes, acceptable.

In the future, I will probably look at optimizing it as well.

**Q: My `lex::rule_token` doesn't seem to be matched?**

A: This could be due to one of two things:

* Multiple rule tokens would match the input. Then the tokenizer just picks the one that comes first.
  Make sure that all rule tokens are mutually exclusive, maybe by using `lex::null_token` and creating them all in one place at necessary.
  See `int_literal` and `float_literal` in the C tokenizer for an example.
* A literal token is a prefix of the rule token, e.g. a C comment `/* … */` and the `/` operator are in conflict.
  By default, the literal token is preferred in that case.
  Implement `is_conflicting_literal()` in your rule token as done by the `comment` token in the C tokenizer.

A mode to test for this issues is planned.

**Q: The `lex::tokenizer` gives me just the next token, how do I implement lookahead for specific tokens?**

A: Simple call `get()` until you've reached the token you want to lookahead, then `reset()` the tokenizer to the earlier position.

**Q: How does it compare to [compile-time-regular-expressions](https://github.com/hanickadot/compile-time-regular-expressions)?**

A: That project implements a RegEx parser at compile-time, which can be used to match strings.
foonathan/lex is project is purely designed to tokenize strings.
You could implement a tokenizer with the compile-time RegEx but I have choosen a different approach.

**Q: How does it compare to [PEGTL](https://github.com/taocpp/PEGTL)?**

A: That project implements matching parsing expression grammars (PEGs), which are a more powerful RegEx, basically.
On top of that they've implemented a parsing interface, so you can create a parse tree, for example.
foonathan/lex currently does just tokenization, but I plan on adding parse rules on top of the tokens later on.
Complex tokens in foonathan/lex can be described using PEG as well, but here the PEGs are described using operator overloading and functions,
and in PEGTL they are described by the type system.

**Q: It breaks when I do this!**

A: Don't do that. And file an issue (or a PR, I have a lot of other projects...).

**Q: This is awesome!**

A: Thanks. I do have a Patreon page, so consider checking it out:

[![Patreon](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://patreon.com/foonathan)

## Documentation

> A full reference documentation is WIP, look at the comments in the header files for now.

### Compiler Support

The library requires a C++14 compiler with reasonable `constexpr` support.
Compilers that are being tested on CI:

* Linux:
    * GCC 5 to 8, but compile-time tokenization is not support for GCC < 8 (still works at runtime)
    * clang 4 to 7
* MacOS:
    * XCode 9 and 10
* Windows:
    * Visual Studio 2017

### Installation

The library is header-only and has only my [debug_assert](https://github.com/foonathan/debug_assert) library as dependency.

If you use CMake, `debug_assert` will be cloned automatically if not installed on the system.
You can use it with `add_subdirectory()` or install it and use `find_package(foonathan_lex)`,
then link to `foonathan::foonathan_lex` and everything will be setup automatically.

## Planned Features

* Parser on top of the tokenizer
* Integrated way to handle data associated with tokens (like the value of an integer literal)
* Optimize compile-time


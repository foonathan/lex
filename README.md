# foonathan/lex

![Project Status](https://img.shields.io/endpoint?url=https%3A%2F%2Fwww.jonathanmueller.dev%2Fproject%2Flex%2Findex.json)
[![Build Status](https://dev.azure.com/foonathan/lex/_apis/build/status/foonathan.lex)](https://dev.azure.com/foonathan/lex/_build/latest?definitionId=2)

> Note: I have a partial parsing implementation already but realized I should switch to a token-less parser instead.
> As such, I am currently working on a major rewrite of the project.

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

Tutorial and reference documentation can be found [here](doc/doc.md).

### Compiler Support

The library requires a C++14 compiler with reasonable `constexpr` support.
Compilers that are being tested on CI:

* Linux:
    * GCC 5 to 8, but compile-time parsing is not supported for GCC < 8 (still works at runtime)
    * clang 4 to 7
* MacOS:
    * XCode 9 and 10
* Windows:
    * Visual Studio 2017, but compile-time parsing sometimes doesn't work (still works at runtime)

### Installation

The library is header-only and requires my [debug_assert](https://github.com/foonathan/debug_assert) library as well as the (header-only and standalone) [Boost.mp11](https://github.com/boostorg/mp11).

#### Using CMake `add_subdirectory()`:

Download and call `add_subdirectory()`.
It will look for the dependencies with `find_package()`, if they're not found, the git submodules will be used.

Then link to `foonathan::foonathan_lex`.

#### Using CMake `find_package()`:

Download and install, setting the CMake variable `FOONATHAN_LEX_FORCE_FIND_PACKAGE=ON`.
This requires the dependencies to be installed as well.

Then call `find_package(foonathan_lex)` and link to `foonathan::foonathan_lex`.

##### With other buildsystems:

You need to set the following options:

* Enable C++14
* Add the include path, so `#include <debug_assert.hpp>` works
* Add the include path, so `#include <boost/mp11/mp11.hpp>` works
* Add the include path, so `#include <foonathan/lex/tokenizer.hpp>` works

## Planned Features

* Parser on top of the tokenizer
* Integrated way to handle data associated with tokens (like the value of an integer literal)
* Optimize compile-time


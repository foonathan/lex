# Header File `lex/ascii.hpp`

The file `ascii.hpp` provides functions to check the categories of an ASCII characters.
All functions are in the namespace `lex::ascii` and have the signature `bool(char)`,
which makes them valid predicates for the [token rule DSL]().

```cpp
namespace lex::ascii
{
    // ASCII check
    constexpr bool is_ascii(char c) noexcept;
    
    // atomic ASCII categories
    constexpr bool is_control(char c) noexcept;
    constexpr bool is_blank(char c) noexcept;
    constexpr bool is_newline(char c) noexcept
    constexpr bool is_other_space(char c) noexcept;
    constexpr bool is_digit(char c) noexcept;
    constexpr bool is_lower(char c) noexcept; 
    constexpr bool is_upper(char c) noexcept;
    constexpr bool is_punct(char c) noexcept;
    
    // compound ASCII categories
    constexpr bool is_space(char c) noexcept;
    constexpr bool is_alpha(char c) noexcept;
    constexpr bool is_alnum(char c) noexcept;
    constexpr bool is_graph(char c) noexcept;
    constexpr bool is_print(char c) noexcept;
}
```

## ASCII Check

`is_ascii()` returns whether or not the character is an ASCII character.
If this returns `false`, all other functions will also return `false`.

## Atomic ASCII Categories

Every ASCII character is exactly in one of these categories:

* *control*: one of the ASCII control characters except space, i.e. a character in the range `0x00` to `0x08`, or `0x0E` to `0x1F` or `0x7F`.
* *blank*: space ` ` or tab `\t`
* *newline*: newline `\n` or carriage return `\r`
* *other space*: form feed `\f` or vertical tab `\v`
* *digit*: ASCII digits `0` through `9`
* *lower*: ASCII lower case characters, `a` through `z`
* *upper*: ASCII upper case characters, `A` through `Z`
* *punct*: ASCII punctuation, i.e. one of ``!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~``.

Each category has a corresponding `is_XXX()` function.

## Compound ASCII Categories

These categories are overlapping, unlike the atomic categories:

* *space*: *blank* or *newline* or *other space*
* *alpha*: *lower* or *upper*
* *alnum*: *lower* or *upper* or *digit*
* *graph*: *lower* or *upper* or *digit* or *punct*
* *print*: *lower* or *upper* or *digit* or *punct* or space ` `

Each category has a corresponding `is_XXX()` function.


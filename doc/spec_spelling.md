# Header File `lex/spelling.hpp`

The file `spelling.hpp` contains the class `lex::token_spelling`, which is the (string) spelling of a token.

```cpp
class token_spelling
{
public:
    explicit token_spelling(const char* ptr, std::size_t size);
    
    // access
    char operator[](std::size_t i) const noexcept;
    
    const char* begin() const noexcept;
    const char* end() const noexcept;
    
    const char* data() const noexcept;
    std::size_t size() const noexcept;
};

// comparison
bool operator==(token_spelling lhs, token_spelling rhs);
bool operator!=(token_spelling lhs, token_spelling rhs);

bool operator==(token_spelling lhs, const char* rhs);
bool operator==(const char* lhs, token_spelling rhs);
bool operator!=(token_spelling lhs, const char* rhs);
bool operator!=(const char* lhs, token_spelling rhs);
```

It is a simple, fully `constexpr` and `noexcept` replacement of `std::string_view`.


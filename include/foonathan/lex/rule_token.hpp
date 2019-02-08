// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED

#include <foonathan/lex/match_result.hpp>
#include <foonathan/lex/token_spec.hpp>

namespace foonathan
{
namespace lex
{
    /// A token that has no associated parsing rule.
    ///
    /// It can only be created by some other [lex::rule_token]().
    struct null_token : detail::base_token
    {};

    /// Whether or not the given token is a null token.
    template <class Token>
    struct is_null_token : std::is_base_of<null_token, Token>
    {};

    /// A token that follows a complex parsing rule.
    ///
    /// It must provide a function `static match_result try_match(const char* cur, const char*
    /// end) noexcept;`. It is invoked with a pointer to the current character and to the end.
    /// There will always be at least one character.
    ///
    /// This function tries to parse a token and reports success or failure.
    /// If it didn't match anything, the next rule is tried.
    /// If it was an error, an error token is created.
    /// If it was a success, the correct token is created.
    ///
    /// It may create tokens of multiple other kinds if the parsing is related.
    /// The other tokens must then be [lex::null_token]() because they have no rules on their
    /// own.
    ///
    /// \notes The rules are tried in an arbitrary order so code should not depend on any
    /// particular ordering.
    /// If a literal token is a prefix of another rule, the `is_conflicting_literal()` function has
    /// to be implemented, informing about the conflicting literal.
    template <class Derived, class TokenSpec>
    struct basic_rule_token : detail::base_token
    {
        /// \exclude
        constexpr basic_rule_token()
        {
            static_assert(std::is_base_of<basic_rule_token, Derived>::value,
                          "invalid type for derived");
        }

        using spec         = TokenSpec;
        using token_kind   = lex::token_kind<TokenSpec>;
        using match_result = lex::match_result<TokenSpec>;

        /// \returns An unmatched result.
        static constexpr match_result unmatched() noexcept
        {
            return match_result::unmatched();
        }

        /// \returns An error result consuming the given number of characters.
        static constexpr match_result error(std::size_t bump) noexcept
        {
            return match_result::error(bump);
        }

        /// \returns A matched result creating the `Derived` token and consuming the given
        /// number of characters.
        template <typename Integer>
        static constexpr match_result success(Integer bump) noexcept
        {
            return match_result::success(token_kind(Derived{}), static_cast<std::size_t>(bump));
        }

        /// \returns A matched result creating some other null token and consuming the given
        /// number of characters.
        template <class Token, typename Integer>
        static constexpr match_result success(Integer bump) noexcept
        {
            return match_result::success(token_kind(Token{}), static_cast<std::size_t>(bump));
        }

        /// Whether or not the given literal token is conflicting with the rule.
        /// \returns Always `false`, but can be overriden by hiding this function.
        static constexpr bool is_conflicting_literal(token_kind) noexcept
        {
            return false;
        }
    };

    /// Whether or not the given token is a rule token.
    template <class Token>
    struct is_rule_token : detail::is_token_impl<basic_rule_token, Token>::value
    {};

    /// [PEG](https://en.wikipedia.org/wiki/Parsing_expression_grammar) combinators for specifying
    /// rules of complex tokens.
    ///
    /// A rule defined in this namespace checks for a certain combination of characters and consumes
    /// a certain amount if necessary. Multiple rules can then be combined to define the token.
    namespace token_rule
    {
        /// \exclude
        struct rule_base
        {};

        //=== atomic rules ===//
        namespace detail
        {
            template <class Rule>
            constexpr auto make_rule(Rule r) noexcept
                -> std::decay_t<decltype(r.try_match(std::declval<const char*&>(), nullptr), r)>
            {
                return r;
            }

            struct char_ : rule_base
            {
                char c;

                constexpr char_(char c) noexcept : c(c) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    if (cur != end && *cur == c)
                    {
                        ++cur;
                        return true;
                    }
                    else
                        return false;
                }
            };

            constexpr char_ make_rule(char c) noexcept
            {
                return {c};
            }

            struct string : rule_base
            {
                const char* str;
                std::size_t length;

                constexpr string(const char* str, std::size_t length) noexcept
                : str(str), length(length)
                {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    if (starts_with(cur, end))
                    {
                        cur += length;
                        return true;
                    }
                    else
                        return false;
                }

            private:
                constexpr bool starts_with(const char* cur, const char* end) const noexcept
                {
                    auto remaining = static_cast<std::size_t>(end - cur);
                    if (remaining < length)
                        return false;

                    auto str_cur = str;
                    auto str_end = str + length;
                    while (str_cur != str_end)
                    {
                        if (*cur != *str_cur)
                            return false;
                        ++cur;
                        ++str_cur;
                    }

                    return true;
                }
            };

            constexpr string make_rule(const char* str) noexcept
            {
                std::size_t length = 0;
                for (auto cur = str; *cur; ++cur)
                    ++length;

                return {str, length};
            }

            template <typename Predicate>
            struct ascii_predicate : rule_base
            {
                Predicate p;

                constexpr ascii_predicate(Predicate p) noexcept : p(p) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    if (cur != end && p(*cur))
                    {
                        ++cur;
                        return true;
                    }
                    else
                        return false;
                }
            };

            template <typename Predicate, typename = decltype(!std::declval<Predicate>()('\0'))>
            constexpr ascii_predicate<Predicate> make_rule(Predicate p) noexcept
            {
                return {p};
            }

            template <typename Function>
            struct function : rule_base
            {
                Function func;

                constexpr function(Function f) noexcept : func(f) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    auto result = func(cur, end);
                    cur += result;
                    return result > 0u;
                }
            };

            template <typename Function, typename = decltype(!std::declval<Function>()(
                                             std::declval<const char*&>(), nullptr))>
            constexpr function<Function> make_rule(Function f) noexcept
            {
                return {f};
            }

            template <class R>
            using rule_type = decltype(make_rule(std::declval<R>()));
        } // namespace detail

        /// Designates a primitive rule.
        ///
        /// An primitive rule is either:
        /// * A character, in which case that character is matched.
        /// * A C string, in which case that string is matched.
        /// * A predicate with the signature `bool(char)` which consumes one character if it returns
        ///   `true`. A [lex::ascii]() function is one example.
        /// * A callable with the signature `std::size_t(const char*, const char*)` which is invoked
        ///   with the current and end pointer, and returns the number of characters that are
        ///   consumed. The rule is considered matched if any characters are consumed.
        ///
        /// For consistency, it can also be invoked passing it any other rule.
        /// Using this function is only necessary if an operator overload is used and neither
        /// operands is a non-atomic rule. One operand has to be wrapped by this function, otherwise
        /// ADL is unable to find the operator overload.
        template <class R>
        constexpr detail::rule_type<R> r(R rule) noexcept
        {
            return detail::make_rule(rule);
        }

        namespace detail
        {
            template <std::size_t N>
            struct any : rule_base
            {
                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    auto remaining = end - cur;
                    if (static_cast<std::size_t>(remaining) < N)
                        return false;
                    else
                    {
                        cur += N;
                        return true;
                    }
                }
            };
        } // namespace detail

        /// If there are characters left, matches and consumes one.
        /// Otherwise, it doesn't match.
        constexpr detail::any<1> any = {};

        /// If there are `N` characters left, matches and consumes `N`.
        /// Otherwise, it doesn't match.
        template <std::size_t N>
        constexpr detail::any<N> skip = {};

        namespace detail
        {
            struct eof : rule_base
            {
                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    return cur == end;
                }
            };
        } // namespace detail

        /// Matches the end of input.
        constexpr detail::eof eof = {};

        namespace detail
        {
            struct fail : rule_base
            {
                constexpr bool try_match(const char*&, const char*) const noexcept
                {
                    return false;
                }
            };
        } // namespace detail

        /// Matches nothing.
        constexpr detail::fail fail = {};

        //=== combinators ===//
        namespace detail
        {
            template <class R1, class R2>
            struct sequence : rule_base
            {
                R1 r1;
                R2 r2;

                constexpr sequence(R1 r1, R2 r2) noexcept : r1(r1), r2(r2) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    auto copy = cur;

                    if (!r1.try_match(copy, end))
                        return false;
                    else if (!r2.try_match(copy, end))
                        return false;

                    cur = copy;
                    return true;
                }
            };
        } // namespace detail

        /// PEG sequence.
        ///
        /// Matches if both `r1` and `r2` match.
        /// If it matches, it consumes all characters consumed by `r1` and `r2`.
        /// Otherwise, it consumes nothing.
        template <class R1, class R2>
        constexpr auto operator+(R1 r1, R2 r2) noexcept
        {
            return detail::sequence<detail::rule_type<R1>, detail::rule_type<R2>>{detail::make_rule(
                                                                                      r1),
                                                                                  detail::make_rule(
                                                                                      r2)};
        }

        namespace detail
        {
            template <class R1, class R2>
            struct choice : rule_base
            {
                R1 r1;
                R2 r2;

                constexpr choice(R1 r1, R2 r2) noexcept : r1(r1), r2(r2) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    if (r1.try_match(cur, end))
                        return true;
                    else if (r2.try_match(cur, end))
                        return true;
                    else
                        return false;
                }
            };
        } // namespace detail

        /// PEG ordered choice.
        ///
        /// Tries to match and consume `r1`.
        /// If that fails, tries to match and consume `r2`.
        /// If that fails as well, doesn't match and consumes nothing.
        template <class R1, class R2>
        constexpr auto operator/(R1 r1, R2 r2) noexcept
        {
            return detail::choice<detail::rule_type<R1>, detail::rule_type<R2>>{detail::make_rule(
                                                                                    r1),
                                                                                detail::make_rule(
                                                                                    r2)};
        }

        namespace detail
        {
            template <class R>
            struct optional : rule_base
            {
                R r;

                constexpr optional(R r) noexcept : r(r) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    r.try_match(cur, end);
                    return true;
                }
            };
        } // namespace detail

        /// PEG optional.
        ///
        /// Tries to match and consume `r`.
        /// If that fails, matches anyway but doesn't consume anything.
        template <class R>
        constexpr auto opt(R r) noexcept
        {
            return detail::optional<detail::rule_type<R>>{detail::make_rule(r)};
        }

        namespace detail
        {
            template <class R>
            struct zero_or_more : rule_base
            {
                R r;

                constexpr zero_or_more(R r) noexcept : r(r) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    while (r.try_match(cur, end))
                    {
                    }
                    return true;
                }
            };
        } // namespace detail

        /// PEG zero-or-more.
        ///
        /// Tries to match and consume `r` as long as possible.
        /// Matches regardless of the number of times `r` was matched.
        template <class R>
        constexpr auto star(R r) noexcept
        {
            return detail::zero_or_more<detail::rule_type<R>>{detail::make_rule(r)};
        }

        /// PEG one-or-more.
        ///
        /// Tries to match and consume `r` as long as possible.
        /// Matches only if `r` was matched at least once.
        template <class R>
        constexpr auto plus(R r) noexcept
        {
            return r + star(r);
        }

        namespace detail
        {
            template <class R>
            struct lookahead : rule_base
            {
                R r;

                constexpr lookahead(R r) noexcept : r(r) {}

                constexpr bool try_match(const char* const& cur, const char* end) const noexcept
                {
                    auto dummy = cur;
                    return r.try_match(dummy, end);
                }
            };
        } // namespace detail

        /// PEG and predicate.
        ///
        /// Matches if `r` matched but does not consume any characters.
        /// \group lookahead
        template <class R>
        constexpr auto lookahead(R r) noexcept
        {
            return detail::lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }
        /// \group lookahead
        template <class R>
        constexpr auto operator&(R r) noexcept
        {
            return detail::lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }

        namespace detail
        {
            template <class R>
            struct neg_lookahead : rule_base
            {
                R r;

                constexpr neg_lookahead(R r) noexcept : r(r) {}

                constexpr bool try_match(const char* const& cur, const char* end) const noexcept
                {
                    auto dummy = cur;
                    return !r.try_match(dummy, end);
                }
            };
        } // namespace detail

        /// PEG not predicate.
        ///
        /// Matches if `r` didn't match but never consumes any characters.
        /// \group neg_lookahead
        template <class R>
        constexpr auto neg_lookahead(R r) noexcept
        {
            return detail::neg_lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }
        /// \group neg_lookahead
        template <class R>
        constexpr auto operator!(R r) noexcept
        {
            return detail::neg_lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }

        namespace detail
        {
            template <class R, std::size_t N>
            struct lookback : rule_base
            {
                R r;

                constexpr lookback(R r) noexcept : r(r) {}

                constexpr bool try_match(const char* const& cur, const char* end) const noexcept
                {
                    auto dummy = cur - N;
                    return r.try_match(dummy, end);
                }
            };
        } // namespace detail

        //=== convenience ===//
        namespace detail
        {
            template <class Rule, class Subtrahend>
            struct rule_minus
            {
                Rule       rule;
                Subtrahend sub;

                constexpr rule_minus(Rule rule, Subtrahend sub) noexcept : rule(rule), sub(sub) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    auto copy = cur;
                    if (!rule.try_match(copy, end))
                        // rule didn't match, so can't match either
                        return false;

                    auto sub_copy = cur;
                    if (sub.try_match(sub_copy, end))
                        // if sub matched, don't match
                        return false;

                    cur = copy;
                    return true;
                }
            };
        } // namespace detail

        /// Rule minus operation.
        ///
        /// Only matches if `rule` matches and `sub` didn't match the character sequence rule
        /// matched. For example, to create a non-zero digit rule use `minus(is_digit, '0')`.
        template <class Rule, class Subtrahend>
        constexpr auto minus(Rule rule, Subtrahend sub) noexcept
        {
            return detail::rule_minus<detail::rule_type<Rule>,
                                      detail::rule_type<Subtrahend>>{detail::make_rule(rule),
                                                                     detail::make_rule(sub)};
        }
        /// If `condition` matches, matches `then` as well.
        /// If `condition` doesn't match, matches `otherwise`.
        ///
        /// Equivalent to `(condition + then) / (!condition + otherwise)`.
        template <class Condition, class Then, class Otherwise>
        constexpr auto if_then_else(Condition condition, Then then, Otherwise otherwise) noexcept
        {
            return (r(condition) + then) / (!r(condition) + otherwise);
        }

        /// Matches `until` until `end` is matched, then matches `end`.
        ///
        /// Equivalent to `star(!end + until) + end`.
        template <class End, class Until = detail::any<1>>
        constexpr auto until(End end, Until until = any) noexcept
        {
            return star(!r(end) + until) + end;
        }

        /// Matches `until` until `end` is matched, then does not match `end`.
        /// `end` must still follow aftwards, however.
        ///
        /// Equivalent to `star(!end + until) + lookahead(end)`.
        template <class End, class Until = detail::any<1>>
        constexpr auto until_excluding(End end, Until until = any) noexcept
        {
            return star(!r(end) + until) + lookahead(end);
        }

        /// Matches a non-empty list of `element`s separated by `separator`.
        /// Does not allow a trailing separator.
        ///
        /// Equivalent to `element + star(separator + element)`.
        template <class Element, class Seperator>
        constexpr auto list(Element element, Seperator separator) noexcept
        {
            return element + star(r(separator) + element);
        }

        /// Matches a non-empty list of `element`s separated by `separator`.
        /// Does allow a trailing separator.
        ///
        /// Equivalent to `list(element, separator) + opt(separator)`.
        template <class Element, class Separator>
        constexpr auto list_trailing(Element element, Separator separator) noexcept
        {
            return list(element, separator) + opt(separator);
        }

        /// Matches `rule` with optional padding `left` and `right`.
        ///
        /// Equivalent to `star(left) + rule + star(right)`.
        template <class PaddingLeft, class Rule, class PaddingRight>
        constexpr auto opt_padded(PaddingLeft left, Rule rule, PaddingRight right) noexcept
        {
            return star(left) + rule + star(right);
        }

        /// Matches `rule` with padding `left` and `right`,
        /// where there must be some padding on at least one side.
        ///
        /// Equivalent to `(plus(left) + rule + star(right)) / `(rule + plus(right))`.
        template <class PaddingLeft, class Rule, class PaddingRight>
        constexpr auto padded(PaddingLeft left, Rule rule, PaddingRight right) noexcept
        {
            return (plus(left) + rule + star(right)) / (rule + plus(right));
        }

        namespace detail
        {
            template <std::size_t Min, std::size_t Max, class Rule>
            struct repeated : rule_base
            {
                Rule rule;

                constexpr repeated(Rule rule) noexcept : rule(rule) {}

                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    auto copy = cur;

                    for (auto i = std::size_t(0); i != Min; ++i)
                        if (!rule.try_match(copy, end))
                            // didn't match if rule didn't occur at least min times
                            return false;
                    // rule was matched at least Min times

                    for (auto i = Min; i <= Max; ++i)
                        if (!rule.try_match(copy, end))
                        {
                            // rule was matched not more than Max, success
                            cur = copy;
                            return true;
                        }

                    // now rule must not match any more
                    return !rule.try_match(copy, end);
                }
            };
        } // namespace detail

        /// Matches if `rule` occurs at least `Min` times and at most `Max` times.
        /// Does not match if `rule` occurs less than `Min` or more than `Max`.
        ///
        /// Equivalent to `(rule + ... + rule) + (opt(rule) + ... + opt(rule)) + !rule`,
        /// where the first expression contains `Min` repetitions of `rule` and the second `Max -
        /// Min` repetitions of `opt(rule)`.
        template <std::size_t Min, std::size_t Max, class Rule>
        constexpr auto repeated(Rule rule) noexcept
        {
            static_assert(Min <= Max, "invalid range");
            static_assert(Max > 0, "empty range");
            return detail::repeated<Min, Max, detail::rule_type<Rule>>(rule);
        }

        /// Matches if `rule` occurs exactly `N` times.
        ///
        /// Equivalent to `repeated<N, N>(rule)`.
        template <std::size_t N, class Rule>
        constexpr auto times(Rule rule) noexcept
        {
            return repeated<N, N>(rule);
        }

        /// Matches if `rule` occurs at most `N` times.
        ///
        /// Equivalent to `repeated<0, N>(rule)`.
        template <std::size_t N, class Rule>
        constexpr auto at_most(Rule rule) noexcept
        {
            return repeated<0, N>(rule);
        }

        /// Matches if `rule` occurs at least `N` times.
        ///
        /// Equivalent to `repeated<N, infinity>(rule)`.
        template <std::size_t N, class Rule>
        constexpr auto at_least(Rule rule) noexcept
        {
            return repeated<N, std::size_t(-1)>(rule);
        }
    } // namespace token_rule

    /// Matches a [lex::token_rule]().
    template <class TokenSpec>
    class rule_matcher
    {
    public:
        /// \effects Creates it passing it the string to be matched.
        explicit constexpr rule_matcher(const char* str, const char* end) noexcept
        : begin_(str), cur_(str), end_(end)
        {}

        /// \returns Whether or not the rule would match at the current position.
        template <class Rule>
        constexpr bool peek(Rule rule) const noexcept
        {
            auto copy = cur_;
            return token_rule::r(rule).try_match(copy, end_);
        }

        /// \effects Consumes the characters consumed by matching the rule at the current position.
        /// \returns Whether the rule matched at the current position.
        template <class Rule>
        constexpr bool match(Rule rule) noexcept
        {
            return token_rule::r(rule).try_match(cur_, end_);
        }

        /// \effects Matches the rule at the current position.
        /// \returns If the rule matched and in total a non-zero amount of characters were consumed,
        /// returns a success [lex::match_result]() for the given kind.
        /// If the rule didn't match and a non-zero amount of characters were consumed,
        /// returns an error result.
        /// Otherwise, if the rule didn't match but no characters were consumed,
        /// returns an unmatched result.
        template <class Rule>
        constexpr match_result<TokenSpec> finish(token_kind<TokenSpec> kind, Rule rule) noexcept
        {
            if (match(rule) && get_bump() > 0)
                return match_result<TokenSpec>::success(kind, get_bump());
            else if (get_bump() > 0)
                return match_result<TokenSpec>::error(get_bump());
            else
                return match_result<TokenSpec>::unmatched();
        }

    private:
        constexpr std::size_t get_bump() const noexcept
        {
            return static_cast<std::size_t>(cur_ - begin_);
        }

        const char* begin_;
        const char* cur_;
        const char* end_;
    };

    /// A token that follows a PEG parsing rule.
    ///
    /// It must provide a function `static constexpr auto rule()` which returns a
    /// [lex::token_rule]() object. It matches a token if the rule matches a non-zero amount of
    /// characters.
    template <class Derived, class TokenSpec>
    struct rule_token : basic_rule_token<Derived, TokenSpec>
    {
        static constexpr lex::match_result<TokenSpec> try_match(const char* str,
                                                                const char* end) noexcept
        {
            constexpr auto rule = Derived::rule();
            return rule_matcher<TokenSpec>(str, end).finish(Derived{}, rule);
        }
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED

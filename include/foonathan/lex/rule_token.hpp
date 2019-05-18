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
    template <class TokenSpec>
    class token;
    template <class TokenSpec>
    class rule_matcher;

    struct null_token : detail::base_token
    {};

    template <class Token>
    struct is_null_token : std::is_base_of<null_token, Token>
    {};

    template <class Derived, class TokenSpec>
    struct basic_rule_token : detail::base_token
    {
        constexpr basic_rule_token()
        {
            static_assert(std::is_base_of<basic_rule_token, Derived>::value,
                          "invalid type for derived");
        }

        using spec         = TokenSpec;
        using token_kind   = lex::token_kind<TokenSpec>;
        using token        = lex::token<TokenSpec>;
        using match_result = lex::match_result<TokenSpec>;
        using rule_matcher = lex::rule_matcher<TokenSpec>;

        static constexpr match_result unmatched() noexcept
        {
            return match_result::unmatched();
        }

        static constexpr match_result error(std::size_t bump) noexcept
        {
            return match_result::error(bump);
        }

        template <typename Integer>
        static constexpr match_result success(Integer bump) noexcept
        {
            return match_result::success(token_kind(Derived{}), static_cast<std::size_t>(bump));
        }

        template <class Token, typename Integer>
        static constexpr match_result success(Integer bump) noexcept
        {
            return match_result::success(token_kind(Token{}), static_cast<std::size_t>(bump));
        }

        static constexpr bool is_conflicting_literal(token_kind) noexcept
        {
            return false;
        }
    };

    template <class Token>
    struct is_rule_token : detail::is_token_impl<basic_rule_token, Token>::value
    {};

    namespace token_rule
    {
        struct base_rule
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

            struct char_ : base_rule
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

            struct string : base_rule
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
            struct ascii_predicate : base_rule
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
            struct function : base_rule
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

        template <class R>
        constexpr detail::rule_type<R> r(R rule) noexcept
        {
            return detail::make_rule(rule);
        }

        namespace detail
        {
            template <std::size_t N>
            struct any : base_rule
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

        constexpr detail::any<1> any = {};

        template <std::size_t N>
        constexpr detail::any<N> skip = {};

        namespace detail
        {
            struct eof : base_rule
            {
                constexpr bool try_match(const char*& cur, const char* end) const noexcept
                {
                    return cur == end;
                }
            };
        } // namespace detail

        constexpr detail::eof eof = {};

        namespace detail
        {
            struct fail : base_rule
            {
                constexpr bool try_match(const char*&, const char*) const noexcept
                {
                    return false;
                }
            };
        } // namespace detail

        constexpr detail::fail fail = {};

        //=== combinators ===//
        namespace detail
        {
            template <class R1, class R2>
            struct sequence : base_rule
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
            struct choice : base_rule
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
            struct optional : base_rule
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

        template <class R>
        constexpr auto opt(R r) noexcept
        {
            return detail::optional<detail::rule_type<R>>{detail::make_rule(r)};
        }

        namespace detail
        {
            template <class R>
            struct zero_or_more : base_rule
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

        template <class R>
        constexpr auto star(R r) noexcept
        {
            return detail::zero_or_more<detail::rule_type<R>>{detail::make_rule(r)};
        }

        template <class R>
        constexpr auto plus(R r) noexcept
        {
            return r + star(r);
        }

        namespace detail
        {
            template <class R>
            struct lookahead : base_rule
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

        template <class R>
        constexpr auto lookahead(R r) noexcept
        {
            return detail::lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }

        template <class R>
        constexpr auto operator&(R r) noexcept
        {
            return detail::lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }

        namespace detail
        {
            template <class R>
            struct neg_lookahead : base_rule
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

        template <class R>
        constexpr auto neg_lookahead(R r) noexcept
        {
            return detail::neg_lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }

        template <class R>
        constexpr auto operator!(R r) noexcept
        {
            return detail::neg_lookahead<detail::rule_type<R>>{detail::make_rule(r)};
        }

        namespace detail
        {
            template <class R, std::size_t N>
            struct lookback : base_rule
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
                    if (sub.try_match(sub_copy, copy))
                        // if sub matched, don't match
                        return false;

                    cur = copy;
                    return true;
                }
            };
        } // namespace detail

        template <class Rule, class Subtrahend>
        constexpr auto minus(Rule rule, Subtrahend sub) noexcept
        {
            return detail::rule_minus<detail::rule_type<Rule>,
                                      detail::rule_type<Subtrahend>>{detail::make_rule(rule),
                                                                     detail::make_rule(sub)};
        }

        template <class Condition, class Then, class Otherwise>
        constexpr auto if_then_else(Condition condition, Then then, Otherwise otherwise) noexcept
        {
            return (r(condition) + then) / (!r(condition) + otherwise);
        }

        template <class End, class Until = detail::any<1>>
        constexpr auto until(End end, Until until = any) noexcept
        {
            return star(!r(end) + until) + end;
        }

        template <class End, class Until = detail::any<1>>
        constexpr auto until_excluding(End end, Until until = any) noexcept
        {
            return star(!r(end) + until) + lookahead(end);
        }

        template <class Element, class Seperator>
        constexpr auto list(Element element, Seperator separator) noexcept
        {
            return element + star(r(separator) + element);
        }

        template <class Element, class Separator>
        constexpr auto list_trailing(Element element, Separator separator) noexcept
        {
            return list(element, separator) + opt(separator);
        }

        template <class PaddingLeft, class Rule, class PaddingRight>
        constexpr auto opt_padded(PaddingLeft left, Rule rule, PaddingRight right) noexcept
        {
            return star(left) + rule + star(right);
        }

        template <class PaddingLeft, class Rule, class PaddingRight>
        constexpr auto padded(PaddingLeft left, Rule rule, PaddingRight right) noexcept
        {
            return (plus(left) + rule + star(right)) / (rule + plus(right));
        }

        namespace detail
        {
            template <std::size_t Min, std::size_t Max, class Rule>
            struct repeated : base_rule
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

        template <std::size_t Min, std::size_t Max, class Rule>
        constexpr auto repeated(Rule rule) noexcept
        {
            static_assert(Min <= Max, "invalid range");
            static_assert(Max > 0, "empty range");
            return detail::repeated<Min, Max, detail::rule_type<Rule>>(rule);
        }

        template <std::size_t N, class Rule>
        constexpr auto times(Rule rule) noexcept
        {
            return repeated<N, N>(rule);
        }

        template <std::size_t N, class Rule>
        constexpr auto at_most(Rule rule) noexcept
        {
            return repeated<0, N>(rule);
        }

        template <std::size_t N, class Rule>
        constexpr auto at_least(Rule rule) noexcept
        {
            return repeated<N, std::size_t(-1)>(rule);
        }
    } // namespace token_rule

    template <class TokenSpec>
    class rule_matcher
    {
    public:
        explicit constexpr rule_matcher(const char* str, const char* end) noexcept
        : begin_(str), cur_(str), end_(end)
        {}

        template <class Rule>
        constexpr bool peek(Rule rule) const noexcept
        {
            auto copy = cur_;
            return token_rule::r(rule).try_match(copy, end_);
        }

        template <class Rule>
        constexpr bool match(Rule rule) noexcept
        {
            return token_rule::r(rule).try_match(cur_, end_);
        }

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

        constexpr match_result<TokenSpec> finish(token_kind<TokenSpec> kind) noexcept
        {
            if (get_bump() > 0)
                return match_result<TokenSpec>::success(kind, get_bump());
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

    template <class Derived, class TokenSpec>
    struct rule_token : basic_rule_token<Derived, TokenSpec>
    {
        static constexpr lex::match_result<TokenSpec> try_match(const char* str,
                                                                const char* end) noexcept
        {
            constexpr auto rule = Derived::rule();
            return lex::rule_matcher<TokenSpec>(str, end).finish(Derived{}, rule);
        }
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED

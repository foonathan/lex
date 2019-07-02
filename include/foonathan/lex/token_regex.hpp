// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_REGEX_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_REGEX_HPP_INCLUDED

#include <boost/mp11/utility.hpp>
#include <type_traits>

#include <foonathan/lex/detail/assert.hpp>
#include <foonathan/lex/detail/string.hpp>
#include <foonathan/lex/token_spec.hpp>

namespace foonathan
{
namespace lex
{
    namespace token_regex
    {
        struct regex
        {};

        namespace detail
        {
            template <class E1, class E2, typename Enable = void>
            struct common_prefix;

            template <class E, class Tokenizer>
            constexpr bool match(Tokenizer& tokenizer)
            {
                if (E::peek(tokenizer))
                    return E::bump(tokenizer);
                else
                    return false;
            }

            //=== epsilon ===//
            // matches the empty string
            struct epsilon : regex
            {
                template <class Tokenizer>
                static constexpr bool peek(const Tokenizer&)
                {
                    return true;
                }

                template <class Tokenizer>
                static constexpr bool bump(Tokenizer&)
                {
                    return true;
                }

                template <typename T>
                static constexpr auto description(T)
                {
                    return lex::detail::make_string("ε");
                }
            };

            // matches nothing
            struct empty_set : regex
            {
                template <class Tokenizer>
                static constexpr bool peek(const Tokenizer&)
                {
                    return false;
                }

                template <class Tokenizer>
                static constexpr bool bump(Tokenizer&)
                {
                    return false;
                }

                template <typename T>
                static constexpr auto description(T)
                {
                    return lex::detail::make_string("null");
                }
            };

            //=== atom ===//
            // the single token
            template <class Token>
            struct atom : regex
            {
                static_assert(is_token<Token>::value, "can only use a token in this context");

                template <class Tokenizer>
                static constexpr bool peek(const Tokenizer& tokenizer)
                {
                    return tokenizer.peek().is(Token{});
                }

                template <class Tokenizer>
                static constexpr bool bump(Tokenizer& tokenizer)
                {
                    tokenizer.bump();
                    return true;
                }

                template <typename T>
                static constexpr auto description(T)
                {
                    return lex::detail::make_string<&Token::name>();
                }
            };

            template <class E>
            using make_atom = boost::mp11::mp_eval_if<std::is_base_of<regex, E>, E, atom, E>;

            //=== sequence ===//
            // A then B
            // only recursion on the right allowed
            template <class A, class B>
            struct sequence : regex
            {
                static_assert(std::is_base_of<regex, A>::value && std::is_base_of<regex, B>::value,
                              "invalid type in regex");

                template <class Tokenizer>
                static constexpr bool peek(const Tokenizer& tokenizer)
                {
                    return A::peek(tokenizer);
                }

                template <class Tokenizer>
                static constexpr bool bump(Tokenizer& tokenizer)
                {
                    return A::bump(tokenizer) && match<B>(tokenizer);
                }

                template <typename T>
                static constexpr auto description(T)
                {
                    return A::description(std::false_type{}) + B::description(std::false_type{});
                }
            };

            template <typename T>
            struct is_sequence : std::false_type
            {};
            template <class A, class B>
            struct is_sequence<sequence<A, B>> : std::true_type
            {};

            template <class A, class B>
            struct make_seq_impl
            {
                using type = sequence<A, B>;
            };
            template <class A, class B>
            using make_sequence = typename make_seq_impl<A, B>::type;

            // special case: left recursion
            template <class AA, class AB, class B>
            struct make_seq_impl<sequence<AA, AB>, B>
            {
                using type = make_sequence<AA, make_sequence<AB, B>>;
            };
            template <class AA, class AB>
            struct make_seq_impl<sequence<AA, AB>, epsilon>
            {
                using type = make_sequence<AA, AB>;
            };
            // special cases: epsilon
            template <class B>
            struct make_seq_impl<epsilon, B>
            {
                using type = B;
            };
            template <class A>
            struct make_seq_impl<A, epsilon>
            {
                using type = A;
            };
            // specialization for <choice<AA, AB>, B> below
            // specialization for <star<A>, B> below

            //=== star ===//
            // zero-or-more
            template <class E>
            struct star : regex
            {
                static_assert(std::is_base_of<regex, E>::value, "invalid type in regex");

                template <class Tokenizer>
                static constexpr bool peek(const Tokenizer&)
                {
                    return true;
                }

                template <class Tokenizer>
                static constexpr bool bump(Tokenizer& tokenizer)
                {
                    // consume as often as possible
                    while (E::peek(tokenizer))
                    {
                        if (!E::bump(tokenizer))
                            return false;
                    }

                    return true;
                }

                template <typename T>
                static constexpr auto description(T)
                {
                    return lex::detail::make_string("(") + E::description(std::true_type{})
                           + lex::detail::make_string(")*");
                }
            };

            template <typename T>
            struct is_star : std::false_type
            {};
            template <class E>
            struct is_star<star<E>> : std::true_type
            {};

            template <class E>
            struct make_star_impl
            {
                using type = star<E>;
            };
            template <class E>
            using make_star = typename make_star_impl<E>::type;

            // special case: recursion
            template <class E>
            struct make_star_impl<star<E>>
            {
                using type = star<E>;
            };
            // special case: epsilon, empty_set
            template <>
            struct make_star_impl<epsilon>
            {
                using type = epsilon;
            };
            template <>
            struct make_star_impl<empty_set>
            {
                using type = empty_set;
            };

            template <class A, class B>
            struct make_seq_impl<star<A>, B>
            {
                using common = common_prefix<A, B>;
                static_assert(std::is_same<typename common::prefix, epsilon>::value,
                              "greedy star will never leave something over for rest");

                using type = sequence<star<A>, B>;
            };

            //=== choice ===//
            struct choice_base : regex
            {};
            // A or B, which must not share a common prefix
            // only recursion on the right allowed
            template <class A, class B>
            struct choice : choice_base
            {
                static_assert(std::is_base_of<regex, A>::value && std::is_base_of<regex, B>::value,
                              "invalid type in regex");

                template <class Tokenizer>
                static constexpr bool peek(const Tokenizer& tokenizer)
                {
                    return A::peek(tokenizer) || B::peek(tokenizer);
                }

                template <class Tokenizer>
                static constexpr bool bump(Tokenizer& tokenizer)
                {
                    if (A::peek(tokenizer))
                        return A::bump(tokenizer);
                    else
                        return match<B>(tokenizer);
                }

                static constexpr auto description(std::true_type /* top_level */)
                {
                    return A::description(std::false_type{}) + lex::detail::make_string("|")
                           + B::description(std::is_base_of<choice_base, B>{});
                }
                static constexpr auto description(std::false_type /* top_level */)
                {
                    return lex::detail::make_string("(") + A::description(std::false_type{})
                           + lex::detail::make_string("|")
                           + B::description(std::is_base_of<choice_base, B>{})
                           + lex::detail::make_string(")");
                }
            };
            template <class B>
            struct choice<epsilon, B> : choice_base
            {
                static_assert(std::is_base_of<regex, B>::value, "invalid type in regex");

                template <class Tokenizer>
                static constexpr bool peek(const Tokenizer&)
                {
                    return true;
                }

                template <class Tokenizer>
                static constexpr bool bump(Tokenizer& tokenizer)
                {
                    // consume the tokens of B, but no matter what, it matches
                    match<B>(tokenizer);
                    return true;
                }

                template <typename T>
                static constexpr auto description(T)
                {
                    return lex::detail::make_string("(") + B::description(std::true_type{})
                           + lex::detail::make_string(")?");
                }
            };
            template <class A>
            struct choice<A, epsilon> : choice<epsilon, A>
            {};
            template <class B>
            struct choice<empty_set, B> : B
            {};
            template <class A>
            struct choice<A, empty_set> : A
            {};

            template <typename T>
            struct is_choice : std::false_type
            {};
            template <class A, class B>
            struct is_choice<choice<A, B>> : std::true_type
            {};

            template <class A, class B>
            struct make_choice_impl
            {
                using common = common_prefix<A, B>;

                template <class T, bool Prefix>
                struct impl;
                // no common prefix
                template <class T>
                struct impl<T, false>
                {
                    using type = choice<A, B>;
                };
                // common prefix
                template <class T>
                struct impl<T, true>
                {
                    // need the prefix, then the choice of the rest
                    using if_prefix = make_sequence<
                        typename T::prefix,
                        typename make_choice_impl<typename T::tail1, typename T::tail2>::type>;

                    // otherwise, choice of the two elses
                    using else_ =
                        typename make_choice_impl<typename T::else1, typename T::else2>::type;

                    using type = typename make_choice_impl<if_prefix, else_>::type;
                };

                using select = impl<common, !std::is_same<typename common::prefix, epsilon>::value>;
                using type   = typename select::type;
            };
            template <class A, class B>
            using make_choice = typename make_choice_impl<A, B>::type;

            // special case: left recursion
            template <class AA, class AB, class B>
            struct make_choice_impl<choice<AA, AB>, B>
            {
                using type = make_choice<AA, make_choice<AB, B>>;
            };
            // special case: the same type
            template <class A>
            struct make_choice_impl<A, A>
            {
                using type = A;
            };

            // move choice out of sequence
            template <class AA, class AB, class B>
            struct make_seq_impl<choice<AA, AB>, B>
            {
                using type = make_choice<make_sequence<AA, B>, make_sequence<AB, B>>;
            };

            //=== common_prefix ===//
            // * prefix: the common prefix of two expressions, or epsilon if there is none
            // * tail1: what still needs to be matched of E1 if prefix matched
            // * tail2: what still needs to be matched of E2 if prefix matched
            // * else1: what needs to be matched of E1 if prefix didn't match (usually E1)
            // * else2: what needs to be matched of E2 if prefix didn't match (usually E2)

            // fallback: reverse arguments
            template <class E1, class E2, typename Enable>
            struct common_prefix : common_prefix<E2, E1>
            {};

            // special case: same type
            template <class E>
            struct common_prefix<E, E, void>
            {
                using prefix = E;

                using tail1 = epsilon;
                using tail2 = epsilon;

                using else1 = empty_set;
                using else2 = empty_set;
            };

            // epsilon and anything: no prefix
            template <class E2>
            struct common_prefix<epsilon, E2, void>
            {
                using prefix = epsilon;

                using tail1 = epsilon;
                using tail2 = E2;

                using else1 = empty_set;
                using else2 = E2;
            };

            // empty set and anything: no prefix
            template <class E2>
            struct common_prefix<empty_set, E2, void>
            {
                using prefix = epsilon;

                using tail1 = empty_set;
                using tail2 = E2;

                using else1 = empty_set;
                using else2 = E2;
            };

            // atom and atom
            template <class T1, class T2>
            struct common_prefix<atom<T1>, atom<T2>, std::enable_if_t<!std::is_same<T1, T2>::value>>
            {
                using prefix = epsilon;

                using tail1 = atom<T1>;
                using tail2 = atom<T2>;

                using else1 = atom<T1>;
                using else2 = atom<T2>;
            };

            // atom and sequence
            template <class T1, class A2, class B2>
            struct common_prefix<atom<T1>, sequence<A2, B2>, void>
            {
                using common = common_prefix<atom<T1>, A2>;

                template <class T, bool Prefix>
                struct impl;
                // no common prefix
                template <class T>
                struct impl<T, false>
                {
                    using prefix = epsilon;

                    using tail1 = atom<T1>;
                    using tail2 = sequence<A2, B2>;

                    using else1 = atom<T1>;
                    using else2 = sequence<A2, B2>;
                };
                // common prefix
                template <class T>
                struct impl<T, true>
                {
                    static_assert(std::is_same<typename T::prefix, atom<T1>>::value,
                                  "has to be entire atom");

                    // it has to be the entire atom
                    using prefix = atom<T1>;

                    // so nothing left for the atom
                    using tail1 = epsilon;
                    // and remainder of sequence
                    using tail2 = make_sequence<typename common::tail2, B2>;

                    // impossible to have the atom or sequence
                    using else1 = empty_set;
                    using else2 = empty_set;
                };

                using select = impl<common, !std::is_same<typename common::prefix, epsilon>::value>;
                using prefix = typename select::prefix;
                using tail1  = typename select::tail1;
                using tail2  = typename select::tail2;
                using else1  = typename select::else1;
                using else2  = typename select::else2;
            };

            // sequence and sequence: base case where the heads are different
            template <class A1, class B1, class A2, class B2>
            struct common_prefix<sequence<A1, B1>, sequence<A2, B2>,
                                 std::enable_if_t<!std::is_same<A1, A2>::value>>
            {
                using common = common_prefix<A1, A2>;
                static_assert(!std::is_same<typename common::prefix, A1>::value
                                  || !std::is_same<typename common::prefix, A2>::value,
                              "A1 and A2 are the same type?!");

                template <class T, bool Prefix>
                struct impl;
                // no common prefix between the heads
                template <class T>
                struct impl<T, false>
                {
                    using prefix = epsilon;

                    using tail1 = sequence<A1, B1>;
                    using tail2 = sequence<A2, B2>;

                    using else1 = sequence<A1, B1>;
                    using else2 = sequence<A2, B2>;
                };
                // common prefix between the heads
                template <class T>
                struct impl<T, true>
                {
                    // as the heads are not the same type,
                    // this is the longest prefix
                    using prefix = typename common::prefix;

                    // still need rest of the sequences
                    using tail1 = make_sequence<typename common::tail1, B1>;
                    using tail2 = make_sequence<typename common::tail2, B2>;

                    // impossible otherwise
                    using else1 = empty_set;
                    using else2 = empty_set;
                };

                using select = impl<common, std::is_same<typename common::prefix, epsilon>::value>;
                using prefix = typename select::prefix;
                using tail1  = typename select::tail1;
                using tail2  = typename select::tail2;
                using else1  = typename select::else1;
                using else2  = typename select::else2;
            };
            // sequence and sequence: same head
            template <class A, class B1, class B2>
            struct common_prefix<sequence<A, B1>, sequence<A, B2>,
                                 std::enable_if_t<!std::is_same<B1, B2>::value>>
            {
                using tail = common_prefix<B1, B2>;

                // the prefix is A and then the prefix of tail
                using prefix = make_sequence<A, typename tail::prefix>;

                // if it matches we only need the respective tails
                using tail1 = typename tail::tail1;
                using tail2 = typename tail::tail2;

                // if it didn't match its impossible
                using else1 = empty_set;
                using else2 = empty_set;
            };

            // star and atom: atom is not the starred expression
            template <class E, class T>
            struct common_prefix<star<E>, atom<T>, void>
            {
                using prefix = epsilon;

                using tail1 = star<E>;
                using tail2 = atom<T>;

                using else1 = star<E>;
                using else2 = atom<T>;
            };
            // star and atom: atom is the starred expression
            template <class T>
            struct common_prefix<star<atom<T>>, atom<T>, void>
            {
                using prefix = atom<T>;

                using tail1 = star<atom<T>>;
                using tail2 = epsilon;

                using else1 = epsilon;
                using else2 = empty_set;
            };

            // star and sequence
            template <class E, class A, class B>
            struct common_prefix<star<E>, sequence<A, B>>
            {
                using common = common_prefix<E, sequence<A, B>>;

                template <class T, bool Prefix, bool Same>
                struct impl;
                // no common prefix
                template <class T>
                struct impl<T, false, false>
                {
                    using prefix = epsilon;

                    using tail1 = star<E>;
                    using tail2 = sequence<A, B>;

                    using else1 = star<E>;
                    using else2 = sequence<A, B>;
                };
                // common prefix but not the same
                template <class T>
                struct impl<T, true, false>
                {
                    using prefix = typename common::prefix;

                    // we need the rest of E and then everything again
                    using tail1 = make_sequence<typename common::tail1, star<E>>;
                    // just the tail of the sequence
                    using tail2 = typename common::tail2;

                    // if the prefix didn't match, star succeeds anyway
                    using else1 = epsilon;
                    // but the sequence doesn't
                    using else2 = empty_set;
                };
                // common prefix that is E
                template <class T>
                struct impl<T, true, true>
                {
                    // see whether we can do it again on the tail
                    using recurse = common_prefix<star<E>, typename common::tail2>;

                    // the prefix is E and then whatever was recursed
                    using prefix = make_sequence<E, typename recurse::prefix>;

                    // we still need star<E>
                    using tail1 = star<E>;
                    // we still need the tail of recurse
                    using tail2 = typename recurse::tail2;

                    // if the prefix didn't match, star succeeds anyway
                    using else1 = epsilon;
                    // but the sequence doesn't
                    using else2 = empty_set;
                };

                using select = impl<common, std::is_same<typename common::prefix, epsilon>::value,
                                    std::is_same<typename common::prefix, E>::value>;
                using prefix = typename select::prefix;
                using tail1  = typename select::tail1;
                using tail2  = typename select::tail2;
                using else1  = typename select::else1;
                using else2  = typename select::else2;
            };

            // choice and anything
            template <class A1, class B1, class B>
            struct common_prefix<choice<A1, B1>, B, void>
            {
                using common1 = common_prefix<A1, B>;
                using common2 = common_prefix<B1, B>;

                template <class T, bool Prefix1, bool Prefix2>
                struct impl;
                // no common prefixes with either case
                template <class T>
                struct impl<T, false, false>
                {
                    using prefix = epsilon;

                    using tail1 = choice<A1, B1>;
                    using tail2 = B;

                    using else1 = choice<A1, B1>;
                    using else2 = B;
                };
                // common prefix with first choice only
                template <class T>
                struct impl<T, true, false>
                {
                    // this is the common prefix
                    using prefix = typename common1::prefix;

                    // if it matches we still need the rest of the first choice
                    using tail1 = typename common1::tail1;
                    // if it matches we still need the rest of the other one
                    using tail2 = typename common1::tail2;

                    // if it didn't match we still have the other alternative and else
                    using else1 = make_choice<typename common1::else1, B1>;
                    // if it dind't match we still have the else
                    using else2 = typename common1::else2;
                };
                // common prefix with second choice only
                template <class T>
                struct impl<T, false, true>
                {
                    // this is the common prefix
                    using prefix = typename common2::prefix;

                    // if it matches we still need the rest of the second choice
                    using tail1 = typename common2::tail1;
                    // if it matches we still need the rest of the other one
                    using tail2 = typename common2::tail2;

                    // if it didn't match we still have the other alternative and else
                    using else1 = make_choice<typename common2::else1, A1>;
                    // if it didn't match we still have the else
                    using else2 = typename common2::else2;
                };
                // common prefix with both choices
                // this means the choice itself has a common prefix,
                // but this is outruled by choice!
                template <class T>
                struct impl<T, true, true>;

                using select = impl<void, !std::is_same<typename common1::prefix, epsilon>::value,
                                    !std::is_same<typename common2::prefix, epsilon>::value>;
                using prefix = typename select::prefix;
                using tail1  = typename select::tail1;
                using tail2  = typename select::tail2;
                using else1  = typename select::else1;
                using else2  = typename select::else2;
            };
        } // namespace detail

        template <class E1, class E2>
        constexpr auto operator+(E1, E2)
        {
            return detail::make_sequence<detail::make_atom<E1>, detail::make_atom<E2>>{};
        }

        template <class E1, class E2>
        constexpr auto operator/(E1, E2)
        {
            return detail::make_choice<detail::make_atom<E1>, detail::make_atom<E2>>{};
        }

        template <class E>
        constexpr auto opt(E)
        {
            return detail::choice<detail::epsilon, detail::make_atom<E>>{};
        }

        template <class E>
        constexpr auto star(E)
        {
            return detail::make_star<detail::make_atom<E>>{};
        }
        template <class E>
        constexpr auto plus(E)
        {
            using e = detail::make_atom<E>;
            return detail::make_sequence<e, detail::make_star<e>>{};
        }
    } // namespace token_regex

    template <class Regex>
    struct is_token_regex
    : std::is_base_of<token_regex::regex, token_regex::detail::make_atom<Regex>>
    {};

    namespace detail
    {
        template <class Regex>
        constexpr auto regex_description = Regex::description(std::true_type{});
    } // namespace detail

    template <class Regex>
    constexpr const char* regex_description(Regex)
    {
        using regex = token_regex::detail::make_atom<Regex>;
        return detail::regex_description<regex>.array;
    }

    template <class Tokenizer, class Regex>
    constexpr bool regex_match(Tokenizer tokenizer, Regex)
    {
        using regex = token_regex::detail::make_atom<Regex>;
        static_assert(std::is_base_of<token_regex::regex, regex>::value, "not a regex");

        if (token_regex::detail::match<regex>(tokenizer))
            return tokenizer.is_done();
        else
            return false;
    }
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_REGEX_HPP_INCLUDED

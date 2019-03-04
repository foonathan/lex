// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/detail/production_rule_base.hpp>

namespace foonathan
{
namespace lex
{
    namespace production_rule
    {
        namespace detail
        {
            template <class Production>
            struct production : base_rule
            {
                template <class Cont>
                struct parser : Cont
                {
                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                        -> decltype(Cont::parse(
                            tokenizer, f, static_cast<Args&&>(args)...,
                            Production::parse(tokenizer, f).template value_or_tag<Production>()))
                    {
                        auto result = Production::parse(tokenizer, f);
                        if (result.is_success())
                            return Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                               result.template value_or_tag<Production>());
                        else
                            return {};
                    }
                };
            };

            template <class... Rules>
            struct sequence : base_rule
            {
                template <class Cont>
                using parser = parser_for<Rules..., Cont>;
            };
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED

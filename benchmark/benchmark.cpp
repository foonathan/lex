// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <benchmark/benchmark.h>

#include "bm_baseline.hpp"
#include "bm_manual.hpp"
#include "bm_tokenizer.hpp"
#include "bm_tokenizer_manual.hpp"

namespace lex = foonathan::lex;

namespace
{
char       all_error[32 * 1024];
char       all_last[32 * 1024];
char       all_first[32 * 1024];
const char punctuation[]    = "....+=+++->*->---=-~";
const char punctuation_ws[] = "...  .  +=  ++  +  ->*  ->  --  -=  -  ~";

auto init = []() noexcept
{
    for (auto& c : all_error)
        c = '@';
    for (auto& c : all_last)
        c = '~';
    for (auto& c : all_first)
        c = '.';
    return 0;
}
();

template <class Func>
void benchmark_impl(Func f, benchmark::State& state, const char* str, const char* end)
{
    for (auto _ : state)
    {
        f(str, end, [](lex::token_spelling spelling) {
            benchmark::DoNotOptimize(spelling.data());
            benchmark::DoNotOptimize(spelling.size());
        });
    }
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(end - str));
}
} // namespace

template <unsigned N>
void bm_baseline(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&baseline, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_baseline, all_error, all_error);
BENCHMARK_CAPTURE(bm_baseline, all_last, all_last);
BENCHMARK_CAPTURE(bm_baseline, all_first, all_first);
BENCHMARK_CAPTURE(bm_baseline, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_baseline, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_manual(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&manual, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_manual, all_error, all_error);
BENCHMARK_CAPTURE(bm_manual, all_last, all_last);
BENCHMARK_CAPTURE(bm_manual, all_first, all_first);
BENCHMARK_CAPTURE(bm_manual, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_manual, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_tokenizer(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&tokenizer, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_tokenizer, all_error, all_error);
BENCHMARK_CAPTURE(bm_tokenizer, all_last, all_last);
BENCHMARK_CAPTURE(bm_tokenizer, all_first, all_first);
BENCHMARK_CAPTURE(bm_tokenizer, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_tokenizer, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_tokenizer_manual(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&tokenizer_manual, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_tokenizer_manual, all_error, all_error);
BENCHMARK_CAPTURE(bm_tokenizer_manual, all_last, all_last);
BENCHMARK_CAPTURE(bm_tokenizer_manual, all_first, all_first);
BENCHMARK_CAPTURE(bm_tokenizer_manual, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_tokenizer_manual, punctuation_ws, punctuation_ws);

BENCHMARK_MAIN();

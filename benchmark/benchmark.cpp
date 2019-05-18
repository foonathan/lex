// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <benchmark/benchmark.h>

#include <fstream>

#include "bm_baseline.hpp"
#include "bm_manual.hpp"
#include "bm_manual_opt.hpp"
#include "bm_tokenizer.hpp"
#include "bm_tokenizer_manual.hpp"
#include "bm_trie.hpp"

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
        f(str, end, [](int kind, lex::token_spelling spelling) {
            benchmark::DoNotOptimize(kind);
            benchmark::DoNotOptimize(spelling.data());
            benchmark::DoNotOptimize(spelling.size());
        });
    }
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations())
                            * static_cast<std::int64_t>(end - str));
}
} // namespace

template <unsigned N>
void bm_0_baseline(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&baseline, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_0_baseline, all_error, all_error);
BENCHMARK_CAPTURE(bm_0_baseline, all_last, all_last);
BENCHMARK_CAPTURE(bm_0_baseline, all_first, all_first);
BENCHMARK_CAPTURE(bm_0_baseline, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_0_baseline, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_1_manual(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&manual, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_1_manual, all_error, all_error);
BENCHMARK_CAPTURE(bm_1_manual, all_last, all_last);
BENCHMARK_CAPTURE(bm_1_manual, all_first, all_first);
BENCHMARK_CAPTURE(bm_1_manual, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_1_manual, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_2_tokenizer_manual(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&tokenizer_manual, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_2_tokenizer_manual, all_error, all_error);
BENCHMARK_CAPTURE(bm_2_tokenizer_manual, all_last, all_last);
BENCHMARK_CAPTURE(bm_2_tokenizer_manual, all_first, all_first);
BENCHMARK_CAPTURE(bm_2_tokenizer_manual, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_2_tokenizer_manual, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_3_manual_opt(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&manual_opt, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_3_manual_opt, all_error, all_error);
BENCHMARK_CAPTURE(bm_3_manual_opt, all_last, all_last);
BENCHMARK_CAPTURE(bm_3_manual_opt, all_first, all_first);
BENCHMARK_CAPTURE(bm_3_manual_opt, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_3_manual_opt, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_4_trie(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&trie, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_4_trie, all_error, all_error);
BENCHMARK_CAPTURE(bm_4_trie, all_last, all_last);
BENCHMARK_CAPTURE(bm_4_trie, all_first, all_first);
BENCHMARK_CAPTURE(bm_4_trie, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_4_trie, punctuation_ws, punctuation_ws);

template <unsigned N>
void bm_5_tokenizer(benchmark::State& state, const char (&array)[N])
{
    benchmark_impl(&tokenizer, state, array, array + N - 1);
}
BENCHMARK_CAPTURE(bm_5_tokenizer, all_error, all_error);
BENCHMARK_CAPTURE(bm_5_tokenizer, all_last, all_last);
BENCHMARK_CAPTURE(bm_5_tokenizer, all_first, all_first);
BENCHMARK_CAPTURE(bm_5_tokenizer, punctuation, punctuation);
BENCHMARK_CAPTURE(bm_5_tokenizer, punctuation_ws, punctuation_ws);

int main(int argc, char* argv[])
{
    // a reporter that generates an HTML table output
    struct Reporter : benchmark::BenchmarkReporter
    {
        bool ReportContext(const Context&) override
        {
            return true;
        }

        void ReportRuns(const std::vector<Run>& report) override
        {
            for (auto& run : report)
            {
                auto name = split_name(run.run_name.str());

                // insert the category if necessary
                auto iter = std::find(categories_.begin(), categories_.end(), name.second);
                if (iter == categories_.end())
                    categories_.push_back(name.second);

                // insert the data
                result_[name.first].push_back(run.counters.at("bytes_per_second"));
            }
        }

        void Finalize() override
        {
            auto& out = GetOutputStream();

            out << R"(<style>
td {
    min-width: 7em;
    text-align: right;
}
</style>
)";

            out << "<table>\n";
            out << "<thead><tr><th> </th>";
            for (auto& cat : categories_)
                out << "<th>" << cat << "</th>";
            out << "</tr></thead>\n";

            out << "<tbody>\n";
            for (auto& pair : result_)
            {
                out << "<tr>";
                out << "<th>" << pair.first << "</th>";
                for (auto& result : pair.second)
                {
                    out << "<td>";
                    print_result(out, result);
                    out << "</td>";
                }
                out << "</tr>\n";
            }
            out << "</tbody>\n";
            out << "</table>\n";

            categories_.clear();
            result_.clear();
        }

    private:
        static std::pair<std::string, std::string> split_name(const std::string& run_name)
        {
            auto slash = run_name.find('/');
            auto name  = run_name.substr(0, slash);
            auto data  = run_name.substr(slash + 1);
            return std::make_pair(name, data);
        }

        static void print_result(std::ostream& out, double result)
        {
            out << static_cast<std::uint64_t>(result / (1024 * 1024)) << " MiB/s";
        }

        std::vector<std::string>                   categories_;
        std::map<std::string, std::vector<double>> result_;
    } reporter;

    // need to specify an output file for custom file reporters
    // this is cheating...
    char extra_flag[] = "--benchmark_out=result.html";
    argv[argc]        = extra_flag;
    ++argc;

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks(nullptr, &reporter);
}

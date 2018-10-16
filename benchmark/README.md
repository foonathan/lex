# lex: Benchmarks

The `foonathan_lex_benchmark` target defines a simple benchmark.
Running it produces a `result.html` file which displays the results in a simple table.

## Setup

The benchmarks all tokenize an input based on tokens that contain a subset of the C++ punctuation tokens,
as well as whitespace.
The various algorithms that are being benchmarked:

* `bm_0_baseline`: This is the baseline algorithm that treats each character of the input as a single token.
It is obviously not correct.

* `bm_1_manual`: This is the naive and manual implementation of a tokenizer.
(`if (starts_with(str, "+")) … else if (starts_with(str, "-")) …`)

* `bm_2_tokenizer_manual`: This is the naive and manual implementation
but wrapped into `lex::rule_token`s so it works with `lex::tokenizer`.

* `bm_3_manual_opt`: This is a manual state machine implementation to tokenize the input.

* `bm_4_trie`: This is an implementation that uses the trie to parse literals and custom code to parse whitespace.

* `bm_5_tokenizer`: This is the implementation that uses the library as intended

The inputs are as follows:

* `all_error`: `32KiB` of an invalid character.
* `all_last`: `32KiB` of the token that is checked last by the manual state machine implementation.
* `all_first`: `32KiB` of the token that is checked first by the manual state machine implementation.
* `punctuation`: All punctuation tokens but no whitespace.
* `punctuation_ws`: All punctuation tokens separated by whitespace.

## Results

On my Thinkpad 13 with an intel core i5 processor I've gotten the following results:

<table>
<thead><tr><th> </th><th>all_error</th><th>all_last</th><th>all_first</th><th>punctuation</th><th>punctuation_ws</th></tr></thead>
<tbody>
<tr><th>bm_0_baseline</th><td>2433 MiB/s</td><td>2425 MiB/s</td><td>2419 MiB/s</td><td>2096 MiB/s</td><td>1240 MiB/s</td></tr>
<tr><th>bm_1_manual</th><td>6 MiB/s</td><td>6 MiB/s</td><td>126 MiB/s</td><td>23 MiB/s</td><td>16 MiB/s</td></tr>
<tr><th>bm_2_tokenizer_manual</th><td>7 MiB/s</td><td>7 MiB/s</td><td>131 MiB/s</td><td>21 MiB/s</td><td>17 MiB/s</td></tr>
<tr><th>bm_3_manual_opt</th><td>813 MiB/s</td><td>602 MiB/s</td><td>1202 MiB/s</td><td>557 MiB/s</td><td>401 MiB/s</td></tr>
<tr><th>bm_4_trie</th><td>697 MiB/s</td><td>985 MiB/s</td><td>605 MiB/s</td><td>650 MiB/s</td><td>516 MiB/s</td></tr>
<tr><th>bm_5_tokenizer</th><td>302 MiB/s</td><td>1401 MiB/s</td><td>770 MiB/s</td><td>779 MiB/s</td><td>398 MiB/s</td></tr>
</tbody>
</table>

The manual naive implementation is vastly outperformed by the handwritten state machine.
My library implementation is on-par or superior to the handwritten state machine,
except in the single-character edge cases.


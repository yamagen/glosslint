# glossemit

`glossemit` is a small UNIX-style filter for emitting publication-ready interlinear glosses from JSONL.

It reads one JSON object per line from standard input and writes formatted output to standard output. The same [`controlled-xx-xx.json`](controlled.md) specification used by `glosslint` supplies publication defaults.

`glossemit` does not know or constrain the outer structure of a corpus. `jq` selects the material to publish and projects it into JSONL.

## Input

Each input line is a JSON object containing a `word-gloss` array. Translation fields may also be present.

```json
{"word-gloss":[{"word":"得","romaji":"e","gloss":"receive.ADV"},{"word":"て","romaji":"te","gloss":"CONJ"}],"translation-en-natural":"It was a matter after receiving."}
```

A corpus can therefore keep any outer JSON organization it needs:

```sh
jq -c '...' corpus.json |
  glossemit -c config/controlled-ja-en.json --latex
```

The same applies to JSON produced by another tool, including SUI-style output:

```sh
jq -c '...' sui-output.json |
  glossemit -c config/controlled-ja-en.json --html
```

The only interface that needs to be shared is the JSONL object passed on standard input.

## LaTeX

LaTeX is the default output mode:

```sh
jq -c '...' corpus.json |
  glossemit -c config/controlled-ja-en.json --latex > gloss.tex
```

The current example configuration emits ExPex fragments using three aligned tiers:

```text
word
romaji
gloss
```

followed by the first available translation selected from the configured translation tags.

The program emits a fragment, not a complete LaTeX document. Package choice, document class, page layout, and surrounding prose remain outside `glossemit`.

See [`examples/latex/`](../examples/latex/) for a complete use example.

## HTML

HTML output is selected with `--html`:

```sh
jq -c '...' corpus.json |
  glossemit -c config/controlled-ja-en.json --html > gloss.html
```

The output is an HTML fragment rather than a complete HTML document. The current configuration groups tiers token by token so that each token remains an interlinear unit when CSS wraps the sequence.

For example:

```html
<div class="gloss">
  <div class="gloss-tokens">
    <div class="gloss-token">
      <div class="gla">得</div>
      <div class="glb">e</div>
      <div class="glc">receive</div>
    </div>
  </div>
  <div class="glft">It was a matter after receiving.</div>
</div>
```

HTML text and class values are escaped by `glossemit`. Elements and class names are selected in the controlled configuration; CSS remains the user's responsibility.

See [`examples/html/glosstest.html`](../examples/html/glosstest.html) for a complete HTML5 example with CSS.

## Configuration

The `emit` section of `controlled-xx-xx.json` contains format-specific defaults. A project can choose which stored annotation fields become visible tiers without changing the source annotation.

This separates preservation from presentation: a corpus may retain kana, romaji, lemma information, and other fields even when a publication emits only a selected subset.

See [`controlled.md`](controlled.md) for details.

## Design

`glossemit` is not a postprocessor tied to `glosslint`. The two programs are sibling tools that share an annotation specification:

```text
                         controlled-xx-xx.json
                           /               \
                          /                 \
annotation stream → glosslint           glossemit ← JSONL projection
                     validation        publication
                                        /       \
                                     LaTeX     HTML
```

This also means that any data source able to produce the required JSONL can use `glossemit`.

```text
each project's JSON
        ↓ jq
      JSONL
        ↓
    glossemit
     /     \
 LaTeX     HTML
```

The program therefore controls output formatting, not how researchers must write their corpus JSON.

## Options

```text
-c, --control-in FILE   controlled specification / emit configuration
    --latex             use emit.latex (default)
    --html              use emit.html
-h, --help              show help
-v, --version           show version
```

`--latex` and `--html` are mutually exclusive.

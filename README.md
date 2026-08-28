# glosslint

![Unix Pipeline](https://img.shields.io/badge/Unix-pipeline-blue)
![Editor Agnostic](https://img.shields.io/badge/editor-agnostic-blue)
![Human in the Loop](https://img.shields.io/badge/human-in--the--loop-blue)

`glosslint` is a small UNIX-style filter for detecting inconsistency in word-gloss annotation.

It does not parse JSON directly. Instead, JSON is flattened with `jq`, and
`glosslint` checks the resulting TSV stream against a controlled vocabulary.

## Requirements

- Compile with a C compiler (e.g., gcc).
- Input must be UTF-8 encoded.
- `jq` is required for flattening JSON data into TSV format.
- A controlled vocabulary JSON file may be supplied with `-c` or `--control-in`.

## Input format

`glosslint` expects six tab-separated fields:

```text
file<TAB>line<TAB>id<TAB>word<TAB>gloss<TAB>pos
```

## Example

```text
data.json	7	117	水	water	N
data.json	8	117	を	OBJ	P
data.json	9	117	あさ	shallow	ADJ.STEM
data.json	10	117	み	REASON	SUF
```

The `file` and `line` fields allow `glosslint` to emit source-aware diagnostics such as:

```text
data.json:8:1: warning: unknown gloss component [id=117 word=を gloss=OBJ pos=P]
```

## Usage

For JSON data, use the included `glosslint-json` wrapper, which extracts
`word-gloss` records with `jq --stream` and preserves source line numbers:

```sh
glosslint-json controlled-ja-en.json data.json
```

The underlying pipeline is:

```text
JSON
  ↓
jq --stream
  ↓
glosslint
  ↓
file:line:column diagnostics
```

`glosslint` can also read an already flattened TSV stream directly:

```sh
glosslint -c controlled-ja-en.json < word-gloss.tsv
```

## Checks

The current version checks:

1. malformed TSV lines
2. empty id
3. empty word
4. empty gloss
5. empty pos
6. unknown POS labels
7. unknown gloss components
8. controlled conjugation labels used in gloss components
9. notation problems such as lowercase lexical glosses joined with `.`
10. unstable gloss usage for the same word

By default, unknown labels are warnings. To treat them as errors, use:

```sh
glosslint -c controlled-ja-en.json --unknown-error < word-gloss.tsv
```

## Controlled vocabulary

A controlled vocabulary is supplied as JSON. It separates at least three namespaces:

```json
{
  "conjugation": {
    "ADV": "adverbial",
    "FIN": "finite"
  },
  "pos": {
    "N": "noun",
    "V": "verb"
  },
  "gloss": {
    "PST": "past",
    "NEG": "negative"
  }
}
```

Lowercase lexical glosses such as `water`, `shallow`, or `stand-up` are open-ended.
Non-lowercase gloss components are checked against the controlled `gloss` and
`conjugation` namespaces, while POS values are checked against `pos`.

The control file is therefore both a validation resource and an explicit,
machine-readable statement of the analytical conventions used by a project.

`glosslint` detects inconsistency; interpretation remains a human decision.

## Build

```sh
cd src
make
```

## Install

```sh
sudo make install
```

After installation:

```sh
which glosslint
glosslint -v
```

## Editor integration

`glosslint` emits diagnostics in a standard source-location form that can be
consumed by editor diagnostic interfaces.

- [Neovim](docs/editors/neovim.md)
- [Visual Studio Code](docs/editors/vscode.md)
- [Emacs](docs/editors/emacs.md)

A typical interactive workflow is:

```text
save
→ check
→ jump
→ correct
→ save again
```

## License

MIT License.

## Author

Hilofumi Yamamoto, Ph.D.  
Institute of Science Tokyo

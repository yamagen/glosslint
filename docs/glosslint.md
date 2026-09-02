# glosslint

`glosslint` is a small UNIX-style filter for detecting inconsistency in word-gloss annotation.

It does not impose a corpus JSON structure. Corpus data can be projected into the required stream with `jq`, then checked against a shared [`controlled-xx-xx.json`](controlled.md) annotation specification.

## Input

`glosslint` reads tab-separated records from standard input:

```text
file<TAB>line<TAB>id<TAB>word<TAB>gloss<TAB>pos
```

For example:

```text
data.json	7	117	水	water	N
data.json	8	117	を	OBJ	P
```

The source-location fields allow diagnostics such as:

```text
data.json:8:1: warning: unknown gloss component [id=117 word=を gloss=OBJ pos=P]
```

The bundled `glosslint-json` wrapper can extract `word-gloss` records from JSON while preserving source line information.

```sh
glosslint-json config/controlled-ja-en.json data.json
```

An already flattened stream can also be checked directly:

```sh
glosslint -c config/controlled-ja-en.json < word-gloss.tsv
```

## Checks

The current implementation checks malformed input, required annotation fields, unknown POS labels, unknown gloss components, controlled conjugation labels, notation problems, unstable gloss usage for the same surface form, and schema constraints declared in the controlled file.

By default, unknown labels are warnings. They can be promoted to errors with:

```sh
glosslint -c config/controlled-ja-en.json --unknown-error < word-gloss.tsv
```

`glosslint` reports inconsistency; interpretation remains a human decision.

## Schema-driven validation

The `schema` object in the controlled file declares which fields are expected in annotation records and how they are checked. Current schema values include `off`, `controlled`, and `integer`.

See [`controlled.md`](controlled.md) for the shared specification format.

## Editor integration

Because diagnostics use source locations, `glosslint` can participate in an interactive editing cycle:

```text
save
  ↓
check
  ↓
jump
  ↓
correct
  ↓
save again
```

Editor-specific notes are available for:

- [Neovim](editors/neovim.md)
- [Visual Studio Code](editors/vscode.md)
- [Emacs](editors/emacs.md)

The editor is not part of the validation logic. `glosslint` remains an editor-independent command-line program.

## Design

`glosslint` follows a small-filter design:

```text
JSON / corpus data
      ↓ jq
TSV annotation stream
      ↓
glosslint + controlled-xx-xx.json
      ↓
file:line:column diagnostics
```

This keeps corpus structure, stream transformation, validation, and editor integration separate. The controlled specification can then be reused independently by [`glossemit`](glossemit.md) for publication output.

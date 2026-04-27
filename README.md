# glosslint

`glosslint` is a small UNIX-style validation filter for word-gloss data.

It does not parse JSON directly. Instead, JSON is flattened with `jq`, and
`glosslint` checks the resulting TSV stream.

## Input format

`glosslint` expects four tab-separated fields:

```text
id<TAB>word<TAB>gloss<TAB>pos
```

## Example:

```text
117	水	water	N
117	を	OBJ	P
117	あさ	shallow	ADJ.STEM
117	み	REASON	SUF
```

## Usage

```sh
jq -r '
  .[] |
  .id as $id |
  ."word-gloss"[] |
  [$id, .word, .gloss, .pos] | @tsv
' data.json | src/glosslint
```

## Checks

The first version checks:

1. malformed TSV lines
2. empty id
3. empty word
4. empty gloss
5. empty pos
6. unknown gloss labels, if a label list is given
7. unstable gloss usage for the same word

## Label file

A label file may be supplied with -l.

```text
# gloss-labels.txt
water
OBJ
shallow
REASON
N
P
SUF
```

## Run:

```sh
src/glosslint -l gloss-labels.txt < word-gloss.tsv
```

By default, unknown labels are warnings. To treat them as errors:

```sh
src/glosslint -l gloss-labels.txt --unknown-error < word-gloss.tsv
```

## Build

```sh
cd src
make
```

## Install

sudo make install

## License

MIT License.

## Author

Hilofumi Yamamoto, Ph.D. Institute for Science Tokyo

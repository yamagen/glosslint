# controlled-xx-xx.json

A `controlled-xx-xx.json` file is a machine-readable annotation specification shared by `glosslint` and `glossemit`.

It is not only a list of gloss abbreviations. It records the fields expected in annotation data, the controlled grammatical inventories used by a project, and the default presentation rules used when glosses are emitted for publication.

A typical file contains the following sections:

```json
{
  "schema": { ... },
  "conjugation": { ... },
  "pos": { ... },
  "gloss": { ... },
  "emit": {
    "latex": { ... },
    "html": { ... }
  }
}
```

## Role

The same specification can be used at several stages of a workflow:

```text
annotation data
      |
      +----> glosslint ----> validation diagnostics
      |
      +----> glossemit ----> LaTeX / HTML
```

`glosslint` uses the controlled file to check whether annotation follows the declared conventions. `glossemit` uses the same file to decide which annotation tiers are emitted and how they are represented in a target format.

The two programs therefore share a specification, but neither depends on the other.

A controlled file is not merely a descriptive inventory. It is an **operational specification** used in actual annotation, validation, and emission within the glosssuite workflow. The distinctions recorded in the file can therefore be tested by practice: annotations are checked against them, inconsistencies are reported, humans revise the data where appropriate, and the same specification is then reused when the annotation is prepared for publication.

This makes a controlled file different from an unchecked list of categories written only for documentation. It is a description that participates in the working annotation system.

## schema

The `schema` object declares the fields expected in each annotation record and how those fields are checked.

For example:

```json
"schema": {
  "word": "off",
  "lemma": "off",
  "kana": "off",
  "lemma-kana": "off",
  "romaji": "off",
  "lemma-romaji": "off",
  "gloss": "controlled",
  "pos": "controlled",
  "ku": "integer"
}
```

Current schema values include:

- `off`: the field must be present, but its content is not validated by the generic schema checker.
- `controlled`: the value is checked against the relevant controlled namespace.
- `integer`: the value must be an integer.

The schema describes structure without forcing every field into a language-independent interpretation. Language-specific checks can remain language-specific.

## Controlled namespaces

The `conjugation`, `pos`, and `gloss` objects define controlled labels used by a project.

For example:

```json
"conjugation": {
  "ADV": "adverbial / ren'yokei",
  "FIN": "finite / sentence-final",
  "IRR": "irrealis / mizenkei"
},
"pos": {
  "N": "noun",
  "V": "verb",
  "AUX": "auxiliary",
  "P": "particle"
},
"gloss": {
  "GEN": "genitive",
  "NEG": "negation / negative",
  "PST": "past"
}
```

Lowercase lexical glosses can remain open-ended, while grammatical labels are explicitly controlled. This allows the specification to constrain analytical notation without requiring a closed lexicon.

## Emission rules

The `emit` object describes publication defaults.

For LaTeX, it can specify which tiers correspond to ExPex commands:

```json
"latex": {
  "lines": [
    { "tag": "word", "command": "\\gla" },
    { "tag": "romaji", "command": "\\glb" },
    { "tag": "gloss", "command": "\\glc", "split": "." }
  ]
}
```

For HTML, it can specify the elements and classes used in the generated fragment:

```json
"html": {
  "container": { "element": "div", "class": "gloss" },
  "tokens": { "element": "div", "class": "gloss-tokens" },
  "token": { "element": "div", "class": "gloss-token" },
  "lines": [
    { "tag": "word", "element": "div", "class": "gla" },
    { "tag": "romaji", "element": "div", "class": "glb" },
    { "tag": "gloss", "element": "div", "class": "glc", "split": "." }
  ]
}
```

Presentation remains separate from annotation storage. A corpus may preserve kana, romaji, lemma information, and other fields even when only a subset is selected for a particular publication format.

## A shared resource for cross-linguistic comparison

Controlled files are not only configuration resources. As language-specific, machine-readable annotation specifications, they can also serve as comparable records of which grammatical and annotation distinctions are operationalized across languages.

For example, a collection such as

```text
controlled-ja-en.json
controlled-ko-en.json
controlled-ainu-en.json
controlled-de-en.json
```

can make it possible to compare, among other things, which distinctions are represented as parts of speech, which inflectional or conjugational categories are explicitly encoded, which gloss labels are controlled, which annotation fields are required, and which information is deliberately left open or underspecified.

This comparison should not require every language to be forced into one universal inventory in advance. Each controlled file can first describe the distinctions that are actually useful for its own annotation practice. Cross-linguistic comparison can then proceed by placing those specifications alongside one another and observing both correspondences and differences.

Because the files are operational specifications rather than unchecked descriptive lists, such comparison is grounded in annotation systems that have actually been exercised through validation and publication. The comparison is therefore based on distinctions that researchers have had to make explicit enough for software to inspect and use.

In this sense, a collection of `controlled-xx-xx.json` files can become a shared scholarly resource: not a manually reconstructed feature matrix, but a set of feature inventories grounded in actual annotation, validation, and publication practice.

## Naming

The `xx-xx` portion indicates the language pairing used by the project. For example:

```text
controlled-ja-en.json
```

can be used for Japanese annotation with English glossing conventions.

The filename is intentionally independent of any particular corpus. The same annotation specification may therefore be shared by multiple works, pipelines, and tools.

A working example is provided in [`config/controlled-ja-en.json`](../config/controlled-ja-en.json).

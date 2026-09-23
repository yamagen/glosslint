# Why gloss annotation should be reusable

Interlinear glossing is not a disposable formatting task.
It involves scholarly decisions about segmentation, lexical interpretation,
grammatical categories, and the relation between an example and its context.

When researchers have already paid the cost of reading an example,
constructing its annotation, and checking that annotation by hand,
the resulting analysis should be preserved as reusable research data.

## Glosses as reusable research data

The Leipzig Glossing Rules explicitly note that glosses are part of the
analysis rather than simply part of the primary data.

This distinction matters for reuse. A glossed example is not merely a
visual arrangement prepared for one publication. It contains analytical
decisions that may be useful in later publications, comparative work,
teaching materials, corpus analysis, or computational processing.

## CrossGram and the reuse of glossed examples

CrossGram provides an important example of this principle.

Haspelmath describes glossed example sentences presented in tabular,
accessible form as a particularly clear improvement in transparency.
He also notes that interlinear glossed text has uses independently of
the typological claims for which an example was originally collected.
Making such examples accessible therefore improves their reusability.

CrossGram consequently exposes primary text, glosses, translations,
sources, and related metadata as searchable data rather than leaving
examples embedded only in PDF publications or supplements.

## The glosssuite approach

glosssuite follows the same general motivation but addresses a different
stage of the research workflow.

A human-reviewed annotation is stored as structured data first.
Publication output is then derived from that stored annotation:

    reviewed annotation
            |
            +----> glosslint ----> validation
            |
            +----> glossemit ----> LaTeX / HTML

The LaTeX or HTML rendering is therefore not the research asset itself.
It is one view of a persistent annotation resource.

This distinction allows the same checked annotation to be reused in
different publications, exported in different presentation formats,
or processed by other research tools such as SUI.

## Human review remains essential

Automation can reduce the cost of producing candidate segmentation or
glosses, but scholarly annotation still requires inspection in context.

The goal is therefore not to eliminate human review, but to avoid paying
for the same human review repeatedly.

Once an analysis has been inspected and accepted, preserving it as
structured annotation data allows that scholarly effort to accumulate
rather than disappear into a one-time rendering.

## Reuse as cumulative scholarship

Reusable gloss annotation supports cumulative research.

It allows later work to inspect, validate, reinterpret, compare, and
republish analyses without reconstructing them from printed examples.
In this sense, persistence and reuse are not merely software conveniences;
they are part of the scholarly value of annotation.

## References

- Comrie, Bernard, Martin Haspelmath, and Balthasar Bickel.
  The Leipzig Glossing Rules.
- Haspelmath, Martin. 2024.
  “Language parameters and construction parameters in the CrossGram database collection.”
- CrossGram. https://crossgram.clld.org/

### glosslint TODO

**1. word-gloss schema lint**

- [ ] 各 `word-gloss` が規定の **9 fields** を持つか
- [ ] 必須キーの欠落検出
- [ ] 余分なキーの検出
- [ ] キー名の typo 検出
- [ ] `ku` の型・値チェック
- [ ] 9 fields の順序も必要ならチェック

規定形：

```text
word
lemma
kana
lemma-kana
romaji
lemma-romaji
gloss
pos
ku
```

これは今回の「いつの間にか9項目→7項目」事故を直接防ぐものですね。

**2. abbreviations lint**

- [ ] `gloss` で使用した略号が `abbreviations` にすべて定義されているか
- [ ] `pos` で使用した略号もすべて定義されているか
- [ ] 定義されているが実際には使われていない略号の警告
- [ ] abbreviation の説明が canonical definition と一致するか

今回なら、

```text
ADV  adverbial form in gloss, and adverb in pos
ATTR attributive form
FIN  finite form
PERF perfective
CONJ connective
```

などですね。`ADV: adverbial` や `FIN: final` への drift を拾います。

**3. gloss / POS consistency lint**

- [ ] `.ADV` と活用形の整合
- [ ] `.ATTR`
- [ ] `.IRR`
- [ ] `.REALIS`
- [ ] `.FIN`
- [ ] AUX と助動詞 gloss の整合
- [ ] P と `ACC / DAT / GEN / NOM / TOP / CONJ / ...` の整合
- [ ] 明らかに不可能な組合せを error / suspicious に分類

ここは最初から厳しくしすぎず、**error と warning を分ける**のがよさそうです。

**4. project convention lint**

- [ ] `ぬ` → `PERF`
- [ ] `り` → `PERF`
- [ ] `つ` → `PERF`
- [ ] `たり` → `PERF`
- [ ] `PFV` が残っていたら警告
- [ ] `ば` → `CONJ`, `pos:P`
- [ ] その他、伊勢・土佐で確定した annotation convention を規則化

つまり、

```text
る / り / PFV.ATTR
```

を今回のように見つけて、

```text
expected: PERF.ATTR
```

と警告できるようにする。

**5. lemma consistency lint**

- [ ] 活用形に対する lemma の整合性
- [ ] `kana` と `lemma-kana` を混同していないか
- [ ] `romaji` と `lemma-romaji` を混同していないか
- [ ] 同じ lemma がデータ中で不必要に複数表記になっていないか
- [ ] 漢字 lemma の表記揺れを一覧化

たとえば、

```text
洗ひ / 洗ふ / あらひ / あらふ
張り / 張る / はり / はる
```

という対応を検査する層です。

**6. Japanese romanization lint**

- [ ] `kana → romaji`
- [ ] `lemma-kana → lemma-romaji`
- [ ] 歴史的仮名遣いを保持する project convention
- [ ] `あひ → ahi`
- [ ] `おもひ → omohi`
- [ ] `いふ → ihu`
- [ ] `うへ → uhe`
- [ ] `しはす → shihasu`
- [ ] `にはか → nihaka`
- [ ] `いりあひ → iriahi`
- [ ] 特例 `をんな → onna`, `をとこ → otoko`
- [ ] 助詞 `は → wa`, `を → o` などの project convention

これは **core glosslint とは分離した Japanese-specific lint** にする。

**7. annotation drift detection**

- [ ] 同一 surface が以前と異なる lemma になった箇所
- [ ] 同一 lemma が以前と異なる gloss を持つ箇所
- [ ] 同一活用形の gloss が揺れている箇所
- [ ] romanization の揺れ
- [ ] abbreviation definition の揺れ
- [ ] 新しく出現した分析を報告

これはかなり重要です。「間違い」と断定するのではなく、

```text
WARN: 思ひ
  omohi : 17 occurrences
  omoi  : 1 occurrence
```

のように出せば、人間が判断できます。

**8. surface-preservation lint**

- [ ] source text と `word-gloss.word` の対応を確認
- [ ] word-gloss を連結すると対象 surface/span を復元できるか
- [ ] normalization によって source surface が消えていないかを警告
- [ ] raw surface と normalized analysis が異なる場合、それを明示的に保持しているか

これは今日のSUIの議論から新しく重要になりました。`契剣` を最初から `契り / けむ` にしてしまったら、lint以前に研究対象そのものを失います。

### 実装順

最初の C 実装なら、私は **1 → 2 → 4 → 6 → 3 → 5 → 7 → 8** の順がよいと思います。

特に **1 と 2 は機械的に100%判定できる**ので、まずここを完成させる。今回の7-field事故なら、この段階だけで即座に止められます。

そして romanization lint は独立した日本語 profile にする。SUIにつながる **surface-preservation lint は最後に付け足す機能というより、将来的には glosslint と SUI の境界部分**になりそうです。

これで、かなりちゃんとした `TODO.md` の骨格になります。

## 追記、

2026/08/31-18:55:55

はい、ここに追記しておくので十分です。むしろ、**既存の TODO を消して書き直すより、「設計整理」として追記する**のがよいと思います。

たとえば末尾に、こんな節を足すと今日の議論がそのまま残ります。

````markdown
### Design notes

#### schema-driven lint

`word-gloss` schema は固定の 9 fields を
`glosslint` 本体にハードコードせず、
`controlled-xx-xx.json` に記述する。

各 field について、たとえば次のような検査種別を指定する。

```text
off
string
integer
numeric
controlled
```
````

- schema に記載された field 自体の存在は常に確認する
- `off` は内容を検査しない
- `string`, `integer`, `numeric` は型を検査する
- `controlled` は control set と照合する

これにより Classical Japanese の 9-field schema だけでなく、
方言・フィールドワーク等の異なる annotation schema にも利用できる。

#### abbreviations

`abbreviations` は独立して lint する対象ではなく、
controlled vocabulary から生成する派生情報とする。

HTML / LaTeX 出力時の footnote や legend は、
将来 `glossemit` のような出力フィルタで生成する。

#### control-set rules

`gloss / POS consistency lint` と
`project convention lint` は、
core の新機能というより `controlled-xx-xx.json` の
記述内容・表現力として発展させる。

#### language-specific modules

`lemma consistency lint` と
`Japanese romanization lint` は
Japanese-specific なため core から分離する。

将来は外部 module / profile とし、
build または設定時に既定の module を選択できる構成を検討する。

#### ledger / SUI boundary

`annotation drift detection` と
`surface-preservation lint` は、
単一 annotation の validation よりも
履歴・差異・provenance を扱う機能である。

ledger は append-only とし、
既存 entry と少しでも異なる analysis は
既存行を更新せず新しい行として記録する。

したがってこれらは、
`glosslint` core よりも将来の `sui.c` / ledger 側で
検討する。

```

これなら元の TODO の「どういう問題を考えていたか」も残るし、その後に**今日の設計判断が上書きではなく履歴として残る**ので、とてもよいです。

特に最後の `append-only` は、あとで `sui.c` を始めるときの重要な設計原則になると思います。
```

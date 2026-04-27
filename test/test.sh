jq -r '
  .[] |
  .id as $id |
  ."word-gloss"[] |
  [
    ($id|tostring),
    (.word // ""),
    (.gloss // ""),
    (.pos // "")
  ] | @tsv
' test.json | ../src/glosslint

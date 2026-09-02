#!/bin/sh

jq --stream -nr '
  foreach (inputs, null) as $x
    (
      { ids: {}, rec: {}, current: null, emit: null };

      .emit = null |

      if $x == null then
        if .current != null then
          .rec[.current] as $r |
          .emit = ([
            input_filename,
            ($r.line // input_line_number),
            ($r.id // ""),
            ($r.word // ""),
            ($r.gloss // ""),
            ($r.pos // ""),
            ($r.json | tojson)
          ] | @tsv) |
          .current = null
        else
          .
        end

      else
        ($x[0]) as $p |

        if (($x|length) == 2
            and ($p|length) == 2
            and $p[1] == "id")
        then
          .ids[($p[0]|tostring)] = ($x[1]|tostring)

        elif (($x|length) == 2
              and ($p|length) == 4
              and $p[1] == "word-gloss")
        then
          ($p[0]|tostring) as $i |
          ($p[2]|tostring) as $j |
          ($i + "|" + $j) as $k |
          ($p[3]) as $f |

          if (.current != null and .current != $k) then
            .rec[.current] as $r |
            .emit = ([
              input_filename,
              ($r.line // input_line_number),
              ($r.id // ""),
              ($r.word // ""),
              ($r.gloss // ""),
              ($r.pos // ""),
              ($r.json | tojson)
            ] | @tsv)
          else
            .
          end |

          .current = $k |
          .rec[$k].id = (.ids[$i] // "") |
          .rec[$k].json[$f] = $x[1] |

          if $f == "word" then
            .rec[$k].word = ($x[1] // "")
          elif $f == "gloss" then
            .rec[$k].gloss = ($x[1] // "") |
            .rec[$k].line = input_line_number
          elif $f == "pos" then
            .rec[$k].pos = ($x[1] // "")
          else
            .
          end

        else
          .
        end
      end;

      .emit // empty
    )
' test.json

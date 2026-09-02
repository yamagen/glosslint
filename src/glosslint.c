#define _POSIX_C_SOURCE 200809L

#include "glosslint.h"

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t errors = 0;
static size_t warnings = 0;

#if defined(__GNUC__) || defined(__clang__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

static void *xmalloc(size_t n) UNUSED;

static void *xmalloc(size_t n) {
  void *p = malloc(n);
  if (!p) {
    fprintf(stderr, "fatal: malloc failed\n");
    exit(2);
  }
  return p;
}

static void *xrealloc(void *ptr, size_t n) {
  void *p = realloc(ptr, n);
  if (!p) {
    fprintf(stderr, "fatal: realloc failed\n");
    exit(2);
  }
  return p;
}

static char *xstrdup(const char *s) {
  char *p = strdup(s ? s : "");
  if (!p) {
    fprintf(stderr, "fatal: strdup failed\n");
    exit(2);
  }
  return p;
}

static void report_msg(MessageLevel level, const char *file, size_t source_line,
                       const char *id, const char *word, const char *gloss,
                       const char *pos, const char *msg) {
  const char *tag = (level == MSG_ERROR) ? "error" : "warning";

  if (level == MSG_ERROR) {
    errors++;
  } else {
    warnings++;
  }

  fprintf(stdout, "%s:%zu:1: %s: %s [id=%s word=%s gloss=%s pos=%s]\n",
          file ? file : "-", source_line, tag, msg, id ? id : "-",
          word ? word : "-", gloss ? gloss : "-", pos ? pos : "-");
}

static int is_lowercase_lexical(const char *s) {
  const unsigned char *p = (const unsigned char *)s;
  int has_alpha = 0;

  if (!s || *s == '\0') {
    return 0;
  }

  for (; *p; p++) {
    if (isalpha(*p)) {
      has_alpha = 1;
      if (!islower(*p)) {
        return 0;
      }
    } else if (*p == '-') {
      continue;
    } else {
      return 0;
    }
  }

  return has_alpha;
}

static void check_gloss_components(const char *file, size_t source_line,
                                   const char *id, const char *word,
                                   const char *gloss, const char *pos,
                                   const ControlSet *control,
                                   const Options *opt) {
  char *copy;
  char *saveptr = NULL;
  char *part;
  int lexical_count = 0;

  if (!gloss || *gloss == '\0') {
    return;
  }

  copy = xstrdup(gloss);

  for (part = strtok_r(copy, ".", &saveptr); part != NULL;
       part = strtok_r(NULL, ".", &saveptr)) {
    if (is_lowercase_lexical(part)) {
      lexical_count++;
      if (lexical_count > 1) {
        report_msg(MSG_WARN, file, source_line, id, word, gloss, pos,
                   "lexical glosses must not be joined with '.'");
      }
      continue;
    }

    lexical_count = 0;

    if (!string_vec_contains(&control->gloss, part) &&
        !string_vec_contains(&control->conjugation, part)) {
      report_msg(opt->unknown_error ? MSG_ERROR : MSG_WARN, file, source_line,
                 id, word, gloss, pos, "unknown gloss component");
    }
  }

  free(copy);
}

static void rstrip(char *s) {
  size_t n;

  if (!s) {
    return;
  }

  n = strlen(s);
  while (n > 0) {
    unsigned char c = (unsigned char)s[n - 1];
    if (c == '\n' || c == '\r') {
      s[n - 1] = '\0';
      n--;
    } else {
      break;
    }
  }
}

void record_vec_init(RecordVec *v) {
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

void record_vec_push(RecordVec *v, Record r) {
  if (v->len == v->cap) {
    size_t new_cap = v->cap ? v->cap * 2 : 64;
    v->items = xrealloc(v->items, new_cap * sizeof(v->items[0]));
    v->cap = new_cap;
  }
  v->items[v->len++] = r;
}

void record_vec_free(RecordVec *v) {
  size_t i;

  for (i = 0; i < v->len; i++) {
    free(v->items[i].file);
    free(v->items[i].id);
    free(v->items[i].word);
    free(v->items[i].gloss);
    free(v->items[i].pos);
  }

  free(v->items);
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

void string_vec_init(StringVec *v) {
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

void string_vec_push_owned(StringVec *v, char *s) {
  if (v->len == v->cap) {
    size_t new_cap = v->cap ? v->cap * 2 : 64;
    v->items = xrealloc(v->items, new_cap * sizeof(v->items[0]));
    v->cap = new_cap;
  }
  v->items[v->len++] = s;
}

int string_vec_contains(const StringVec *v, const char *s) {
  size_t i;

  for (i = 0; i < v->len; i++) {
    if (strcmp(v->items[i], s) == 0) {
      return 1;
    }
  }

  return 0;
}

void string_vec_free(StringVec *v) {
  size_t i;

  for (i = 0; i < v->len; i++) {
    free(v->items[i]);
  }

  free(v->items);
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

void schema_vec_init(SchemaVec *v) {
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

void schema_vec_push_owned(SchemaVec *v, char *name, SchemaRule rule) {
  if (v->len == v->cap) {
    size_t new_cap = v->cap ? v->cap * 2 : 16;
    v->items = xrealloc(v->items, new_cap * sizeof(v->items[0]));
    v->cap = new_cap;
  }

  v->items[v->len].name = name;
  v->items[v->len].rule = rule;
  v->len++;
}

void schema_vec_free(SchemaVec *v) {
  size_t i;

  for (i = 0; i < v->len; i++) {
    free(v->items[i].name);
  }

  free(v->items);
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

void control_set_init(ControlSet *c) {
  string_vec_init(&c->conjugation);
  string_vec_init(&c->pos);
  string_vec_init(&c->gloss);
  schema_vec_init(&c->schema);
}

void control_set_free(ControlSet *c) {
  string_vec_free(&c->conjugation);
  string_vec_free(&c->pos);
  string_vec_free(&c->gloss);
  schema_vec_free(&c->schema);
}

static int split_tsv6_or_7(char *line, char **f0, char **f1, char **f2,
                           char **f3, char **f4, char **f5, char **f6) {
  char *p;
  char *fields[7];
  int count = 0;

  fields[count++] = line;

  for (p = line; *p; p++) {
    if (*p == '\t') {
      *p = '\0';
      if (count < 7) {
        fields[count++] = p + 1;
      } else {
        return -1;
      }
    }
  }

  if (count != 6 && count != 7) {
    return 0;
  }

  *f0 = fields[0];
  *f1 = fields[1];
  *f2 = fields[2];
  *f3 = fields[3];
  *f4 = fields[4];
  *f5 = fields[5];
  *f6 = (count == 7) ? fields[6] : NULL;

  return 1;
}

static size_t count_json_object_fields(const char *json) {
  const unsigned char *p = (const unsigned char *)json;
  size_t count = 0;
  int depth = 0;
  int in_string = 0;
  int escaped = 0;

  if (!json) {
    return 0;
  }

  for (; *p; p++) {
    if (in_string) {
      if (escaped) {
        escaped = 0;
      } else if (*p == '\\') {
        escaped = 1;
      } else if (*p == '"') {
        in_string = 0;
      }
      continue;
    }

    if (*p == '"') {
      in_string = 1;
    } else if (*p == '{') {
      depth++;
    } else if (*p == '}') {
      if (depth > 0) {
        depth--;
      }
    } else if (*p == ':' && depth == 1) {
      count++;
    }
  }

  return count;
}

static int schema_has_field(const SchemaVec *schema, const char *name) {
  size_t i;

  for (i = 0; i < schema->len; i++) {
    if (strcmp(schema->items[i].name, name) == 0) {
      return 1;
    }
  }

  return 0;
}

static void collect_json_object_keys(const char *json, StringVec *keys) {
  const char *p = json;
  int object_depth = 0;
  int array_depth = 0;
  int in_string = 0;
  int escaped = 0;
  int expect_key = 0;
  const char *key_start = NULL;

  if (!json) {
    return;
  }

  for (; *p; p++) {
    unsigned char c = (unsigned char)*p;

    if (in_string) {
      if (escaped) {
        escaped = 0;
      } else if (c == '\\') {
        escaped = 1;
      } else if (c == '"') {
        in_string = 0;
        if (key_start) {
          size_t len = (size_t)(p - key_start);
          char *key = xmalloc(len + 1);
          memcpy(key, key_start, len);
          key[len] = '\0';
          string_vec_push_owned(keys, key);
          key_start = NULL;
          expect_key = 0;
        }
      }
      continue;
    }

    if (c == '"') {
      in_string = 1;
      if (object_depth == 1 && array_depth == 0 && expect_key) {
        key_start = p + 1;
      }
    } else if (c == '{') {
      object_depth++;
      if (object_depth == 1) {
        expect_key = 1;
      }
    } else if (c == '}') {
      if (object_depth > 0) {
        object_depth--;
      }
    } else if (c == '[') {
      array_depth++;
    } else if (c == ']') {
      if (array_depth > 0) {
        array_depth--;
      }
    } else if (c == ',' && object_depth == 1 && array_depth == 0) {
      expect_key = 1;
    }
  }
}

static void check_schema_fields(const char *file, size_t source_line,
                                const char *id, const char *word,
                                const char *gloss, const char *pos,
                                const char *record_json,
                                const SchemaVec *schema) {
  StringVec keys;
  size_t i;
  char msg[512];

  if (!record_json || schema->len == 0) {
    return;
  }

  string_vec_init(&keys);
  collect_json_object_keys(record_json, &keys);

  if (count_json_object_fields(record_json) != schema->len) {
    report_msg(MSG_ERROR, file, source_line, id, word, gloss, pos,
               "schema field count mismatch");
  }

  for (i = 0; i < schema->len; i++) {
    if (!string_vec_contains(&keys, schema->items[i].name)) {
      snprintf(msg, sizeof(msg), "missing field: %s", schema->items[i].name);
      report_msg(MSG_ERROR, file, source_line, id, word, gloss, pos, msg);
    }
  }

  for (i = 0; i < keys.len; i++) {
    if (!schema_has_field(schema, keys.items[i])) {
      snprintf(msg, sizeof(msg), "unknown field: %s", keys.items[i]);
      report_msg(MSG_ERROR, file, source_line, id, word, gloss, pos, msg);
    }
  }

  string_vec_free(&keys);
}

static int schema_rule_from_string(const char *s, SchemaRule *rule) {
  if (strcmp(s, "off") == 0) {
    *rule = SCHEMA_OFF;
  } else if (strcmp(s, "controlled") == 0) {
    *rule = SCHEMA_CONTROLLED;
  } else if (strcmp(s, "string") == 0) {
    *rule = SCHEMA_STRING;
  } else if (strcmp(s, "integer") == 0) {
    *rule = SCHEMA_INTEGER;
  } else if (strcmp(s, "numeric") == 0) {
    *rule = SCHEMA_NUMERIC;
  } else {
    return 0;
  }

  return 1;
}

static int parse_json_string_pair(char *line, char **key, char **value) {
  char *p;
  char *q;
  char *r;
  char *s;

  p = strchr(line, '"');
  if (!p) {
    return 0;
  }

  q = strchr(p + 1, '"');
  if (!q) {
    return 0;
  }

  r = strchr(q + 1, '"');
  if (!r) {
    return 0;
  }

  s = strchr(r + 1, '"');
  if (!s) {
    return 0;
  }

  *q = '\0';
  *s = '\0';
  *key = p + 1;
  *value = r + 1;
  return 1;
}

static int is_section_header(const char *line, const char *name) {
  char pattern[128];

  snprintf(pattern, sizeof(pattern), "\"%s\": {", name);
  return strstr(line, pattern) != NULL;
}

static int load_control_file(const char *path, ControlSet *control) {
  FILE *fp;
  char *line = NULL;
  size_t cap = 0;
  ssize_t nread;
  ControlSection section = SEC_NONE;

  fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "fatal: cannot open control file '%s': %s\n", path,
            strerror(errno));
    return -1;
  }

  while ((nread = getline(&line, &cap, fp)) != -1) {
    char *p;
    char *q;

    (void)nread;

    if (is_section_header(line, "conjugation")) {
      section = SEC_CONJUGATION;
      continue;
    }

    if (is_section_header(line, "pos")) {
      section = SEC_POS;
      continue;
    }

    if (is_section_header(line, "gloss")) {
      section = SEC_GLOSS;
      continue;
    }

    if (is_section_header(line, "schema")) {
      section = SEC_SCHEMA;
      continue;
    }

    if (section == SEC_SCHEMA) {
      char *key;
      char *value;
      SchemaRule rule;

      if (!parse_json_string_pair(line, &key, &value)) {
        continue;
      }

      if (!schema_rule_from_string(value, &rule)) {
        fprintf(stderr, "fatal: unknown schema rule '%s' for field '%s'\n",
                value, key);
        free(line);
        fclose(fp);
        return -1;
      }

      schema_vec_push_owned(&control->schema, xstrdup(key), rule);
      continue;
    }

    p = strchr(line, '"');
    if (!p) {
      continue;
    }

    q = strchr(p + 1, '"');
    if (!q) {
      continue;
    }

    *q = '\0';
    p++;

    switch (section) {
    case SEC_CONJUGATION:
      if (!string_vec_contains(&control->conjugation, p)) {
        string_vec_push_owned(&control->conjugation, xstrdup(p));
      }
      break;

    case SEC_POS:
      if (!string_vec_contains(&control->pos, p)) {
        string_vec_push_owned(&control->pos, xstrdup(p));
      }
      break;

    case SEC_GLOSS:
      if (!string_vec_contains(&control->gloss, p)) {
        string_vec_push_owned(&control->gloss, xstrdup(p));
      }
      break;

    default:
      break;
    }
  }

  free(line);
  fclose(fp);
  return 0;
}

static void read_records(FILE *fp, RecordVec *records,
                         const ControlSet *control, const Options *opt) {
  char *line = NULL;
  size_t cap = 0;
  ssize_t nread;
  size_t line_no = 0;

  while ((nread = getline(&line, &cap, fp)) != -1) {
    char *file;
    char *source_line_s;
    char *id;
    char *word;
    char *gloss;
    char *pos;
    char *record_json;
    size_t source_line;
    int split_result;
    Record r;

    (void)nread;
    line_no++;
    rstrip(line);

    if (line[0] == '\0') {
      continue;
    }

    split_result = split_tsv6_or_7(line, &file, &source_line_s, &id, &word,
                                   &gloss, &pos, &record_json);

    if (split_result != 1) {
      fprintf(stderr,
              "fatal: input line %zu: malformed TSV line: expected 6 or 7 fields\n",
              line_no);
      errors++;
      continue;
    }

    source_line = (size_t)strtoul(source_line_s, NULL, 10);

    check_schema_fields(file, source_line, id, word, gloss, pos, record_json,
                        &control->schema);

    if (*id == '\0') {
      report_msg(MSG_ERROR, file, source_line, id, word, gloss, pos,
                 "empty id");
    }

    if (*word == '\0') {
      report_msg(MSG_ERROR, file, source_line, id, word, gloss, pos,
                 "empty word");
    }

    if (*gloss == '\0') {
      report_msg(MSG_ERROR, file, source_line, id, word, gloss, pos,
                 "empty gloss");
    }

    if (*gloss != '\0') {
      check_gloss_components(file, source_line, id, word, gloss, pos, control,
                             opt);
    }

    if (*pos == '\0') {
      report_msg(MSG_ERROR, file, source_line, id, word, gloss, pos,
                 "empty pos");
    }

    if (*pos != '\0' && !string_vec_contains(&control->pos, pos)) {
      report_msg(opt->unknown_error ? MSG_ERROR : MSG_WARN, file, source_line,
                 id, word, gloss, pos, "unknown POS");
    }

    r.file = xstrdup(file);
    r.source_line = source_line;
    r.id = xstrdup(id);
    r.word = xstrdup(word);
    r.gloss = xstrdup(gloss);
    r.pos = xstrdup(pos);
    r.line_no = line_no;

    record_vec_push(records, r);
  }

  free(line);
}

static int gloss_seen_for_word(const RecordVec *records, const char *word,
                               const char *gloss, size_t before_index) {
  size_t i;

  for (i = 0; i < before_index; i++) {
    if (strcmp(records->items[i].word, word) == 0 &&
        strcmp(records->items[i].gloss, gloss) == 0) {
      return 1;
    }
  }

  return 0;
}

static int word_has_different_gloss_before(const RecordVec *records,
                                           const char *word, const char *gloss,
                                           size_t before_index) {
  size_t i;

  for (i = 0; i < before_index; i++) {
    if (strcmp(records->items[i].word, word) == 0 &&
        strcmp(records->items[i].gloss, gloss) != 0) {
      return 1;
    }
  }

  return 0;
}

static void check_variation(const RecordVec *records) {
  size_t i;

  for (i = 0; i < records->len; i++) {
    const Record *r = &records->items[i];

    if (*r->word == '\0' || *r->gloss == '\0') {
      continue;
    }

    if (!gloss_seen_for_word(records, r->word, r->gloss, i) &&
        word_has_different_gloss_before(records, r->word, r->gloss, i)) {
      report_msg(MSG_WARN, r->file, r->source_line, r->id, r->word, r->gloss,
                 r->pos, "unstable gloss usage");
    }
  }
}

static void usage(FILE *out) {
  fprintf(out,
          "glosslint %s\n"
          "usage: glosslint [options] < word-gloss.tsv\n\n"
          "Input format:\n"
          "  file<TAB>line<TAB>id<TAB>word<TAB>gloss<TAB>pos[<TAB>record-json]\n\n"
          "Options:\n"
          "  -c, --control-in FILE  controlled vocabulary JSON\n"
          "      --unknown-error    treat unknown labels as errors\n"
          "  -q, --quiet            print only summary\n"
          "  -h, --help             show this help\n"
          "  -v, --version          show version\n",
          GLOSSLINT_VERSION);
}

static void parse_options(int argc, char **argv, Options *opt) {
  static struct option long_opts[] = {{"control-in", required_argument, 0, 'c'},
                                      {"unknown-error", no_argument, 0, 1},
                                      {"quiet", no_argument, 0, 'q'},
                                      {"help", no_argument, 0, 'h'},
                                      {"version", no_argument, 0, 'v'},
                                      {0, 0, 0, 0}};
  int c;

  opt->quiet = 0;
  opt->unknown_error = 0;
  opt->control_file = NULL;

  while ((c = getopt_long(argc, argv, "c:qhv", long_opts, NULL)) != -1) {
    switch (c) {
    case 'c':
      opt->control_file = optarg;
      break;
    case 'q':
      opt->quiet = 1;
      break;
    case 'h':
      usage(stdout);
      exit(0);
    case 'v':
      printf("glosslint %s\n", GLOSSLINT_VERSION);
      exit(0);
    case 1:
      opt->unknown_error = 1;
      break;
    default:
      usage(stderr);
      exit(2);
    }
  }
}

int main(int argc, char **argv) {
  Options opt;
  RecordVec records;
  ControlSet control;

  parse_options(argc, argv, &opt);

  record_vec_init(&records);
  control_set_init(&control);

  if (opt.control_file) {
    if (load_control_file(opt.control_file, &control) != 0) {
      control_set_free(&control);
      record_vec_free(&records);
      return 2;
    }

    fprintf(stderr,
            "control: conjugation=%zu pos=%zu gloss=%zu schema=%zu\n",
            control.conjugation.len, control.pos.len, control.gloss.len,
            control.schema.len);
  }

  read_records(stdin, &records, &control, &opt);
  check_variation(&records);

  if (!opt.quiet) {
    fprintf(stderr, "SUMMARY\trecords=%zu\terrors=%zu\twarnings=%zu\n",
            records.len, errors, warnings);
  }

  control_set_free(&control);
  record_vec_free(&records);

  return errors ? 1 : 0;
}

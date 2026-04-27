#define _POSIX_C_SOURCE 200809L

#include "glosslint.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static size_t errors = 0;
static size_t warnings = 0;

#if defined(__GNUC__) || defined(__clang__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

static void *xmalloc(size_t n) UNUSED;


static void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "fatal: malloc failed\n");
        exit(2);
    }
    return p;
}

static void *xrealloc(void *ptr, size_t n)
{
    void *p = realloc(ptr, n);
    if (!p) {
        fprintf(stderr, "fatal: realloc failed\n");
        exit(2);
    }
    return p;
}

static char *xstrdup(const char *s)
{
    char *p = strdup(s ? s : "");
    if (!p) {
        fprintf(stderr, "fatal: strdup failed\n");
        exit(2);
    }
    return p;
}

static void rstrip(char *s)
{
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

static char *trim_in_place(char *s)
{
    char *end;

    while (*s && isspace((unsigned char)*s)) {
        s++;
    }

    end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) {
        *(end - 1) = '\0';
        end--;
    }

    return s;
}

void record_vec_init(RecordVec *v)
{
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

void record_vec_push(RecordVec *v, Record r)
{
    if (v->len == v->cap) {
        size_t new_cap = v->cap ? v->cap * 2 : 64;
        v->items = xrealloc(v->items, new_cap * sizeof(v->items[0]));
        v->cap = new_cap;
    }
    v->items[v->len++] = r;
}

void record_vec_free(RecordVec *v)
{
    size_t i;

    for (i = 0; i < v->len; i++) {
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

void string_vec_init(StringVec *v)
{
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

void string_vec_push_owned(StringVec *v, char *s)
{
    if (v->len == v->cap) {
        size_t new_cap = v->cap ? v->cap * 2 : 64;
        v->items = xrealloc(v->items, new_cap * sizeof(v->items[0]));
        v->cap = new_cap;
    }
    v->items[v->len++] = s;
}

int string_vec_contains(const StringVec *v, const char *s)
{
    size_t i;

    for (i = 0; i < v->len; i++) {
        if (strcmp(v->items[i], s) == 0) {
            return 1;
        }
    }

    return 0;
}

void string_vec_free(StringVec *v)
{
    size_t i;

    for (i = 0; i < v->len; i++) {
        free(v->items[i]);
    }

    free(v->items);
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

static void report_msg(MessageLevel level,
                       size_t line_no,
                       const char *id,
                       const char *word,
                       const char *gloss,
                       const char *pos,
                       const char *msg)
{
    const char *tag = (level == MSG_ERROR) ? "ERROR" : "WARN";

    if (level == MSG_ERROR) {
        errors++;
    } else {
        warnings++;
    }

    fprintf(stderr,
            "%s\tline=%zu\tid=%s\tword=%s\tgloss=%s\tpos=%s\t%s\n",
            tag,
            line_no,
            id ? id : "-",
            word ? word : "-",
            gloss ? gloss : "-",
            pos ? pos : "-",
            msg);
}

static int split_tsv4(char *line, char **f0, char **f1, char **f2, char **f3)
{
    char *p;
    char *fields[4];
    int count = 0;

    fields[count++] = line;

    for (p = line; *p; p++) {
        if (*p == '\t') {
            *p = '\0';
            if (count < 4) {
                fields[count++] = p + 1;
            } else {
                return -1;
            }
        }
    }

    if (count != 4) {
        return 0;
    }

    *f0 = fields[0];
    *f1 = fields[1];
    *f2 = fields[2];
    *f3 = fields[3];

    return 1;
}

static int load_labels(const char *path, StringVec *labels)
{
    FILE *fp;
    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;

    fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "fatal: cannot open label file '%s': %s\n",
                path, strerror(errno));
        return -1;
    }

    while ((nread = getline(&line, &cap, fp)) != -1) {
        char *s;
        (void)nread;

        rstrip(line);
        s = trim_in_place(line);

        if (*s == '\0' || *s == '#') {
            continue;
        }

        if (!string_vec_contains(labels, s)) {
            string_vec_push_owned(labels, xstrdup(s));
        }
    }

    free(line);
    fclose(fp);
    return 0;
}

static void read_records(FILE *fp,
                         RecordVec *records,
                         const StringVec *labels,
                         const Options *opt)
{
    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    size_t line_no = 0;

    while ((nread = getline(&line, &cap, fp)) != -1) {
        char *id;
        char *word;
        char *gloss;
        char *pos;
        int split_result;
        Record r;

        (void)nread;
        line_no++;
        rstrip(line);

        if (line[0] == '\0') {
            continue;
        }

        split_result = split_tsv4(line, &id, &word, &gloss, &pos);

        if (split_result != 1) {
            report_msg(MSG_ERROR, line_no, "-", "-", "-", "-",
                       "malformed TSV line: expected 4 fields");
            continue;
        }

        if (*id == '\0') {
            report_msg(MSG_ERROR, line_no, id, word, gloss, pos,
                       "empty id");
        }

        if (*word == '\0') {
            report_msg(MSG_ERROR, line_no, id, word, gloss, pos,
                       "empty word");
        }

        if (*gloss == '\0') {
            report_msg(MSG_ERROR, line_no, id, word, gloss, pos,
                       "empty gloss");
        }

        if (*pos == '\0') {
            report_msg(MSG_ERROR, line_no, id, word, gloss, pos,
                       "empty pos");
        }

        if (labels->len > 0 && *gloss != '\0' &&
            !string_vec_contains(labels, gloss)) {
            report_msg(opt->unknown_error ? MSG_ERROR : MSG_WARN,
                       line_no, id, word, gloss, pos,
                       "unknown gloss label");
        }

        r.id = xstrdup(id);
        r.word = xstrdup(word);
        r.gloss = xstrdup(gloss);
        r.pos = xstrdup(pos);
        r.line_no = line_no;

        record_vec_push(records, r);
    }

    free(line);
}

static int gloss_seen_for_word(const RecordVec *records,
                               const char *word,
                               const char *gloss,
                               size_t before_index)
{
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
                                           const char *word,
                                           const char *gloss,
                                           size_t before_index)
{
    size_t i;

    for (i = 0; i < before_index; i++) {
        if (strcmp(records->items[i].word, word) == 0 &&
            strcmp(records->items[i].gloss, gloss) != 0) {
            return 1;
        }
    }

    return 0;
}

static void print_gloss_set_for_word(const RecordVec *records,
                                     const char *word)
{
    size_t i;
    int first = 1;

    fprintf(stderr, "glosses=");

    for (i = 0; i < records->len; i++) {
        if (strcmp(records->items[i].word, word) == 0) {
            size_t j;
            int seen = 0;

            for (j = 0; j < i; j++) {
                if (strcmp(records->items[j].word, word) == 0 &&
                    strcmp(records->items[j].gloss,
                           records->items[i].gloss) == 0) {
                    seen = 1;
                    break;
                }
            }

            if (!seen) {
                if (!first) {
                    fprintf(stderr, "/");
                }
                fprintf(stderr, "%s", records->items[i].gloss);
                first = 0;
            }
        }
    }
}

static void check_variation(const RecordVec *records)
{
    size_t i;

    for (i = 0; i < records->len; i++) {
        const Record *r = &records->items[i];

        if (*r->word == '\0' || *r->gloss == '\0') {
            continue;
        }

        if (!gloss_seen_for_word(records, r->word, r->gloss, i) &&
            word_has_different_gloss_before(records, r->word, r->gloss, i)) {
            warnings++;
            fprintf(stderr,
                    "WARN\tline=%zu\tid=%s\tword=%s\tgloss=%s\tpos=%s\tunstable gloss usage\t",
                    r->line_no, r->id, r->word, r->gloss, r->pos);
            print_gloss_set_for_word(records, r->word);
            fprintf(stderr, "\n");
        }
    }
}

static void usage(FILE *out)
{
    fprintf(out,
            "glosslint %s\n"
            "usage: glosslint [options] < word-gloss.tsv\n\n"
            "Input format:\n"
            "  id<TAB>word<TAB>gloss<TAB>pos\n\n"
            "Options:\n"
            "  -l, --labels FILE      allowed gloss label list\n"
            "      --unknown-error    treat unknown labels as errors\n"
            "  -q, --quiet            print only summary\n"
            "  -h, --help             show this help\n"
            "  -v, --version          show version\n",
            GLOSSLINT_VERSION);
}

static void parse_options(int argc, char **argv, Options *opt)
{
    static struct option long_opts[] = {
        {"labels",        required_argument, 0, 'l'},
        {"unknown-error", no_argument,       0,  1 },
        {"quiet",         no_argument,       0, 'q'},
        {"help",          no_argument,       0, 'h'},
        {"version",       no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int c;

    opt->quiet = 0;
    opt->unknown_error = 0;
    opt->label_file = NULL;

    while ((c = getopt_long(argc, argv, "l:qhv", long_opts, NULL)) != -1) {
        switch (c) {
        case 'l':
            opt->label_file = optarg;
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

int main(int argc, char **argv)
{
    Options opt;
    RecordVec records;
    StringVec labels;

    parse_options(argc, argv, &opt);

    record_vec_init(&records);
    string_vec_init(&labels);

    if (opt.label_file) {
        if (load_labels(opt.label_file, &labels) != 0) {
            string_vec_free(&labels);
            record_vec_free(&records);
            return 2;
        }
    }

    read_records(stdin, &records, &labels, &opt);
    check_variation(&records);

    if (!opt.quiet) {
        fprintf(stderr,
                "SUMMARY\trecords=%zu\terrors=%zu\twarnings=%zu\n",
                records.len, errors, warnings);
    }

    string_vec_free(&labels);
    record_vec_free(&records);

    return errors ? 1 : 0;
}

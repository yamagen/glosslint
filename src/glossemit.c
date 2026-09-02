#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLOSSEMIT_VERSION "0.2.0"

typedef enum {
    MODE_LATEX,
    MODE_HTML
} emit_mode_t;

static void usage(FILE *fp, const char *prog)
{
    fprintf(fp,
            "usage: %s -c FILE [--latex|--html]\n"
            "\n"
            "Read one JSON object per line from standard input and emit\n"
            "formatted glosses to standard output according to FILE.\n"
            "\n"
            "options:\n"
            "  -c, --control-in FILE  controlled vocabulary / emit config\n"
            "      --latex            use emit.latex (default)\n"
            "      --html             use emit.html\n"
            "  -h, --help             show this help\n"
            "  -v, --version          show version\n",
            prog);
}

static size_t split_length(const char *value, const char *split)
{
    const char *cut;

    if (split != NULL && *split != '\0' &&
        (cut = strstr(value, split)) != NULL)
        return (size_t)(cut - value);

    return strlen(value);
}

static void emit_plain_value(const char *value, const char *split)
{
    fwrite(value, 1, split_length(value, split), stdout);
}

static void emit_html_escaped_n(const char *value, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++) {
        switch (value[i]) {
        case '&':
            fputs("&amp;", stdout);
            break;
        case '<':
            fputs("&lt;", stdout);
            break;
        case '>':
            fputs("&gt;", stdout);
            break;
        case '"':
            fputs("&quot;", stdout);
            break;
        case '\'':
            fputs("&#39;", stdout);
            break;
        default:
            fputc((unsigned char)value[i], stdout);
            break;
        }
    }
}

static void emit_html_value(const char *value, const char *split)
{
    emit_html_escaped_n(value, split_length(value, split));
}

static int emit_string_array(json_t *array, const char *path)
{
    size_t i;
    json_t *item;

    if (!json_is_array(array)) {
        fprintf(stderr, "glossemit: %s must be an array\n", path);
        return -1;
    }

    json_array_foreach(array, i, item) {
        if (!json_is_string(item)) {
            fprintf(stderr, "glossemit: %s[%zu] must be a string\n",
                    path, i);
            return -1;
        }
        puts(json_string_value(item));
    }

    return 0;
}

static int emit_latex_tier(json_t *word_gloss, json_t *spec,
                           size_t spec_index)
{
    json_t *tag_obj = json_object_get(spec, "tag");
    json_t *command_obj = json_object_get(spec, "command");
    json_t *split_obj = json_object_get(spec, "split");
    const char *tag;
    const char *command;
    const char *split = NULL;
    size_t i;
    json_t *token;

    if (!json_is_string(tag_obj) || !json_is_string(command_obj)) {
        fprintf(stderr,
                "glossemit: emit.latex.lines[%zu] needs string tag and command\n",
                spec_index);
        return -1;
    }

    if (split_obj != NULL) {
        if (!json_is_string(split_obj)) {
            fprintf(stderr,
                    "glossemit: emit.latex.lines[%zu].split must be a string\n",
                    spec_index);
            return -1;
        }
        split = json_string_value(split_obj);
    }

    tag = json_string_value(tag_obj);
    command = json_string_value(command_obj);

    fputs(command, stdout);

    json_array_foreach(word_gloss, i, token) {
        json_t *value_obj;

        if (!json_is_object(token)) {
            fprintf(stderr,
                    "glossemit: word-gloss[%zu] must be an object\n", i);
            return -1;
        }

        value_obj = json_object_get(token, tag);
        if (!json_is_string(value_obj)) {
            fprintf(stderr,
                    "glossemit: word-gloss[%zu].%s must be a string\n",
                    i, tag);
            return -1;
        }

        fputc(' ', stdout);
        emit_plain_value(json_string_value(value_obj), split);
    }

    fputs(" //\n", stdout);
    return 0;
}

static int emit_latex_translation(json_t *record, json_t *translation)
{
    json_t *tags;
    json_t *command_obj;
    const char *command;
    size_t i;
    json_t *tag_obj;

    if (translation == NULL)
        return 0;

    if (!json_is_object(translation)) {
        fprintf(stderr, "glossemit: emit.latex.translation must be an object\n");
        return -1;
    }

    tags = json_object_get(translation, "tags");
    command_obj = json_object_get(translation, "command");
    if (!json_is_array(tags) || !json_is_string(command_obj)) {
        fprintf(stderr,
                "glossemit: emit.latex.translation needs tags array and command\n");
        return -1;
    }

    command = json_string_value(command_obj);

    json_array_foreach(tags, i, tag_obj) {
        json_t *value_obj;

        if (!json_is_string(tag_obj)) {
            fprintf(stderr,
                    "glossemit: emit.latex.translation.tags[%zu] must be a string\n",
                    i);
            return -1;
        }

        value_obj = json_object_get(record, json_string_value(tag_obj));
        if (json_is_string(value_obj) && json_string_length(value_obj) > 0) {
            printf("%s %s //\n", command, json_string_value(value_obj));
            return 0;
        }
    }

    return 0;
}

static int emit_latex_record(json_t *record, json_t *latex)
{
    json_t *begin;
    json_t *end;
    json_t *lines;
    json_t *translation;
    json_t *word_gloss;
    size_t i;
    json_t *spec;

    if (!json_is_object(record)) {
        fprintf(stderr, "glossemit: input record must be a JSON object\n");
        return -1;
    }

    word_gloss = json_object_get(record, "word-gloss");
    if (!json_is_array(word_gloss)) {
        fprintf(stderr, "glossemit: input record needs word-gloss array\n");
        return -1;
    }

    begin = json_object_get(latex, "begin");
    end = json_object_get(latex, "end");
    lines = json_object_get(latex, "lines");
    translation = json_object_get(latex, "translation");

    if (!json_is_array(lines)) {
        fprintf(stderr, "glossemit: emit.latex.lines must be an array\n");
        return -1;
    }

    if (emit_string_array(begin, "emit.latex.begin") != 0)
        return -1;

    json_array_foreach(lines, i, spec) {
        if (!json_is_object(spec)) {
            fprintf(stderr,
                    "glossemit: emit.latex.lines[%zu] must be an object\n", i);
            return -1;
        }
        if (emit_latex_tier(word_gloss, spec, i) != 0)
            return -1;
    }

    if (emit_latex_translation(record, translation) != 0)
        return -1;

    if (emit_string_array(end, "emit.latex.end") != 0)
        return -1;

    return 0;
}

static int valid_html_element(const char *name)
{
    const unsigned char *p = (const unsigned char *)name;

    if (*p == '\0' || !isalpha(*p))
        return 0;
    p++;
    while (*p != '\0') {
        if (!isalnum(*p) && *p != '-' && *p != ':')
            return 0;
        p++;
    }
    return 1;
}

static int html_spec(json_t *spec, const char *path,
                     const char **element, const char **class_name)
{
    json_t *element_obj;
    json_t *class_obj;

    if (!json_is_object(spec)) {
        fprintf(stderr, "glossemit: %s must be an object\n", path);
        return -1;
    }

    element_obj = json_object_get(spec, "element");
    class_obj = json_object_get(spec, "class");

    if (!json_is_string(element_obj) ||
        !valid_html_element(json_string_value(element_obj))) {
        fprintf(stderr, "glossemit: %s.element must be a valid element name\n",
                path);
        return -1;
    }
    if (class_obj != NULL && !json_is_string(class_obj)) {
        fprintf(stderr, "glossemit: %s.class must be a string\n", path);
        return -1;
    }

    *element = json_string_value(element_obj);
    *class_name = class_obj != NULL ? json_string_value(class_obj) : NULL;
    return 0;
}

static int html_open(json_t *spec, const char *path, int indent,
                     const char **element_out)
{
    const char *element;
    const char *class_name;

    if (html_spec(spec, path, &element, &class_name) != 0)
        return -1;

    printf("%*s<%s", indent, "", element);
    if (class_name != NULL && *class_name != '\0') {
        fputs(" class=\"", stdout);
        emit_html_escaped_n(class_name, strlen(class_name));
        fputc('"', stdout);
    }
    fputc('>', stdout);

    if (element_out != NULL)
        *element_out = element;
    return 0;
}

static void html_close(const char *element)
{
    printf("</%s>", element);
}

static int emit_html_token_line(json_t *token, json_t *spec,
                                size_t token_index, size_t spec_index,
                                int indent)
{
    json_t *tag_obj;
    json_t *split_obj;
    json_t *value_obj;
    const char *tag;
    const char *split = NULL;
    const char *element;
    char path[96];

    snprintf(path, sizeof(path), "emit.html.lines[%zu]", spec_index);

    tag_obj = json_object_get(spec, "tag");
    if (!json_is_string(tag_obj)) {
        fprintf(stderr, "glossemit: %s.tag must be a string\n", path);
        return -1;
    }
    tag = json_string_value(tag_obj);

    split_obj = json_object_get(spec, "split");
    if (split_obj != NULL) {
        if (!json_is_string(split_obj)) {
            fprintf(stderr, "glossemit: %s.split must be a string\n", path);
            return -1;
        }
        split = json_string_value(split_obj);
    }

    value_obj = json_object_get(token, tag);
    if (!json_is_string(value_obj)) {
        fprintf(stderr,
                "glossemit: word-gloss[%zu].%s must be a string\n",
                token_index, tag);
        return -1;
    }

    if (html_open(spec, path, indent, &element) != 0)
        return -1;
    emit_html_value(json_string_value(value_obj), split);
    html_close(element);
    fputc('\n', stdout);
    return 0;
}

static int emit_html_translation(json_t *record, json_t *translation,
                                 int indent)
{
    json_t *tags;
    size_t i;
    json_t *tag_obj;
    const char *element;

    if (translation == NULL)
        return 0;
    if (!json_is_object(translation)) {
        fprintf(stderr, "glossemit: emit.html.translation must be an object\n");
        return -1;
    }

    tags = json_object_get(translation, "tags");
    if (!json_is_array(tags)) {
        fprintf(stderr, "glossemit: emit.html.translation.tags must be an array\n");
        return -1;
    }

    json_array_foreach(tags, i, tag_obj) {
        json_t *value_obj;

        if (!json_is_string(tag_obj)) {
            fprintf(stderr,
                    "glossemit: emit.html.translation.tags[%zu] must be a string\n",
                    i);
            return -1;
        }

        value_obj = json_object_get(record, json_string_value(tag_obj));
        if (json_is_string(value_obj) && json_string_length(value_obj) > 0) {
            if (html_open(translation, "emit.html.translation", indent,
                          &element) != 0)
                return -1;
            emit_html_value(json_string_value(value_obj), NULL);
            html_close(element);
            fputc('\n', stdout);
            return 0;
        }
    }

    return 0;
}

static int emit_html_record(json_t *record, json_t *html)
{
    json_t *container;
    json_t *tokens_spec;
    json_t *token_spec;
    json_t *lines;
    json_t *translation;
    json_t *word_gloss;
    const char *container_element;
    const char *tokens_element;
    size_t i;
    json_t *token;

    if (!json_is_object(record)) {
        fprintf(stderr, "glossemit: input record must be a JSON object\n");
        return -1;
    }

    word_gloss = json_object_get(record, "word-gloss");
    if (!json_is_array(word_gloss)) {
        fprintf(stderr, "glossemit: input record needs word-gloss array\n");
        return -1;
    }

    container = json_object_get(html, "container");
    tokens_spec = json_object_get(html, "tokens");
    token_spec = json_object_get(html, "token");
    lines = json_object_get(html, "lines");
    translation = json_object_get(html, "translation");

    if (!json_is_array(lines)) {
        fprintf(stderr, "glossemit: emit.html.lines must be an array\n");
        return -1;
    }

    if (html_open(container, "emit.html.container", 0,
                  &container_element) != 0)
        return -1;
    fputc('\n', stdout);

    if (html_open(tokens_spec, "emit.html.tokens", 2,
                  &tokens_element) != 0)
        return -1;
    fputc('\n', stdout);

    json_array_foreach(word_gloss, i, token) {
        const char *token_element;
        size_t j;
        json_t *spec;

        if (!json_is_object(token)) {
            fprintf(stderr, "glossemit: word-gloss[%zu] must be an object\n", i);
            return -1;
        }

        if (html_open(token_spec, "emit.html.token", 4,
                      &token_element) != 0)
            return -1;
        fputc('\n', stdout);

        json_array_foreach(lines, j, spec) {
            if (!json_is_object(spec)) {
                fprintf(stderr,
                        "glossemit: emit.html.lines[%zu] must be an object\n", j);
                return -1;
            }
            if (emit_html_token_line(token, spec, i, j, 6) != 0)
                return -1;
        }

        printf("    </%s>\n", token_element);
    }

    printf("  </%s>\n", tokens_element);

    if (emit_html_translation(record, translation, 2) != 0)
        return -1;

    printf("</%s>\n", container_element);
    return 0;
}

int main(int argc, char **argv)
{
    const char *control_path = NULL;
    json_t *control;
    json_t *emit;
    json_t *format;
    json_error_t error;
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    unsigned long line_no = 0;
    int status = EXIT_SUCCESS;
    int opt;
    emit_mode_t mode = MODE_LATEX;
    int mode_seen = 0;

    static const struct option long_options[] = {
        {"control-in", required_argument, NULL, 'c'},
        {"latex", no_argument, NULL, 1000},
        {"html", no_argument, NULL, 1001},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "c:hv", long_options, NULL)) != -1) {
        switch (opt) {
        case 'c':
            control_path = optarg;
            break;
        case 1000:
            if (mode_seen && mode != MODE_LATEX) {
                fprintf(stderr, "glossemit: --latex and --html are exclusive\n");
                return EXIT_FAILURE;
            }
            mode = MODE_LATEX;
            mode_seen = 1;
            break;
        case 1001:
            if (mode_seen && mode != MODE_HTML) {
                fprintf(stderr, "glossemit: --latex and --html are exclusive\n");
                return EXIT_FAILURE;
            }
            mode = MODE_HTML;
            mode_seen = 1;
            break;
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        case 'v':
            printf("glossemit %s\n", GLOSSEMIT_VERSION);
            return EXIT_SUCCESS;
        default:
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "glossemit: unexpected argument: %s\n", argv[optind]);
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    if (control_path == NULL) {
        fprintf(stderr, "glossemit: -c/--control-in is required\n");
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    control = json_load_file(control_path, 0, &error);
    if (control == NULL) {
        fprintf(stderr, "glossemit: %s:%d:%d: %s\n",
                control_path, error.line, error.column, error.text);
        return EXIT_FAILURE;
    }

    emit = json_object_get(control, "emit");
    format = json_is_object(emit)
             ? json_object_get(emit, mode == MODE_HTML ? "html" : "latex")
             : NULL;
    if (!json_is_object(format)) {
        fprintf(stderr, "glossemit: %s: missing emit.%s object\n",
                control_path, mode == MODE_HTML ? "html" : "latex");
        json_decref(control);
        return EXIT_FAILURE;
    }

    errno = 0;
    while ((len = getline(&line, &cap, stdin)) != -1) {
        json_t *record;

        line_no++;
        if (len == 0)
            continue;
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len == 0)
            continue;

        record = json_loads(line, 0, &error);
        if (record == NULL) {
            fprintf(stderr, "glossemit: stdin:%lu:%d: %s\n",
                    line_no, error.column, error.text);
            status = EXIT_FAILURE;
            continue;
        }

        if ((mode == MODE_HTML
             ? emit_html_record(record, format)
             : emit_latex_record(record, format)) != 0)
            status = EXIT_FAILURE;

        json_decref(record);
    }

    if (ferror(stdin)) {
        fprintf(stderr, "glossemit: read error: %s\n", strerror(errno));
        status = EXIT_FAILURE;
    }

    free(line);
    json_decref(control);
    return status;
}

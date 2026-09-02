#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <getopt.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLOSSEMIT_VERSION "0.1.0"

static void usage(FILE *fp, const char *prog)
{
    fprintf(fp,
            "usage: %s -c FILE [--latex]\n"
            "\n"
            "Read one JSON object per line from standard input and emit\n"
            "formatted glosses to standard output according to FILE.\n"
            "\n"
            "options:\n"
            "  -c, --control-in FILE  controlled vocabulary / emit config\n"
            "      --latex            use emit.latex (default)\n"
            "  -h, --help             show this help\n"
            "  -v, --version          show version\n",
            prog);
}

static int emit_string_array(json_t *array, const char *name)
{
    size_t i;
    json_t *item;

    if (!json_is_array(array)) {
        fprintf(stderr, "glossemit: emit.latex.%s must be an array\n", name);
        return -1;
    }

    json_array_foreach(array, i, item) {
        if (!json_is_string(item)) {
            fprintf(stderr,
                    "glossemit: emit.latex.%s[%zu] must be a string\n",
                    name, i);
            return -1;
        }
        puts(json_string_value(item));
    }

    return 0;
}

static void emit_value(const char *value, const char *split)
{
    const char *cut;

    if (split != NULL && *split != '\0' &&
        (cut = strstr(value, split)) != NULL) {
        fwrite(value, 1, (size_t)(cut - value), stdout);
    } else {
        fputs(value, stdout);
    }
}

static int emit_tier(json_t *word_gloss, json_t *spec, size_t spec_index)
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
        const char *value;

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
        value = json_string_value(value_obj);

        fputc(' ', stdout);
        emit_value(value, split);
    }

    fputs(" //\n", stdout);
    return 0;
}

static int emit_translation(json_t *record, json_t *translation)
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

static int emit_record(json_t *record, json_t *latex)
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

    if (emit_string_array(begin, "begin") != 0)
        return -1;

    json_array_foreach(lines, i, spec) {
        if (!json_is_object(spec)) {
            fprintf(stderr,
                    "glossemit: emit.latex.lines[%zu] must be an object\n", i);
            return -1;
        }
        if (emit_tier(word_gloss, spec, i) != 0)
            return -1;
    }

    if (emit_translation(record, translation) != 0)
        return -1;

    if (emit_string_array(end, "end") != 0)
        return -1;

    return 0;
}

int main(int argc, char **argv)
{
    const char *control_path = NULL;
    json_t *control;
    json_t *emit;
    json_t *latex;
    json_error_t error;
    char *line = NULL;
    size_t cap = 0;
    ssize_t len;
    unsigned long line_no = 0;
    int status = EXIT_SUCCESS;
    int opt;

    static const struct option long_options[] = {
        {"control-in", required_argument, NULL, 'c'},
        {"latex", no_argument, NULL, 1000},
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
    latex = json_is_object(emit) ? json_object_get(emit, "latex") : NULL;
    if (!json_is_object(latex)) {
        fprintf(stderr,
                "glossemit: %s: missing emit.latex object\n", control_path);
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

        if (emit_record(record, latex) != 0)
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

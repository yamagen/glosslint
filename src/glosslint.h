#ifndef GLOSSLINT_H
#define GLOSSLINT_H

#include <stddef.h>

#define GLOSSLINT_VERSION "0.1.0"

typedef enum {
    MSG_ERROR = 1,
    MSG_WARN  = 2
} MessageLevel;

typedef struct {
    char *id;
    char *word;
    char *gloss;
    char *pos;
    size_t line_no;
} Record;

typedef struct {
    Record *items;
    size_t len;
    size_t cap;
} RecordVec;

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} StringVec;

typedef struct {
    int quiet;
    int unknown_error;
    const char *label_file;
} Options;

void record_vec_init(RecordVec *v);
void record_vec_push(RecordVec *v, Record r);
void record_vec_free(RecordVec *v);

void string_vec_init(StringVec *v);
void string_vec_push_owned(StringVec *v, char *s);
int  string_vec_contains(const StringVec *v, const char *s);
void string_vec_free(StringVec *v);

#endif

#ifndef GLOSSLINT_H
#define GLOSSLINT_H

#include <stddef.h>

#define GLOSSLINT_VERSION "0.1.0"

typedef enum { MSG_ERROR = 1, MSG_WARN = 2 } MessageLevel;

typedef struct {
  char *file;
  size_t source_line;
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

typedef enum {
  SCHEMA_OFF = 0,
  SCHEMA_CONTROLLED,
  SCHEMA_STRING,
  SCHEMA_INTEGER,
  SCHEMA_NUMERIC
} SchemaRule;

typedef struct {
  char *name;
  SchemaRule rule;
} SchemaField;

typedef struct {
  SchemaField *items;
  size_t len;
  size_t cap;
} SchemaVec;

typedef struct {
  StringVec conjugation;
  StringVec pos;
  StringVec gloss;
  SchemaVec schema;
} ControlSet;

typedef struct {
  int quiet;
  int unknown_error;
  const char *control_file;
} Options;

void record_vec_init(RecordVec *v);
void record_vec_push(RecordVec *v, Record r);
void record_vec_free(RecordVec *v);

void string_vec_init(StringVec *v);
void string_vec_push_owned(StringVec *v, char *s);
int string_vec_contains(const StringVec *v, const char *s);
void string_vec_free(StringVec *v);

void schema_vec_init(SchemaVec *v);
void schema_vec_push_owned(SchemaVec *v, char *name, SchemaRule rule);
void schema_vec_free(SchemaVec *v);

void control_set_init(ControlSet *c);
void control_set_free(ControlSet *c);

typedef enum {
  SEC_NONE = 0,
  SEC_CONJUGATION,
  SEC_POS,
  SEC_GLOSS,
  SEC_SCHEMA
} ControlSection;

#endif

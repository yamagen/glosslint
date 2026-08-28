# Visual Studio Code Integration

`glosslint` emits diagnostics in the form:

```text
file:line:column: warning: message
file:line:column: error: message
```

Visual Studio Code can read these diagnostics with a task and a problem matcher.

## Example `.vscode/tasks.json`

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "glosslint",
      "type": "shell",
      "command": "/path/to/glosslint-json",
      "args": ["/path/to/controlled-ja-en.json", "${file}"],
      "problemMatcher": {
        "owner": "glosslint",
        "pattern": {
          "regexp": "^(.+):(\\d+):(\\d+): (warning|error): (.*)$",
          "file": 1,
          "line": 2,
          "column": 3,
          "severity": 4,
          "message": 5
        }
      }
    }
  ]
}
```

Run the task from:

```text
Terminal → Run Task... → glosslint
```

Diagnostics will appear in the Problems panel. Selecting a problem jumps to the corresponding source line.

The paths to `glosslint-json` and the control file should be adjusted for the local installation.

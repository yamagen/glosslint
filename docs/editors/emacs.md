# Emacs Integration

`glosslint` emits diagnostics in the form:

```text
file:line:column: warning: message
file:line:column: error: message
```

Emacs can use these diagnostics through `compilation-mode`.

## Example

```elisp
(defun glosslint-buffer ()
  (interactive)
  (let ((control "/path/to/controlled-ja-en.json")
        (runner "/path/to/glosslint-json"))
    (compile
     (format "%s %s %s"
             (shell-quote-argument runner)
             (shell-quote-argument control)
             (shell-quote-argument buffer-file-name)))))
```

Run:

```text
M-x glosslint-buffer
```

The output appears in a compilation buffer. Diagnostic locations can be followed with the usual compilation commands.

If necessary, a project-specific `compilation-error-regexp-alist` entry can be added for the exact `glosslint` diagnostic format.

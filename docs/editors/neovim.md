# Neovim Integration

`glosslint` emits diagnostics in the form:

```text
file:line:column: warning: message
file:line:column: error: message
```

Neovim can read these diagnostics into the quickfix list.

## Example

```lua
local function run_glosslint()
  local file = vim.fn.expand("%:p")
  local control = vim.fn.expand("~/path/to/glosslint/controlled-ja-en.json")
  local runner = vim.fn.expand("~/path/to/glosslint/bin/glosslint-json")

  local cmd = string.format(
    "%q %q %q 2>/dev/null",
    runner,
    control,
    file
  )

  local output = vim.fn.system(cmd)

  vim.fn.setqflist({}, " ", {
    title = "glosslint",
    lines = vim.split(output, "\n", { trimempty = true }),
    efm = "%f:%l:%c: %t%*[^:]: %m",
  })
end

vim.api.nvim_create_user_command("Glosslint", function()
  run_glosslint()
  vim.cmd("copen")
end, {})

vim.api.nvim_create_autocmd("BufWritePost", {
  pattern = "*.json",
  callback = function()
    run_glosslint()
  end,
})
```

A convenient location is:

```text
~/.config/nvim/plugin/glosslint.lua
```

Then run:

```vim
:Glosslint
```

or simply save a JSON file.

The workflow becomes:

```text
save
→ check
→ quickfix
→ jump
→ correct
→ save again
```

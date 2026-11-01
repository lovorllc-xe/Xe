# Xe Language Support for Visual Studio Code

Basic language support for Xe source files (`.xe`) and Xe library files
(`.xel`). It provides syntax coloring, line comments, bracket matching, and
starter snippets. It does not include the Xe compiler or language server.

The optional **Xe Vibrant** color theme uses green strings and cycles nested
brackets through green, cyan, orange, purple, pink, and yellow. The bracket
colors change with nesting depth; for example, an outer `{}` pair and a nested
`{}` pair intentionally receive different colors so their matching braces are
easy to distinguish.

To enable it, open the Command Palette, choose **Preferences: Color Theme**, and
select **Xe Vibrant**. Other themes can use different syntax and bracket colors.

## Install from this folder

Copy or symlink this directory into your VS Code extensions folder, then reload
VS Code:

- Linux: `~/.vscode/extensions/xe-language-support`
- Windows: `%USERPROFILE%\\.vscode\\extensions\\xe-language-support`
- macOS: `~/.vscode/extensions/xe-language-support`

Alternatively, open this directory in VS Code and use **Developer: Install
Extension from Location...** if that command is available in your VS Code build.

## Snippets

Type one of these prefixes in an Xe file and accept the completion:

| Prefix | Inserts |
| --- | --- |
| `main` | `fn main(): int` entry point |
| `fn` | Function declaration |
| `module` | Module declaration |
| `ifelse` | `if` / `else` blocks |
| `while` | `while` loop |
| `var` | Mutable variable |
| `const` | Immutable binding |
| `uses` | `.xel` library import |
| `record` | Record type declaration |
| `writeln` | Line output call |

## Xe syntax covered

The grammar highlights Xe declarations, control flow, primitive types, numeric
and string literals, comments, operators, and inline assembly blocks. It follows
the syntax used by the Xe compiler sources; highlighting does not validate
whether a program compiles.

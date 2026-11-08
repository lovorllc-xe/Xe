---
name: xe-language
description: Write, explain, debug, and review programs in the Xe programming language. Use when a request involves `.xe` source, `.xel` libraries, Xe compiler behavior, or Xe examples.
---

# Xe Language

Help the user write Xe that matches the compiler and libraries in their current project. For syntax, library APIs, and compiler behavior, treat the checked-out Xe source and installed `xe --help` as authoritative; documentation and examples can lag behind. Do not invent syntax or library functions. Read [the Xe language guide](references/language-guide.md) for the current conventions and known limits.

When adding or changing a program:

1. Follow the user's requested scope and keep the result in Xe unless they ask for another language.
2. Check the actual `.xel` declaration before using a library function. Imports use `uses name`; `io` and `mem` are included automatically.
3. Give the program a `fn main(): int` entry point when it should build as an executable, and return an exit code.
4. If verification is requested or useful to the task, build with `xe --build source.xe -o output.xee` and run `./output.xee`. Never use the removed `xe --run` option. Do not claim a build passed unless it was run.
5. For compiler implementation work, preserve Xe's self-hosted compiler path and native code generation; do not replace it with a transpiler to C or C++.

Keep explanations concrete. Mention platform or library limits when they affect the requested program, and distinguish supported behavior from a workaround or unverified assumption.

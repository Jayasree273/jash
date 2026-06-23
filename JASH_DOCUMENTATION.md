# JASH (Just Alquimist SHell) - Technical Manual

## Table of Contents
1. [Overview](#1-overview)
2. [Architecture](#2-architecture)
3. [End-to-End Execution Flow](#3-end-to-end-execution-flow)
4. [Component Details](#4-component-details)
5. [Process Lifecycle](#5-process-lifecycle)
6. [Memory Management](#6-memory-management)
7. [Environment Handling](#7-environment-handling)

---

## 1. Overview
JASH is a minimal, POSIX-compliant shell implemented in C. It serves as a tool for understanding systems engineering, process lifecycle management, and Linux/Unix internals. It supports essential shell features including variable expansion, command redirection, and built-in utility management.

---

## 2. Architecture
JASH follows a modular pipeline design, ensuring a clean separation between raw input processing and OS-level execution:

* **`src/lexer/`**: Tokenizes input strings with quote/escape awareness.
* **`src/expand/`**: Processes `$`, `${}`, `$?`, and `~` expansions.
* **`src/parser/`**: Structures tokens into commands, separating arguments from operators.
* **`src/planner/`**: Determines the execution strategy (Built-in vs. External).
* **`src/exec/`**: Manages process orchestration via `fork`/`exec`.
* **`src/env/`**: Manages the internal shell environment table.

---

## 3. End-to-End Execution Flow
Every command typed into the `jash$` prompt follows this sequence in `main.c`:

1. **Input**: `getline()` captures the user command.
2. **Lexing**: `jash_lex()` creates a token stream.
3. **Expansion**: `jash_expand()` replaces variables with actual values based on the current environment.
4. **Parsing**: `jash_parse()` builds a `jash_command_t` structure, isolating redirections from arguments.
5. **Planning**: `jash_plan()` resolves the command name and assigns execution parameters.
6. **Execution**: `jash_exec_run()` invokes the engine.
7. **Cleanup**: All allocated structures (tokens, commands, plans) are freed to prevent memory leaks.

---

## 4. Component Details

### The Lexer
The lexer handles three types of tokens:
- **Words**: Standard strings, potentially containing quotes.
- **Operators**: `>`, `>>`, `<`.
- **EOL**: Represents the end of the input line.

### The Parser
The parser uses a two-pass strategy:
1. **Count Pass**: Determines the number of arguments and redirections to allocate memory accurately.
2. **Populate Pass**: Dupes strings into the `argv` array and structures the redirection instructions.

---

## 5. Process Lifecycle

### External Commands
1. **Fork**: The shell clones itself.
2. **Redirection**: The child calls `apply_redirections()`, which opens target files and uses `dup2()` to overwrite standard file descriptors (`stdin`/`stdout`).
3. **Exec**: The child calls `execvp()`, which replaces the process image with the target binary.
4. **Wait**: The parent waits via `waitpid()`, collecting the exit status or signal code.

### Built-in Commands
Built-ins (e.g., `cd`, `exit`) are executed directly by the shell to allow them to affect the shell's process state. If a built-in requires redirection, JASH forks the process, performs the redirection in the child, executes the builtin, and exits the child—leaving the parent shell's state intact.

---

## 6. Memory Management
JASH relies on consistent cleanup routines. Each module provides a dedicated `free` function:
- `jash_token_stream_free()`
- `jash_command_free()`
- `jash_exec_plan_free()`
- `jash_env_free()`

*Developers are responsible for ensuring that any `malloc` in a new feature is registered in the corresponding free function.*

---

## 7. Environment Handling
The environment is tracked in `jash_env_table_t`. 
- **Lookup**: Uses `jash_env_get()` to retrieve values for expansion.
- **Modification**: `jash_env_set()` manages both creation and updates, tracking whether a variable should be exported to child processes.
- **Unsetting**: `jash_env_unset()` cleanly removes variables and shrinks the environment array.

---
*Document generated for JASH version 1.0.0*

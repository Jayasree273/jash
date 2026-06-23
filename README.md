# JASH - Just Another Shell

A minimal but functional shell implementation in C, implementing core POSIX shell features.

## Features

- **Lexing & Parsing**: Quote handling (single/double quotes), variable expansion
- **Variable Expansion**: `$NAME`, `${NAME}`, `$?`, `~` (home directory)
- **Redirections**: `>` (write), `>>` (append), `<` (read)
- **Builtins**: `pwd`, `cd`, `exit`, `export`, `unset`, `env`, `echo`
- **External Commands**: Full fork/exec/wait support with PATH resolution
- **Environment**: Full environment variable management with export tracking

## Building

```bash
make clean && make
./bin/jash

## Usage

# Variables
export FOO=hello
echo $FOO                    # prints: hello

# Quoting
echo 'no expansion $FOO'     # prints: no expansion $FOO
echo "with expansion $FOO"   # prints: with expansion hello

# Redirections
echo "data" > /tmp/file.txt  # write
cat /tmp/file.txt
echo "more" >> /tmp/file.txt # append
cat < /tmp/file.txt          # read

# Builtins
pwd                          # print working directory
cd /tmp                      # change directory
export BAR=world             # set and export variable
env                          # list exported variables
unset FOO                    # remove variable
exit                         # exit shell

```

## Architecture

- `src/lexer/`: Tokenization with quote awareness
- `src/expand/`: Variable and tilde expansion
- `src/parser/`: Command parsing with redirection extraction
- `src/planner/`: Command planning (builtin vs external)
- `src/exec/`: Execution engine (fork/exec for external, direct call for builtins)
- `src/builtins/`: Built-in command implementations
- `src/env/`: Environment variable table management

## TODO
- [ ] Pipes (|)
- [ ] Command substitution (`cmd`, $(cmd))
- [ ] Background jobs (&, jobs, fg, bg)
- [ ] More builtins (test, [, true, false, printf)
- [ ] Signal handling (SIGINT, SIGTERM)
- [ ] Command history
- [ ] Globbing (*, ?, [...])

## Requirements
- [ ] POSIX C11 compiler (gcc, clang)
- [ ] Linux/Unix system with standard C library

## License
[MIT License](LICENSE)

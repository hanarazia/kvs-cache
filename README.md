# KVS Cache (C)

The purpose of this project is to implement an in-memory key-value store (KVS) in C with multiple cache eviction policies (LRU, FIFO, CLOCK). The project supports different replacement algorithms through polymorphic C design and allows client interaction through command-line interfaces.

## How to Run Locally (Requires: C Compiler)

1. Clone the repo  
  ```bash
  git clone https://github.com/yourusername/kvs-cache.git
  cd kvs-cache
  ```
2. Build the project
  ```
  make
  ```
3. Run the program with desired flags
  ```
    ./client -i             Starts in interactive mode
    ./client -f <file>      Runs commands from a provided script file
    ./client -h             Displays help/usage information
  ```

---

## Files in Repo

| File            | Description                                                                 |
|-----------------|-----------------------------------------------------------------------------|
| `client.c`      | Main CLI program to interact with the key-value store                       |
| `constants.h`   | Contains configuration macros/constants used throughout the codebase        |
| `kvs.c`         | Core KVS logic that interfaces with eviction policy implementations         |
| `kvs.h`         | Header for shared KVS structure and client-facing API                       |
| `kvs_base.c`    | Abstract base logic shared by all eviction policy implementations           |
| `kvs_base.h`    | Header for base cache logic                                                 |
| `kvs_fifo.c`    | FIFO eviction policy implementation                                         |
| `kvs_fifo.h`    | Header for FIFO policy                                                      |
| `kvs_lru.c`     | LRU (Least Recently Used) eviction policy implementation                    |
| `kvs_lru.h`     | Header for LRU policy                                                       |
| `kvs_clock.c`   | CLOCK eviction policy implementation                                        |
| `kvs_clock.h`   | Header for CLOCK policy                                                     |
| `Makefile`      | Builds the project and applies clang-format to all source files             |

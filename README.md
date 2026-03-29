# Memory-Efficient-Versioned-File-Indexer-
## Approach & Object-Oriented Design
This solution implements a memory-efficient text indexing system using pure C++17 object-oriented principles to ensure modularity and scalability.

1.  **`BufferedFileReader`:** Manages memory constraints by reading large text files in fixed-size chunks (256 KB - 1024 KB limit properly enforced).
2.  **`Tokenizer`:** Parses alphanumeric characters incrementally from the buffer chunk, correctly handling words that are split across chunk sizes by carrying over the `leftover` suffix.
3.  **`VersionedIndex`:** Serves as the frequency dictionary map to handle word tallies logic for specific file versions. Employs function overloading.
4.  **`QueryProcessor`:** An abstract base class demonstrating runtime polymorphism. 
    - Derives `WordQuery`, `TopKQuery`, `DiffQuery`.
    - Dispatches dynamically based on the parsed command-line variables.

## Fulfillment of Constraints
-   **Memory Restrictions:** At no point is the entire file loaded into memory. Chunks are rigidly read to sizes ranging tightly between 256 and 1024 KB.
-   **Exception Handling:** Used extensively to catch missing files, malformed numeric command arguments, and buffer size violations.
-   **Templates:** A custom implementation `template <typename K, typename V> std::vector<std::pair<K, V>> sortMapByValue(...)` is used to rank top-frequency terms seamlessly without relying solely on built-in map iterators. 
-   **Case-Sensitivity:** The index is wholly case-insensitive; every incoming word token is formatted to lowercase prior to dictionary insertion.

## Compilation and Execution

### Compilation
The program relies on standard C++17 features. Compile it using `g++` (or any compatible C++17 compiler):

```bash
g++ -std=c++17 -O3 240338_Dev.cpp -o analyzer
```

### Execution
Run the compiled binary (`analyzer` on Linux/macOS or `analyzer.exe` on Windows) via the command line by providing the required arguments:

**Word Query (Single File):**
```bash
./analyzer --file dataset_v1.txt --version v1 --buffer 512 --query word --word error
```

**Top-K Query (Single File):**
```bash
./analyzer --file dataset_v1.txt --version v1 --buffer 512 --query top --top 10
```

**Difference Query (Two Files):**
```bash
./analyzer --file1 dataset_v1.txt --version1 v1 --file2 dataset_v2.txt --version2 v2 --buffer 512 --query diff --word error
```

**Notes:**
- `--buffer` expects a size in kilobytes (KB) strictly between `256` and `1024`.

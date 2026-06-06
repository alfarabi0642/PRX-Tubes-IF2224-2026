# Arion Compiler - Milestone 1, 2, 3 & 4

**Tugas Besar IF2224 Teori Bahasa Formal dan Otomata 2026**

## Deskripsi Program

Program ini adalah compiler sederhana untuk bahasa Arion.

- **Milestone 1:** lexical analyzer berbasis DFA.
- **Milestone 2:** syntax analyzer berbasis recursive descent parser.
- **Milestone 3:** semantic analyzer berbasis AST, symbol table, dan type checking.
- **Milestone 4:** backend yang mengubah Decorated AST menjadi instruksi stack machine dan mengeksekusinya dengan interpreter.

Alur program untuk Milestone 1 sampai Milestone 3:

```text
Source File
  -> Lexer
  -> Filter komentar/newline
  -> Parser
  -> Parse Tree
  -> AST Builder
  -> Semantic Analyzer
  -> Decorated AST + Symbol Table + Semantic Diagnostics
```

Alur program untuk Milestone 4:

```text
Decorated AST File
  -> Decorated AST Loader
  -> Semantic Metadata Rehydration
  -> Intermediate Code Generator
  -> Stack Machine Instructions
  -> Interpreter
  -> Program Output
```

Untuk Milestone 1 sampai 3, program menampilkan parse tree, decorated AST, tabel
simbol, dan hasil pengecekan semantik. Untuk Milestone 4, sesuai Q&A resmi,
input `.txt` adalah Decorated AST, bukan source code Arion, sehingga lexer dan
parser tidak dijalankan.

Untuk input milestone 3, laporan semantik disimpan ke:

```text
test/milestone-3/<nama_file>_SEMANTIC.txt
```

Untuk input milestone 4, laporan backend disimpan ke:

```text
test/milestone-4/<nama_file>_BACKEND.txt
```

File output Milestone 4 hanya berisi intermediate code dan output aktual program.

## Identitas Kelompok

**Nama Kelompok:** PRX

| Nama | NIM |
| :--- | :--- |
| Al Farabi | 13524086 |
| Ishaq Irfan Farizal | 13524094 |
| Daniel Anindito Nugroho | 13524002 |
| Ishak Palentino | 13524022 |

## Requirements

- `g++` dengan standar C++17.
- `make` atau `mingw32-make`.

## Project Structure

```text
PRX-Tubes-IF2224-2026/
|-- src/
|    │   main.cpp
|    │   
|    ├───common
|    │       token.cpp
|    │       token.hpp
|    │       utils.cpp
|    │       utils.hpp
|    │       
|    ├───lexer
|    │       lexer.cpp
|    │       lexer.hpp
|    │       
|    ├───parser
|    │       parser.hpp
|    │       parser_core.cpp
|    │       parser_declarations.cpp
|    │       parser_expressions.cpp
|    │       parser_statements.cpp
|    │       parser_toplevel.cpp
|    │       parse_tree.hpp
|    │       
|    └───semantic
|            ast.cpp
|            ast.hpp
|            ast_builder.cpp
|            ast_builder.hpp
|            diagnostic.hpp
|            printer.cpp
|            printer.hpp
|            semantic_analyzer.cpp
|            semantic_analyzer.hpp
|            semantic_analyzer_declarations.cpp
|            semantic_analyzer_expressions.cpp
|            semantic_analyzer_statements.cpp
|            symbol_table.cpp
|            symbol_table.hpp
|            types.cpp
|            types.hpp
|-- test/
|   |-- milestone-1/
|   |-- milestone-2/
|   |-- milestone-3/
|   `-- milestone-4/
|-- doc/
|-- bin/
|-- build/
|-- Makefile
`-- README.md
```

## Setup & Compilation

1. Clean build lama:

   ```bash
   make clean
   ```

2. Build program:

   ```bash
   make
   ```

   Jika menggunakan MinGW di Windows:

   ```bash
   mingw32-make
   ```

3. Jalankan program:

   ```bash
   ./bin/arion.exe test/milestone-3/tc1.txt
   ```

   PowerShell:

   ```powershell
   .\bin\arion.exe test\milestone-3\tc1.txt
   ```

## Output Build

- Executable: `bin/arion.exe`
- Object file: `build/obj/...`

File `.o` dan executable hasil build tidak dibuat di root directory.

## Output Program

Untuk Milestone 1 sampai 3, input yang valid secara lexical dan syntax menghasilkan:

- Parse tree hasil parser.
- Decorated AST hasil konversi parse tree dan anotasi semantic analyzer.
- Tabel simbol `tab`, `btab`, dan `atab`.
- Daftar semantic error jika ditemukan pelanggaran semantik.

Untuk Milestone 4, input adalah Decorated AST dalam format tree yang sama dengan
printer semantic:

```text
=== Decorated AST ===
Program(M4DastBasic)
|-- DeclarationPart
|   \-- VarDecl(x) : type=Integer
|       \-- TypeRef(integer) : type=Integer
\-- CompoundStatement
    |-- AssignStatement : type=Integer
    |   |-- Variable(x) : type=Integer
    |   \-- Literal(10) value=10 literal=Integer : type=Integer
    |-- Call(writeln)
    |   \-- Variable(x) : type=Integer
    \-- EmptyStatement
```

Loader Decorated AST akan membangun ulang metadata semantic yang dibutuhkan
backend, termasuk symbol table dan type registry. Karena metadata direkonstruksi
dari deklarasi AST, input DAST harus tetap memuat node deklarasi seperti `VarDecl`
dan `TypeDecl`.

Output backend Milestone 4 menggunakan section stabil berikut:

```text
=== INTERMEDIATE CODE ===
0 INT 0 4
1 LIT 0 10
2 STO 0 3
3 LOD 0 3
4 OPR 0 14
5 RET 0 0

=== OUTPUT ===
10
```

Jika input M4 bukan Decorated AST, berisi node tidak dikenal, atau metadata tidak
dapat direkonstruksi dari deklarasi AST, program menulis `=== BACKEND INPUT ERRORS ===`
dan keluar dengan kode error. Jika code generation gagal, program menulis
`=== BACKEND ERRORS ===`. Jika runtime gagal, program menulis
`=== RUNTIME ERRORS ===`.


## Revisi Milestone 3

Implementasi juga menyesuaikan beberapa revisi dari milestone sebelumnya:

- Comment tidak dijadikan node parse tree untuk tahap parser/semantic.
- Empty statement valid, misalnya `begin ; end`.
- Identifier seperti `x2` tetap dibaca sebagai satu identifier.
- `while` dan `for` wajib menggunakan `compound-statement` setelah `do`.
- Satu semicolon setelah blok `while` atau `for` cukup mengikuti aturan `statement-list`.


## Testing

- Test Milestone 1 ada di `test/milestone-1`.
- Test Milestone 2 ada di `test/milestone-2`.
- Test Milestone 3 ada di `test/milestone-3`.
- Test Milestone 4 ada di `test/milestone-4`.

Contoh menjalankan salah satu test milestone 3:

```powershell
.\bin\arion.exe test\milestone-3\tc1.txt
```

Contoh menjalankan test milestone 4:

```powershell
.\bin\arion.exe test\milestone-4\dast-valid-basic.txt
```



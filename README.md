# Arion Compiler - Milestone 1, 2, 3 & 4

**Tugas Besar IF2224 Teori Bahasa Formal dan Otomata 2026**

## Deskripsi Program

Program ini adalah compiler sederhana untuk bahasa Arion.

- **Milestone 1:** lexical analyzer berbasis DFA.
- **Milestone 2:** syntax analyzer berbasis recursive descent parser.
- **Milestone 3:** semantic analyzer berbasis AST, symbol table, dan type checking.
- **Milestone 4:** backend MVP yang mengubah Decorated AST menjadi instruksi stack machine dan mengeksekusinya dengan interpreter.

Alur program saat ini:

```text
Source File
  -> Lexer
  -> Filter komentar/newline
  -> Parser
  -> Parse Tree
  -> AST Builder
  -> Semantic Analyzer
  -> Decorated AST + Symbol Table + Semantic Diagnostics
  -> Intermediate Code Generator
  -> Stack Machine Instructions
  -> Interpreter
  -> Program Output
```

Program akan menampilkan parse tree, decorated AST, tabel simbol, dan hasil pengecekan
semantik ke terminal. Jika analisis semantik sukses, program juga menampilkan
intermediate code dan output akhir program Arion.

Untuk input milestone 3, laporan semantik disimpan ke:

```text
test/milestone-3/<nama_file>_SEMANTIC.txt
```

Untuk input milestone 4, laporan backend disimpan ke:

```text
test/milestone-4/<nama_file>_BACKEND.txt
```

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

Untuk input yang valid secara lexical dan syntax, program menghasilkan:

- Parse tree hasil parser.
- Decorated AST hasil konversi parse tree dan anotasi semantic analyzer.
- Tabel simbol `tab` untuk identifier, konstanta, variabel, tipe, prosedur, dan fungsi.
- Tabel blok `btab` untuk informasi blok prosedur, fungsi, dan record.
- Tabel array `atab` untuk metadata tipe array.
- Daftar semantic error jika ditemukan pelanggaran semantik.
- Intermediate code dalam format instruksi stack machine, misalnya `INT`, `LIT`,
  `LOD`, `STO`, `JMP`, `JPC`, `OPR`, dan `RET`.
- Output final program Arion dari interpreter.

Jika ditemukan lexical error, proses parser dan semantic analyzer tidak dijalankan.
Jika ditemukan parser error, proses semantic analyzer tidak dijalankan. Jika semantic
error ditemukan, program tetap mencetak laporan semantik lalu keluar dengan kode error.
Backend hanya dijalankan setelah lexical, parser, AST builder, dan semantic analyzer
sukses. Jika code generation gagal, program mencetak `=== Backend Errors ===` dan
interpreter tidak dijalankan. Jika runtime gagal, intermediate code tetap dicetak,
lalu program mencetak `=== Runtime Errors ===`.

Output backend menggunakan section stabil:

```text
=== Intermediate Code ===
0 INT 0 ...
1 LIT 0 ...

=== Program Output ===
...
```

Sesuai QNA Milestone 4, intermediate code yang wajib dicetak dan dieksekusi adalah
instruksi stack machine. Three Address Code dipakai sebagai konsep flattening AST,
bukan format akhir yang dibaca interpreter.

Side-file Milestone 4 memakai suffix `_BACKEND.txt` agar fixture M4 tidak menimpa
snapshot semantic Milestone 3 yang memakai suffix `_SEMANTIC.txt`.

## Cakupan Semantic Analyzer

Implementasi milestone 3 mencakup:

- Konversi parse tree menjadi AST.
- Inisialisasi predefined identifier seperti `Integer`, `Real`, `Char`, `Boolean`,
  `String`, `True`, `False`, `readln`, dan `writeln`.
- Pencatatan deklarasi konstanta, tipe, variabel, prosedur, fungsi, parameter, dan field record.
- Pengecekan deklarasi ulang identifier dalam scope yang sama.
- Lookup identifier dari scope terdalam ke scope luar.
- Pengecekan tipe assignment, ekspresi aritmetika, ekspresi logika, dan operator relasional.
- Pengecekan kondisi `if`, `while`, `repeat until`, `for`, dan `case`.
- Pengecekan akses array dan field record.
- Pengecekan argumen procedure/function call.
- Pengecekan penggunaan variabel lokal sebelum inisialisasi.

## Cakupan Backend Milestone 4

Backend MVP saat ini mencakup:

- Variabel global scalar dan address frame-relative.
- Literal `integer`, `real`, `char`, `boolean`, dan `string`.
- Assignment, ekspresi unary/binary, aritmetika, modulo, comparison, dan boolean
  short-circuit.
- `write` dan `writeln`, termasuk beberapa argumen.
- Control flow `if`, `while`, `repeat until`, dan `for to/downto`.
- Runtime diagnostic untuk stack underflow/overflow frame, invalid address,
  invalid jump, division/modulo by zero, dan guard jumlah langkah interpreter.

Batasan yang masih partial atau belum didukung backend:

- `readln`.
- Array indexing dan record field pada code generation.
- Source-level procedure/function call, termasuk `CAL` runtime.
- `case` statement.
- Lexical level selain 0.

Input yang memakai fitur partial tetap harus gagal dengan diagnostic backend/runtime
yang informatif, bukan crash.

## Revisi Milestone 3

Implementasi juga menyesuaikan beberapa revisi dari milestone sebelumnya:

- Comment tidak dijadikan node parse tree untuk tahap parser/semantic.
- Empty statement valid, misalnya `begin ; end`.
- Identifier seperti `x2` tetap dibaca sebagai satu identifier.
- `while` dan `for` wajib menggunakan `compound-statement` setelah `do`.
- Satu semicolon setelah blok `while` atau `for` cukup mengikuti aturan `statement-list`.

## Pembagian Tugas Milestone 3

| NIM | Tugas |
| :--- | :--- |
| 13524002 | Parser dan lexer revisions untuk Milestone 3, termasuk validasi empty statement, comment filtering serta perubahan while, for agar wajib compound statement, dan dokumen bagian implementasi. |
| 13524022 | Implementasi AST model dan AST Builder, termasuk konversi parse tree menjadi AST ringkas untuk deklarasi, statement, expression, array, record, procedure, function call, dan testing. |
| 13524086 | Implementasi type system dan symbol table (TypeRegistry, tab, btab, atab, predefined identifiers), serta integrasi dan refactoring modul semantic analysis. |
| 13524094 | Implementasi semantic analyzer, decorated AST, printer, diagnostics, integrasi main.cpp, refactoring akhir, dan pengujian Milestone 3. |

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
.\bin\arion.exe test\milestone-4\tc1.txt
```

Smoke test final yang digunakan untuk release candidate:

```powershell
mingw32-make -B
.\bin\arion.exe test\milestone-4\tc1.txt
.\bin\arion.exe test\milestone-4\tc2.txt
.\bin\arion.exe test\milestone-4\tc3.txt
.\bin\arion.exe test\milestone-4\tc4.txt
.\bin\arion.exe test\milestone-4\tc5.txt
.\bin\arion.exe test\milestone-4\tc6.txt
.\bin\arion.exe test\milestone-4\tc7.txt
.\bin\arion.exe test\milestone-4\tc8.txt
.\bin\arion.exe test\milestone-4\tc9.txt
.\bin\arion.exe test\milestone-4\tc10.txt
.\bin\arion.exe test\milestone-3\tc1.txt
.\bin\arion.exe test\milestone-3\tc10.txt
git diff --check
```

`tc1` sampai `tc9` adalah happy-path backend MVP. `tc10` memuat fitur composite,
procedure/function, dan `case`; hasil yang diharapkan untuk saat ini adalah diagnostic
backend yang rapi.

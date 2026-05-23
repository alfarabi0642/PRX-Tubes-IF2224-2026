# Arion Compiler - Milestone 1, 2 & 3

**Tugas Besar IF2224 Teori Bahasa Formal dan Otomata 2026**

## Deskripsi Program

Program ini adalah compiler sederhana untuk bahasa Arion.

- **Milestone 1:** lexical analyzer berbasis DFA.
- **Milestone 2:** syntax analyzer berbasis recursive descent parser.
- **Milestone 3:** semantic analyzer berbasis AST, symbol table, dan type checking.

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
```

Program akan menampilkan parse tree, decorated AST, tabel simbol, dan hasil pengecekan
semantik ke terminal. Laporan semantik juga disimpan ke:

```text
test/milestone-3/<nama_file>_SEMANTIC.txt
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
|   `-- milestone-3/
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

Jika ditemukan lexical error, proses parser dan semantic analyzer tidak dijalankan.
Jika ditemukan parser error, proses semantic analyzer tidak dijalankan. Jika semantic
error ditemukan, program tetap mencetak laporan semantik lalu keluar dengan kode error.

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

Contoh menjalankan salah satu test milestone 3:

```powershell
.\bin\arion.exe test\milestone-3\tc1.txt
```

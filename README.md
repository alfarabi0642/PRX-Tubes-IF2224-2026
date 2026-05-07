# Arion Compiler - Milestone 1 & 2

**Tugas Besar IF2224 Teori Bahasa Formal dan Otomata 2026**

## Deskripsi Program

Program ini adalah compiler sederhana untuk bahasa Arion.

- **Milestone 1:** lexical analyzer berbasis DFA.
- **Milestone 2:** syntax analyzer berbasis recursive descent parser.

Alur program saat ini:

```text
Source File -> Lexer -> Filter komentar/newline -> Parser -> Parse Tree
```

Parse tree akan ditampilkan di terminal dan disimpan ke:

```text
test/milestone-2/<nama_file>_OUTPUT.txt
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
|   |-- main.cpp
|   |-- common/
|   |-- lexer/
|   `-- parser/
|-- test/
|   |-- milestone-1/
|   `-- milestone-2/
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
   ./bin/arion.exe test/milestone-2/phase3_spec_example.txt
   ```

   PowerShell:

   ```powershell
   .\bin\arion.exe test\milestone-2\phase3_spec_example.txt
   ```

## Output Build

- Executable: `bin/arion.exe`
- Object file: `build/obj/...`

File `.o` dan executable hasil build tidak dibuat di root directory.

## Pembagian Tugas Milestone 2

| NIM | Tugas |
| :--- | :--- |
| 13524002 | Subprogram declaration, formal parameter, statement, variable chain, assignment, if |
| 13524022 | Case, while, repeat, for, procedure/function call, expression, operator precedence |
| 13524086 | Parser infrastructure, top-level grammar, parse tree, main.cpp, Makefile |
| 13524094 | Constant, type, var, array, range, enumerated, record |

## Testing

- Test Milestone 1 ada di `test/milestone-1`.
- Test Milestone 2 ada di `test/milestone-2`.

# 🚀 Arion Lexer - Milestone 1
**Tugas Besar IF2224 Teori Bahasa Formal dan Otomata 2026**

## 📝 Deskripsi Program
Program ini adalah *Lexical Analyzer* untuk bahasa pemrograman Arion yang dibangun menggunakan C++. Menggunakan pendekatan *Deterministic Finite Automata* (DFA), program ini mengubah *source code* menjadi rangkaian *tokens* dan mampu menangani *edge cases* seperti angka negatif, komentar *multi-line*, serta *error recovery* pada karakter ilegal.

## 👥 Identitas Kelompok
**Kode Kelompok:** khusus 1352___
**Nama Kelompok:** PRX

| Nama | NIM |
| :--- | :--- |
| Al Farabi | 13524086 |
| Ishaq Irfan Farizal | 13524094 |
| Daniel Anindito Nugroho | 13524002 |
| Ishak Palentino | 13524022 |

## 🛠 Requirements
- **GCC/G++ Compiler** (Standar C++17).
- **GNU Make**.
- Lingkungan **Linux/WSL** atau **MinGW/Git Bash**.

## 📂 Project Structure
```text
PRX-Tubes-IF2224-2026/
├── src/
│   ├── main.cpp         
│   ├── lexer/           # Lexical parsing architecture and DFA models
│   │   ├── lexer.hpp
│   │   └── lexer.cpp
│   ├── common/          # Universal token objects and mapping helper utilities
│   │   ├── token.hpp
│   │   └── utils.cpp
├── doc/                 # Compilation models and formal project references
└── Makefile             # Automatic build configurations
```

## Setup & Compilation 

To run this application, leverage the standard GNU Make sequence inside the project root:

1. **Clean Object Caches** (Optional, for fresh builds)
   ```bash
   make clean
   ```

2. **Build Sequence**
   ```bash
   make
   ```

3. **Execution**
   ```bash
   ./lexer_test.exe path/to/input/file.txt
   ```

| Nama | Pembagian Tugas |
| :--- | :--- |
| Al Farabi | Pembuatan laporan, perancangan DFA, penggambaran DFA, implementasi lexer.cpp dan lexer.hpp |
| Ishaq Irfan Farizal | Pembuatan laporan, perancangan DFA, penggambaran DFA, pengetesan program, implementasi token.cpp dan token.hpp |
| Daniel Anindito Nugroho | Pembuatan laporan, perancangan DFA, penggambaran DFA,  implementasi utils.cpp dan utils.hpp |
| Ishak Palentino | Pembuatan laporan, perancangan DFA, penggambaran DFA, implementasi main.cpp, MakeFile, dan readme  |

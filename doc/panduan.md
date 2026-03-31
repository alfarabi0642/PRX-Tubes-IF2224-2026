Ini adalah **Master Blueprint** untuk tim kamu. Panduan ini dirancang agar setiap anggota tahu persis apa yang harus dilakukan, file apa yang harus dibuat, dan bagaimana cara menggabungkannya tanpa bentrok.

---

### **FASE 1: THE FOUNDATION (Semua Anggota - Hari 1-2)**
**Objektif:** Menyamakan persepsi teknis agar kode yang dibuat Member B, C, dan D bisa masuk ke "mesin" yang dibuat Member A.

*   **Tugas Bersama:**
    1.  Menyepakati nomor state DFA (misal: State 0 = Start, State 10 = Ident, dll).
    2.  Menyepakati nama konstanta token di `token.h`.
*   **Konsep:** *Contract-Based Programming* (bekerja berdasarkan kesepakatan antarmuka).
*   **Step-by-Step:**
    1.  Buka tabel token di PDF hal 9-11.
    2.  Tulis semua nama token ke dalam `enum`.
    3.  Tentukan struktur `struct Token`.
*   **Output File:** `src/common/token.h`
    *   **Isi:** `enum TokenType { T_INTCON, T_IDENT, ... }` dan `struct Token { TokenType type; string lexeme; int line; }`.
    *   **Fungsi:** Kamus pusat identitas token untuk seluruh program.

---

### **FASE 2: INFRASTRUCTURE & REPO SETUP (Member A & C - Hari 2-3)**

#### **Member C: The Git Master**
*   **Objektif:** Menyiapkan wadah kolaborasi.
*   **Konsep:** *Version Control System (Git)*.
*   **Step-by-Step:**
    1.  Buat repo GitHub: `[KODE]-Tubes-IF2224-2026`.
    2.  Undang semua member.
    3.  Buat `.gitignore` (abaikan `.exe`, `.o`).
    4.  Buat folder `src/`, `test/`, `doc/`.
*   **Output:** Repository GitHub yang aktif.

#### **Member A: The Architect**
*   **Objektif:** Membuat "Mesin" utama yang menjalankan DFA.
*   **Konsep:** *File Stream (C-Style for speed)* dan *Main Loop*.
*   **Step-by-Step:**
    1.  Buat `main.cpp`: Logika membuka file `.txt`, memanggil fungsi Lexer, dan menulis output ke `output.txt`.
    2.  Buat `Makefile`: Aturan kompilasi agar tim tinggal mengetik `make`.
*   **Output File 1:** `src/main.cpp`
    *   **Isi:** Fungsi `main()` yang mengelola argumen file input/output.
*   **Output File 2:** `Makefile`
    *   **Isi:** Perintah `g++ src/*.cpp -o arion-lexer`.

---

### **FASE 3: CORE LEXER IMPLEMENTATION (Member B, C, D - Hari 4-10)**
Semua anggota mengerjakan satu file yang sama: `src/lexer/lexer.cpp`, namun di bagian (fungsi) yang berbeda.

#### **Member B: Identifiers, Keywords, & Error Handling**
*   **Objektif:** Menangani kata dan logika kesalahan.
*   **Konsep:** *Lookup Table* & *Case-Insensitivity*.
*   **Step-by-Step:**
    1.  Buat fungsi `isKeyword(string s)`: Mengubah string ke `toLowerCase()`, lalu cek apakah ada di daftar 19+ keyword.
    2.  Implementasikan State DFA untuk Huruf: Jika ketemu huruf, terus baca sampai ketemu non-huruf/angka.
    3.  Buat fungsi `handleError()`: Cetak baris mana yang salah jika ada simbol aneh.
*   **Output:** Blok kode di `lexer.cpp` yang menangani `T_IDENT` dan semua `T_...SY` (keywords).

#### **Member C: Literals (Numbers, Strings, Comments)**
*   **Objektif:** Menangani angka, teks di dalam petik, dan membuang komentar.
*   **Konsep:** *Greedy Consumption* (mengambil karakter sebanyak mungkin).
*   **Step-by-Step:**
    1.  **Numbers:** Jika ketemu digit, cek apakah ada titik (`.`). Jika ada, itu `REALCON`, jika tidak `INTCON`.
    2.  **Strings:** Jika ketemu `'`, baca terus sampai ketemu `'` lagi. Simpan isinya.
    3.  **Comments:** Jika ketemu `{` atau `(*`, buang semua karakter sampai ketemu penutupnya. Jangan jadikan token!
*   **Output:** Blok kode di `lexer.cpp` untuk `T_INTCON`, `T_REALCON`, `T_STRING`, `T_CHARCON`.

#### **Member D: Operators & Punctuation**
*   **Objektif:** Menangani simbol matematika dan tanda baca.
*   **Konsep:** *Lookahead (ungetc)*.
*   **Step-by-Step:**
    1.  **Single Char:** Jika ketemu `+`, `-`, `*`, langsung jadi token.
    2.  **Double Char:** Jika ketemu `:`, intip karakter berikutnya. Jika `=`, maka jadi `T_BECOMES` (`:=`). Jika bukan, kembalikan karakter intipan dengan `ungetc()` dan jadikan `T_COLON` (`:`).
    3.  Lakukan hal yang sama untuk `<>`, `<=`, `>=`, `==`.
*   **Output:** Blok kode di `lexer.cpp` untuk semua operator dan tanda baca (semicolon, comma, dll).

---

### **FASE 4: INTEGRATION & TESTING (Member A & D - Hari 11-14)**

#### **Member A: Integration**
*   **Objektif:** Memastikan semua blok kode B, C, D menyatu.
*   **Step-by-Step:**
    1.  Gabungkan semua fungsi ke dalam `class Lexer`.
    2.  Tes compile dengan `make`. Perbaiki jika ada variabel bentrok.
*   **Output:** Program `arion-lexer` yang bisa jalan.

#### **Member D: Test Runner**
*   **Objektif:** Memvalidasi kebenaran program sesuai spek Hal 12.
*   **Step-by-Step:**
    1.  Buat 5 file uji di `test/milestone-1/`.
    2.  Bandingkan output program dengan contoh di Hal 12 & 13 secara manual.
*   **Output:** 5 set input/output `.txt`.

---

### **FASE 5: VISUALIZATION & REPORTING (Member D & Semua - Hari 15-Deadline)**

#### **Member D: DFA Diagram & Laporan Final**
*   **Objektif:** Memenuhi syarat dokumentasi (Hal 13, 14, 18).
*   **Konsep:** *State Transition Diagram*.
*   **Step-by-Step:**
    1.  Buka Draw.io. Gambar setiap state yang sudah dikoding.
    2.  Pastikan panah transisi akurat (misal: dari State Start ke State Realcon lewat titik `.`).
    3.  Kumpulkan penjelasan teknis dari Member A, B, C (teori & implementasi).
    4.  Ekspor ke PDF: `Laporan-1-[KODE].pdf`.
*   **Output:** File Diagram DFA dan Laporan PDF Final.

---

### **RINGKASAN FLOW PROGRAM (Pipeline)**

1.  **Start:** `main.cpp` menerima argumen file `test/input.txt`.
2.  **Initialization:** `Lexer` disiapkan, pointer file diarahkan ke awal file.
3.  **Looping:**
    *   `Lexer::getNextToken()` dipanggil.
    *   **C-Style Read:** `fgetc()` ambil 1 karakter.
    *   **DFA Logic:** Masuk ke `switch(state)`.
    *   **Lookahead:** Jika perlu, `ungetc()` mengembalikan karakter.
    *   **Token Created:** Sebuah `struct Token` dikirim kembali ke `main`.
4.  **Logging:** `main.cpp` menulis tipe token dan lexeme-nya ke `output.txt`.
5.  **End:** File ditutup, `make clean` dilakukan.

---

### **Panduan Sukses untuk Setiap Member:**
*   **Member A:** Jangan pakai `std::cin`, pakai `fopen` dan `fgetc` karena kamu butuh `ungetc`.
*   **Member B:** Jangan lupa `toLowerCase()` sebelum cek Keyword, karena Arion *case-insensitive*.
*   **Member C:** Hati-hati dengan `realcon` seperti `12.`. Di beberapa bahasa ini ilegal, cek apakah Arion butuh angka setelah titik.
*   **Member D:** Gambar diagram DFA **setelah** kode stabil, agar apa yang digambar sama persis dengan apa yang jalan di kode (Poin penilaian Hal 17).

**Dengan mengikuti mapping ini, tim kamu bekerja secara paralel, rapi, dan modular sesuai permintaan asisten.**
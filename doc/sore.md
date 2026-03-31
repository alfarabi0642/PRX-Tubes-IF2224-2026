Berikut adalah penjelasan mendalam untuk setiap objektif **Groundwork** (persiapan sebelum *coding*) agar tim kamu memiliki fondasi yang solid sesuai spesifikasi tugas besar Arion.

---

### **1. Standarisasi Kamus Token (`token.h`)**
*   **Konsep:** **Enumerasi (Enum)**. Dalam pemrograman, *enum* digunakan untuk menciptakan tipe data yang memiliki nilai konstan terbatas. Ini memastikan tidak ada kesalahan pengetikan nama token antar anggota tim.
*   **Cara Menyelesaikan:** Buka tabel di halaman 9-11 dokumen. Masukkan semua nama di kolom "Token" (misal: `intcon`, `plus`, `beginsy`) ke dalam satu blok `enum`. Gunakan huruf kapital untuk konvensi konstanta (misal: `INTCON`, `PLUS`).
*   **Output Ekspektasi:** Sebuah file `token.h` yang berisi daftar lengkap 52+ identitas token yang akan dikenali oleh program.
    *   *Contoh:* `enum TokenType { T_INTCON, T_REALCON, T_PLUS, T_BEGINSY, ... };`

### **2. Penentuan Struktur Data Utama**
*   **Konsep:** **Data Encapsulation (Struct)**. Sebuah token bukan hanya tipenya, tapi juga nilai aslinya (*lexeme*). Kita perlu membungkus informasi ini agar mudah dikirim antar fungsi.
*   **Cara Menyelesaikan:** Buat sebuah `struct` atau `class` sederhana yang menampung `TokenType` (hasil objektif 1) dan sebuah `string` untuk menyimpan teks asli dari kode sumber (misal: "3.14"). Tambahkan atribut `line` (baris) untuk mempermudah laporan error.
*   **Output Ekspektasi:** Definisi `struct Token` di dalam `token.h`.
    *   *Contoh:* `struct Token { TokenType type; std::string lexeme; int line; };`

### **3. Sketsa Kasar Diagram DFA (The Big Map)**
*   **Konsep:** **Deterministic Finite Automata (DFA)**. Mesin matematika yang berpindah antar *State* berdasarkan input karakter. DFA harus memiliki satu *Start State* dan beberapa *Final State* (Accepting State).
*   **Cara Menyelesaikan:** Diskusi bersama: "Jika kita di State 0 dan membaca huruf, kita pindah ke State mana?". Gambar lingkaran-lingkaran di kertas/papan tulis. Beri nomor pada setiap state. Tentukan karakter apa yang memicu transisi (huruf, angka, simbol, atau spasi).
*   **Output Ekspektasi:** Foto atau coretan digital "Peta Jalan" DFA yang menunjukkan alur logika dari awal pembacaan sampai sebuah token ditemukan. Ini akan menjadi panduan Member D untuk membuat diagram Draw.io yang rapi.

### **4. Kesepakatan Logika "Lookahead" (`ungetc`)**
*   **Konsep:** **Input Buffering & Lookahead**. DFA seringkali baru tahu sebuah token selesai setelah membaca karakter "milik" token berikutnya.
    *   *Contoh:* Untuk tahu angka `123` sudah berakhir, kamu harus baca karakter setelahnya (misal spasi atau `;`). Karakter `;` tersebut tidak boleh hilang, harus dikembalikan ke sistem agar bisa dibaca lagi sebagai token tersendiri.
*   **Cara Menyelesaikan:** Sepakati penggunaan fungsi standar C `fgetc()` untuk baca satu karakter dan `ungetc()` untuk mengembalikan karakter ke aliran file (*file stream*).
*   **Output Ekspektasi:** Kesamaan pemahaman bahwa setiap fungsi transisi harus siap melakukan "langkah mundur" satu karakter sebelum mengirimkan hasil token.

### **5. Definisi Karakter (Helper Functions)**
*   **Konsep:** **Abstraksi Logika**. Daripada menulis `if (c >= 'a' && c <= 'z')` berulang kali di banyak file, buatlah fungsi pembantu yang deskriptif.
*   **Cara Menyelesaikan:** Buat file `utils.h`. Implementasikan fungsi `isAlpha`, `isDigit`, `isWhitespace`, dan fungsi krusial `toLower` (untuk menangani *case-insensitivity* sesuai halaman 11).
*   **Output Ekspektasi:** File `utils.h` yang bisa di-*include* oleh semua member untuk mempermudah penulisan kondisi `if-else` pada DFA.

### **6. Setup Repository & Git Flow**
*   **Konsep:** **Version Control & Collaboration**. Cara kerja tim di industri untuk memastikan kode tidak tertimpa dan riwayat perubahan tercatat (Wajib di Hal 15).
*   **Cara Menyelesaikan:** Member C membuat repo di GitHub. Semua member di-invite sebagai kolaborator. Tentukan aturan: "Jangan kerja di branch `main`, buat branch baru dengan nama `feat-logic-angka`". Gunakan `.gitignore` agar file hasil kompilasi (`.exe` atau `.o`) tidak masuk ke GitHub.
*   **Output Ekspektasi:** Link GitHub repository yang sudah memiliki folder `src/`, `test/`, dan `doc/` yang kosong namun siap diisi.

### **7. Keputusan Strategi "Keyword vs Identifier"**
*   **Konsep:** **Symbol Table Lookup**. Menentukan apakah kata seperti `if` ditangani lewat jalur DFA yang rumit (karakter demi karakter) atau lewat pengecekan string setelah kata terbentuk.
*   **Cara Menyelesaikan:** Diskusi: "Apakah kita mau buat State khusus untuk 'i' lalu 'f' (jalur DFA murni), atau kita baca semua kata sebagai `IDENT` dulu, lalu kita bandingkan string-nya dengan daftar keyword?".
    *   *Rekomendasi:* Baca sebagai `IDENT` dulu, lalu gunakan fungsi `isKeyword(string s)` yang mengembalikan `TokenType` yang sesuai (misal `IFSY`). Ini jauh lebih bersih dan mudah di-debug.
*   **Output Ekspektasi:** Kesepakatan tertulis di grup/repo bahwa pengerjaan keyword akan menggunakan metode *Lookup Table* (pencocokan string).

---

### **Ringkasan Flow Kerja Secara Runtut:**
1.  **Hadir dalam Meeting 1:** Selesaikan Objektif **1, 2, 3, 4, 7** (Diskusi & Kesepakatan).
2.  **Setelah Meeting:**
    *   Member A & C mengerjakan Objektif **5 & 6** (Setup teknis file & GitHub).
    *   Member A membagikan file `token.h`, `utils.h`, dan struktur `main.cpp` dasar ke semua orang.
3.  **Eksekusi:** Masing-masing member mulai *coding* di laptop sendiri (menggunakan branch masing-masing) dengan mengacu pada **Peta DFA** (Objektif 3) yang sudah dibuat bersama.

Dengan menyelesaikan objektif ini, tim kamu tidak akan bingung saat harus menggabungkan kode di minggu terakhir!
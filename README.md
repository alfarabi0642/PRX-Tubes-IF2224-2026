# 🚀 Arion Compiler - Milestone 1: Lexical Analyzer
**Tugas Besar IF2224 Teori Bahasa Formal dan Otomata 2026**

A robust, object-oriented Lexical Analyzer (Scanner) built in C++ to process the Arion programming language. This tokenizer implements a complete Deterministic Finite Automaton (DFA) string matching scheme mapped directly to an explicit architectural transition state machine—guaranteeing compliance, absolute coverage, and unyielding error recovery.

## 🌟 Capabilities & Edge Handling
The lexer explicitly resolves all advanced structural language constraints and Q&A edge cases natively in the core DFA loops:

- **Strict State Constraints:** Features an explicitly delineated DFA transitioning standard (`S0` to `S99`) entirely mapping custom symbols, reserved keywords, alphanumerics, and syntax structures.
- **Dynamic Unget Buffer:** Manages safe multiple rollback pushbacks for integers trailing period validations (e.g., `5.` safely mapped independently from `.18`).
- **Resilient Error Recovery (S1):** Non-fatal error traps! Identifiers failing syntax schemas skip bad characters securely back onto the sequence trace log and seamlessly reboot mapping checks towards `S0` without infinite loop locking or application crashes.
- **Negative Integer Edge Casts (Q42):** Robust negative integer sign validations pushing standard minus boundaries logically towards adjoining integer constructs exactly as guided by specification.
- **Strict String Mapping:** Gracefully supports string validations natively blocking single-line crossbounds (`\n`), accepting exact escape character nesting (`''''`), and accurately managing empty strings (`''`).
- **Comprehensive Match Resolution:** Implements the *Longest Match* rule for identifying variable lengths consistently avoiding overlap.

## 🛠 Tech Stack
- **Language:** C++ (Standard ISO)
- **Architecture:** Object-Oriented DFA State Machine
- **Dependencies:** Standard Template Library (`std::string`, `std::vector`, `std::stack`, `std::ctype`)
- **Build System:** `make` / GCC

## 📂 Project Structure
```text
PRX-Tubes-IF2224-2026/
├── src/
│   ├── main.cpp         # System orchestrator
│   ├── lexer/           # Lexical parsing architecture and DFA models
│   │   ├── lexer.hpp
│   │   └── lexer.cpp
│   ├── common/          # Universal token objects and mapping helper utilities
│   │   ├── token.hpp
│   │   └── utils.cpp
├── doc/                 # Compilation models and formal project references
└── Makefile             # Automatic build configurations
```

## 🚀 Setup & Compilation 

To run this application, leverage the standard GNU Make sequence inside the project root:

1. **Clean Object Caches** (Optional, for fresh builds)
   ```bash
   make clean
   ```

2. **Build Sequence**
   ```bash
   make
   ```
   *This links and compiles the executable map natively into standard outputs like `lexer_test.exe`.*

3. **Execution**
   ```bash
   ./lexer_test.exe
   ```
   *Alternatively, you can just execute:*
   ```bash
   make run
   ```
   *By default, the executor routes tests loaded from the targeted Arion source test configurations.*

## 👥 Authors
Designed & Developed by team **PRX-Tubes-IF2224-2026** for Milestone 1 compilation targets. 

---
*TBFO 2026 // Institut Teknologi Bandung*
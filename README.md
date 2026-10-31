# CISC Instruction Format Analyzer

A browser-based educational tool for Computer Organization and Architecture. It displays
verified 32-bit x86 CISC instruction encodings field by field, and compares x86's
variable-length instructions with fixed-length 32-bit RISC-V instructions.

The application has exactly two sections:

1. **CISC Instruction Format Display** — select a sample x86 instruction and inspect its
   machine-code bytes, field breakdown, and total length.
2. **Variable-Length vs Fixed-Length Instruction Formats** — a visual comparison of x86
   CISC and the RISC-V RV32I base integer ISA.

The backend is written in C++17 using the Crow web framework. The frontend is plain
HTML, CSS, and vanilla JavaScript. All instruction data comes from the C++ backend over
a REST API.

---

## Prerequisites

- A C++17 compiler with working thread support. On Windows, use **MinGW-w64 GCC** (GCC 9
  or newer recommended). The older MinGW.org GCC 6.3 does not provide `std::thread` and
  cannot build Crow.
- **CMake 3.15+** (recommended), or you can compile directly with `g++`.
- A modern web browser.

Crow (single-header v1.3.5) and standalone Asio 1.36.0 are included in `third_party/`, so
no package manager or network access is needed to build.

---

## Build and Run

### Option A: CMake (recommended)

If `build\cisc_analyzer.exe` is already present, you can skip the build and run it directly.

From the project root:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\build\cisc_analyzer.exe
```

If CMake uses a multi-configuration generator, the executable may be at
`build\Debug\cisc_analyzer.exe` instead.

### Option B: direct g++ build

From the project root:

```powershell
g++ -std=c++17 -O2 -static backend/main.cpp backend/analyzer.cpp -Ithird_party/crow -Ithird_party/asio/include -o cisc_analyzer.exe -lws2_32 -lmswsock
.\cisc_analyzer.exe
```

Run the executable from the project root so it can find the `frontend` folder. The CMake
build records the absolute frontend path, so its executable can be started from anywhere.
You can also set the `CISC_FRONTEND_DIR` environment variable to the frontend folder.
MinGW builds use `-static`, so the executable does not depend on MinGW runtime DLLs.

### Open the application

With the backend running, open:

```
http://localhost:8080
```

The server listens only on `127.0.0.1:8080`. Stop it with `Ctrl+C`.

---

## Project Structure

```
CISC Instruction Format Analyser/
├── backend/
│   ├── main.cpp          # Crow routes, JSON responses, static file serving
│   ├── analyzer.cpp      # Instruction data, validation, comparison data
│   └── analyzer.h
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── script.js         # Fetches data from the backend and renders it
├── third_party/
│   ├── crow/
│   │   ├── crow_all.h
│   │   └── LICENSE
│   └── asio/
│       ├── include/
│       └── LICENSE_1_0.txt
├── CMakeLists.txt
└── README.md
```

---

## Sample Instructions

All examples are valid 32-bit IA-32 encodings in Intel syntax. Byte sequences were checked
against NASM/NDISASM.

| Assembly | Machine code | Length | Present fields |
|---|---|---|---|
| `PUSH EAX` | `50` | 1 byte | Opcode |
| `MOV EAX, EBX` | `89 D8` | 2 bytes | Opcode, ModR/M |
| `MOV EAX, DWORD PTR FS:[EBX]` | `64 8B 03` | 3 bytes | Prefix, Opcode, ModR/M |
| `MOV EAX, DWORD PTR [EBX+4]` | `8B 43 04` | 3 bytes | Opcode, ModR/M, Displacement |
| `ADD EAX, 5` | `83 C0 05` | 3 bytes | Opcode, ModR/M, Immediate |
| `MOV EAX, DWORD PTR [EBX+ECX*4+8]` | `8B 44 8B 08` | 4 bytes | Opcode, ModR/M, SIB, Displacement |
| `ADD EAX, 0x12345678` | `05 78 56 34 12` | 5 bytes | Opcode, Immediate |
| `MOV EAX, DWORD PTR [EBX+ECX*4+0x12345678]` | `8B 84 8B 78 56 34 12` | 7 bytes | Opcode, ModR/M, SIB, Displacement |
| `LOCK ADD DWORD PTR [EBX], EAX` | `F0 01 03` | 3 bytes | Prefix, Opcode, ModR/M |

Instruction length is not hard-coded: the backend calculates it from the stored byte
sequence. At startup the backend also verifies that the field layout covers every byte
exactly once with no gaps or overlaps.

### Field definitions used by the tool

- **Prefix** — optional bytes that modify the instruction (segment override, LOCK, REP,
  operand-size, address-size).
- **Opcode** — identifies the operation; some short forms encode a register inside the
  opcode (for example `50`–`57` for `PUSH` of a 32-bit register).
- **ModR/M byte** — one byte split into `mod` (2 bits), `reg` (3 bits), and `r/m`
  (3 bits); selects register operands and the memory-addressing form.
- **SIB byte** — one byte split into `scale` (2 bits), `index` (3 bits), and `base`
  (3 bits); used when ModR/M `r/m = 100` to encode `[base + index × scale]`.
- **Displacement** — a 1-, 2-, or 4-byte signed offset added to the computed address.
- **Immediate data** — a constant operand encoded directly in the instruction.

Fields not used by a particular instruction are labelled **Absent**; their descriptions
state why rather than inventing byte values.

### RISC-V reference examples

The comparison uses the **RV32I base integer instruction set**, where every base
instruction is one 32-bit word (4 bytes):

| Assembly | Bytes in memory | 32-bit word | Format |
|---|---|---|---|
| `ADD x5, x6, x7` | `B3 02 73 00` | `0x007302B3` | R-type |
| `ADDI x5, x6, 10` | `93 02 A3 00` | `0x00A30293` | I-type |
| `LW x5, 8(x6)` | `83 22 83 00` | `0x00832283` | I-type |

The byte order shown is little-endian, matching how the 32-bit word is stored in memory.
The application also shows the R-type field layout of `ADD` (funct7, rs2, rs1, funct3,
rd, opcode) to illustrate fixed field positions inside a fixed-width word.

---

## REST API

The frontend only displays data returned by these endpoints:

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/instructions` | List of sample instructions with id, assembly, hex bytes, and length |
| GET | `/api/instructions/{id}` | Full encoding details, all six field categories, and length |
| GET | `/api/comparison` | Explanations, length samples, RISC-V field layout, and comparison rows |

Static frontend files are served from `/`, `/style.css`, and `/script.js`.

---

## Accuracy and Scope Notes

- The tool uses **32-bit x86 (IA-32) protected-mode** encodings and Intel syntax. It does
  not claim to support 16-bit or 64-bit encodings, or arbitrary x86 decoding.
- The examples are a predefined, verified collection. The backend does not implement a
  general-purpose disassembler.
- 32-bit x86 instructions can be 1 to 15 bytes long; the nine examples range from 1 to
  7 bytes.
- Fixed length here refers specifically to **RV32I base instructions**, which are always
  32 bits. It is not a universal property of RISC architectures: the optional RISC-V
  `C` compressed extension adds 16-bit instructions, and some other RISC ISAs use
  variable-length encodings. The application states this explicitly.
- The comparison shows the trade-off rather than a universal winner: x86 can encode
  common operations compactly but requires sequential, variable-length decoding; RV32I
  spends 4 bytes per instruction but has simple, regular fetch and decode.
- No quiz, login, database, simulator, file upload, AI, benchmarking, or extra
  architecture sections are included.

---

## Troubleshooting

**"Could not locate the frontend directory"**
Run the executable from the project root, place `frontend` next to the executable, or set
`CISC_FRONTEND_DIR` to the absolute path of the frontend folder.

**The page loads but shows "Instruction data unavailable"**
Make sure the C++ backend is still running and open the site through
`http://localhost:8080`, not by opening `index.html` directly from disk.

**Build error about `std::thread` or `<thread>`**
Use a MinGW-w64 toolchain with POSIX threads. MinGW.org GCC 6.3 is too old for Crow.

**Port 8080 is already in use**
Stop the process using port 8080, then start the backend again.

---

## Third-Party Components

- **Crow** v1.3.5 — BSD-3-Clause. See `third_party/crow/LICENSE`.
- **Asio** 1.36.0 — Boost Software License 1.0. See
  `third_party/asio/LICENSE_1_0.txt`.

These libraries are vendored only to build the C++ REST server locally.

# CISC Instruction Format Analyzer

An educational tool that displays sample CISC instruction formats and explains how
variable-length instructions differ from fixed-length instruction formats.

The C++17 backend uses the Crow web framework. The browser interface is built with HTML,
CSS and vanilla JavaScript and displays the data returned by the backend.

---

## What the Application Shows

The tool has exactly two sections.

### 1. CISC Instruction Format Explorer

Four simple 32-bit x86 instructions are included:

| Instruction | Machine code | Length |
| --- | --- | --- |
| `PUSH EAX` | `50` | 1 byte |
| `MOV EAX, EBX` | `89 D8` | 2 bytes |
| `MOV EAX, [EBX+4]` | `8B 43 04` | 3 bytes |
| `ADD EAX, 0x12345678` | `05 78 56 34 12` | 5 bytes |

Selecting an instruction shows:

- the instruction name,
- what it does,
- its machine-code bytes,
- what each byte means,
- the total instruction length.

Each byte block is clickable. Clicking a byte (or any byte in a multi-byte field) shows a
short explanation of that part of the instruction.

Only four instruction fields are explained:

- **Opcode** — tells the CPU what operation to perform.
- **ModR/M** — helps identify the registers or memory to use.
- **Displacement** — an extra number used to find a memory location.
- **Immediate** — a number given directly inside the instruction.

### 2. Variable-Length vs Fixed-Length Comparison

The comparison shows the four CISC instructions with lengths of 1, 2, 3 and 5 bytes next to
three valid RV32I examples that are all exactly 4 bytes long:

| RISC-V instruction | Machine code | Length |
| --- | --- | --- |
| `ADD x5, x6, x7` | `B3 02 73 00` | 4 bytes |
| `ADDI x5, x6, 10` | `93 02 A3 00` | 4 bytes |
| `LW x5, 8(x6)` | `83 22 83 00` | 4 bytes |

The page uses byte blocks to make the different CISC lengths easy to compare with the
equal-sized RISC-V instructions, and displays this explanation:

> CISC instructions can have different sizes. In the RV32I base instruction set, every
> standard instruction has the same size of 4 bytes.

---

## Technology

- C++17
- Crow (included in `third_party/crow`)
- Standalone Asio (included in `third_party/asio`)
- HTML, CSS and vanilla JavaScript
- CMake 3.15 or newer

Instruction data, byte layout validation and comparison data are kept in C++. The browser
only displays the JSON returned by the C++ backend.

The application does not include a quiz, login, AI, database, CPU simulator, extra
architectures or any instruction examples beyond the four listed above.

---

## Project Structure

```text
CISC Instruction Format Analyser/
|
|-- CMakeLists.txt
|-- README.md
|-- run.bat
|
|-- include/
|   |-- api.h
|   |-- comparison.h
|   `-- instruction.h
|
|-- src/
|   |-- api.cpp
|   |-- comparison.cpp
|   |-- instruction.cpp
|   `-- main.cpp
|
|-- web/
|   `-- index.html
|
`-- third_party/
    |-- asio/
    `-- crow/
```

---

## Build and Run

### One-click (Windows)

Double-click `run.bat`, or run:

```powershell
.\run.bat
```

Then open <http://localhost:8080> in your browser.

### Manual build

With MinGW:

```powershell
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\cisc_instruction_analyzer.exe
```

With Visual Studio, use the appropriate generator and build the
`cisc_instruction_analyzer` target.

The server listens on port 8080. Stop it with `Ctrl+C`.

---

## REST API

The existing API structure is unchanged:

| Endpoint | Purpose |
| --- | --- |
| `GET /` | Serves `web/index.html` |
| `GET /api/instructions` | Returns the four-instruction catalog |
| `GET /api/instruction/:id` | Returns one instruction with its byte fields |
| `GET /api/comparison` | Returns CISC and RV32I comparison samples |

Instruction IDs are `push-eax`, `mov-eax-ebx`, `mov-eax-ebx-disp8` and `add-eax-imm32`.

---

## Notes on Correctness

- Machine-code bytes and lengths are stored together and checked at startup.
- The startup check confirms that every byte belongs to exactly one field, with no gaps or
  overlaps.
- Lengths are calculated from the stored byte sequences rather than being hard-coded in the
  frontend.
- The RV32I examples use standard 32-bit encodings, and each occupies exactly 4 bytes.

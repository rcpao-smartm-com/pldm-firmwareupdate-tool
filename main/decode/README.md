# PLDM Firmware Update Parser

## 1. Overview

This project provides a command-line tool and accompanying static library to parse PLDM Firmware Update headers as defined in DSP0267 **1.0–1.3**. It processes the package header, device ID records, optional Downstream Device area (1.1+), component image information (including Component Opaque Data on 1.2+), Reference Manifest fields (1.3), and verifies CRC32 checksums (header always; package payload CRC on 1.3).

## 2. Key Components

- **`pldm_fw_update_parse.h`**  
  Declares data structures (e.g., `PLDMFirmwarePackageHeader`, `PLDMFirmwareDeviceIdRecord`, `PLDMComponentImageInfo`) and parsing helper function prototypes for endian-aware reads, DSP0267 version detection, printing string types, descriptor IDs, and component classifications.

- **`pldm_fw_update_parse.c`**  
  Implements the functions declared in `pldm_fw_update_parse.h`. Contains lookup tables for descriptor IDs and component classifications, and utilities such as byte-order extraction and string type printing.

- **`main.c`**  
  Includes the `parseFile()` function, which:
  1. Reads the PLDM firmware header size from the binary file.
  2. Detects DSP0267 1.0–1.3 from Package Header Identifier UUID / format revision.
  3. Parses and displays package header, device records, version-specific areas, and checksums.
  4. Recomputes CRC32 and reports OK/FAIL (non-zero exit on mismatch).

- **Makefile**  
  - Builds `libparse.a` and `parse_pldm` (Linux) or `parse_pldm.exe` (Windows).
  - Uses `-Wall`, `-Wextra`, and `-O2`.
  - Defines a `clean` target to remove build artifacts.

## 3. Dependencies

- **C Compiler** supporting C11 (e.g., GCC)
- **zlib** (`-lz`)
- **json-c** (`-ljson-c`)
- Make tool (for building via the provided Makefile)

## 4. Build Instructions

1. Ensure that `zlib` and `json-c` are installed on your system.  
2. Open a terminal in `main/decode`.  
3. Run `make` to build the static library (`libparse.a`) and the executable.

This will produce:
- `pldm_fw_update_parse.o`  
- `main.o`  
- `libparse.a`  
- `parse_pldm` (or `parse_pldm.exe` on Windows)

From the repo root, `make decode` builds the tool and parses the package produced by `make encode` (`main/pldm1.0-img_0.bin`).

## 5. Usage

```bash
./parse_pldm <firmware_file.bin>
```

1. Provide the path to your PLDM firmware package (header-only or full package).  
2. The program detects the DSP0267 format (1.0–1.3), prints header fields, and verifies checksums.  
3. If no argument is supplied, it prompts for a file path.

Supported layout extras by version:
| Version | Extra fields handled |
|---------|----------------------|
| 1.0 | Base package header |
| 1.1 | Downstream Device ID Record Count (+ skip record bodies by length) |
| 1.2 | ComponentOpaqueDataLength / Data |
| 1.3 | ReferenceManifestLength / Data, Package Payload Checksum |

## 6. Example

```bash
./parse_pldm ../pldm1.0-img_0.bin
```

```
--------------------------------------------
PLDM Firmware Update: Full Parsing
--------------------------------------------
PackageHeaderIdentifier (UUID): ...
PackageHeaderFormatRevision: 0x01
PackageHeaderSize: ...
DSP0267 Package Format: 1.0
...
Package Header Checksum (CRC32): 0x...
Package Header Checksum: OK (matches computed 0x...)
```

## 7. Cleaning Up
To remove generated object files, the static library, and the executable, run:

```bash
make clean
```



# PLDM Firmware package Header Generator

This project offers the capability to parse and generate the PLDM firmware update header. Primarily, it decodes the PLDM.json file to produce a binary firmware update header file named pldm_update_header.bin. This binary file is an encapsulated firmware update header. Subsequently, the system calculates the CRC32 for the entire PLDM firmware update header and appends this CRC32 to the end of the header section within the package. Finally, the content from the image_payload.bin file is appended to the created pldm_update_header.bin, forming a complete PLDM firmware update package

# Overview
Starting with a source file named PLDM.json, this generator will:
1. Parse the JSON content to understand the structure and data it contains.
2. Generate an intermediary binary file, PLDM_FW_PKG_Header.bin, representing the firmware package header from the parsed data.
3. If an image_payload.bin file is present, its contents will be appended to pldm_update_header.bin
4 Finally, the utility generates Pldm Firmware Update Package file "pldm_update_pkg.bin ".

# Key Features
### Structured Input: 
The program anticipates a standardized JSON format in PLDM.json, enabling users to easily modify or expand their firmware data.

### Binary Generation: 
It skillfully crafts a binary representation of the package header, optimized for firmware applications.

### Checksum Calculation: 
To ensure the integrity of the data, a checksum is calculated for the entire package header and subsequently appended to the pldm_update_header.bin file.

### Payload Integration: 
Upon detecting an image_payload.bin, the utility effortlessly incorporates its content to the resulting binary header.

## How to Use
1. Place your PLDM.json and optionally, image_payload.bin, in the project's directory.
2. Run the utility.
3. Once completed, retrieve the PLDM_FW_PKG_Header.bin with the appended checksum.
This streamlined process ensures quick turnaround times, allowing firmware developers to focus on their core functionalities while trusting the integrity and accuracy of their package headers.

## Prerequisites
Ensure the following are set up on your system before proceeding:

(1) C compiler: For instance, GCC.

(2) json-c: A JSON library for C.

(3) Files:

        (i) PLDM.json: This should be present in the same directory as the program, serving as the input.
        (ii) image_payload.bin: This file's content will be appended to the generated firmware package header.
## Important Notes
Ensure PLDM.json and image_payload.bin are located in the same directory.
Install all necessary dependencies before running the program.

## Input
The program expects `PLDM.json` in the `main/` directory, plus a firmware image path and version passed on the command line (the image is packaged as the component payload).

## Output
Upon successful execution, the utility generates:
- `pldm_update_header.bin`: package header including checksum field(s)
- `pldm<spec>-<image_basename>`: full PLDM firmware update package (header + payload), e.g. `pldm1.0-img_0.bin`


## Usage
### Installation
For the json-c library:
    
    sudo apt install libjson-c-dev # Ubuntu
    sudo dnf install -y json-c-devel # Fedora

### Encoding 
Build, then encode a package for a DSP0267 header revision:

    make all
    cd main
    ./pldm_encode <image_file> <firmware_version> <spec_version>
    # On Windows the binary is pldm_encode.exe
    # spec_version: 1.0 | 1.1 | 1.2 | 1.3

Example (also what `make encode` runs):

    make encode
    # -> main/pldm_encode img_0.bin 1.0.0 1.0
    # -> writes main/pldm1.0-img_0.bin

| `<spec_version>` | Package Header Identifier (UUID) | Format revision |
|------------------|----------------------------------|-----------------|
| 1.0 | `F018878C-…-CA02` | 0x01 |
| 1.1 | `1244D264-…-7D5A` | 0x02 (+ empty Downstream Device area) |
| 1.2 | `3119CE2F-…-F6BF` | 0x03 (+ Component Opaque Data length 0) |
| 1.3 | `7B291C99-…-3C78` | 0x04 (+ Reference Manifest length 0, payload CRC) |

![image](https://github.com/quanta-Irenelin/PLDM_FW_UPDATE/assets/85274528/3168a588-b750-4157-8a1a-c09a56324a77)
![image](https://github.com/quanta-Irenelin/PLDM_FW_UPDATE/assets/85274528/bf9a6505-9c90-46b1-a966-17715edf0ff9)


### Decoding
Decode auto-detects DSP0267 **1.0–1.3** from the package UUID / format revision, walks the matching header layout, and verifies CRC32 checksums (header always; payload CRC for 1.3).

Encode+decode smoke test for DSP0267 **1.0–1.3** (builds tools, writes `main/pldm{1.0,1.1,1.2,1.3}-img_0.bin`, verifies CRC on each):

    make decode

Or decode a package manually:

    cd main/decode
    ./parse_pldm ../pldm1.0-img_0.bin
    ./parse_pldm ../pldm1.1-img_0.bin
    ./parse_pldm ../pldm1.2-img_0.bin
    ./parse_pldm ../pldm1.3-img_0.bin
    # On Windows: parse_pldm.exe

Limitations:

- This tool’s encoder writes empty/zero-length optional fields: Downstream Device ID Record Count `0` (1.1+), Component Opaque Data length `0` (1.2+), and Reference Manifest length `0` (1.3). So `make decode` exercises the version layouts and CRCs, but not non-empty opaque/manifest/downstream payloads.
- Non-empty Downstream Device ID records (e.g. from other tools) are skipped by `RecordLength` so the rest of the header stays aligned; detailed pretty-print of those record bodies is not implemented yet.

![image](https://github.com/quanta-Irenelin/PLDM_FW_UPDATE/assets/85274528/01490ea7-1c65-4ba3-87b0-20c350b4496c)


 ### Pldm Firmware Update Package file name: pldm_update_pkg.bin  
![image](https://github.com/quanta-Irenelin/PLDM_FW_UPDATE/assets/85274528/ae13fd83-fa27-45e1-8870-36d22cf1d68c)

### clear:
To clean the generated file:

        make clean
![image](https://github.com/quanta-Irenelin/PLDM_FW_UPDATE/assets/85274528/093990a6-d5fd-4d96-b5e1-354e8d0c6f6c)


### Details of the libpldm.a Library
You can view the object files within the libpldm.a static library using:
    
    $ ar -t libpldm.a

#### Contained objects:
1. compo.o
2. pkg_info.o
3. FD_ID.o
4. data_trans_fxn.o
5. parse_PLDM_json.o
6. pldm_version.o


## Additional Resources
For more information on the PLDM firmware update specification, refer to DSP0267 (DMTF), including versions 1.0.x through 1.3.0:

https://www.dmtf.org/standards/pmci


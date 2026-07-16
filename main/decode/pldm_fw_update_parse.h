#ifndef PLDM_FW_UPDATE_PARSE_H
#define PLDM_FW_UPDATE_PARSE_H

#include <stddef.h>
#include <stdint.h>

 /**
 * @file pldm_fw_update_parse.h
 * @brief Data structures and definitions for parsing/constructing PLDM Firmware Update headers
 *        and associated records, according to DSP0267 specification.
 *
 *        All multi-byte fields use little-endian byte ordering per Clause 5.2 of the spec.
 *        On little-endian hosts (e.g. x86, ARM), storing them directly in uint16_t/uint32_t
 *        matches the on-wire format. On big-endian hosts, code must byte-swap when reading/writing.
 */

/**
 * @brief The PLDM specification typically uses a CRC-32 as the PackageHeaderChecksum,
 *        computed across all bytes of the package header except for the 4-byte checksum field itself.
 */

/** 
 * @brief Extract a 16-bit integer from two little-endian bytes.
 * @param low  Least significant byte
 * @param high Most significant byte
 * @return The combined uint16_t value
 */
uint16_t pldmFwExtractUint16LE(uint8_t low, uint8_t high);

/**
 * @brief Extract a 32-bit integer from four little-endian bytes.
 * @param b0 LSB
 * @param b1
 * @param b2
 * @param b3 MSB
 * @return The combined uint32_t value
 */
uint32_t pldmFwExtractUint32LE(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

/**
 * @brief Print bytes in reversed order (hexadecimal).
 *        Used for debugging endianness or memory layouts.
 * @param data Pointer to the byte array
 * @param length Number of bytes
 */
void pldmFwPrintReversedHex(const uint8_t* data, size_t length);

/**
 * @brief Print a string type (Table 20) given a 1-byte code [0..5].
 * @param type  The 1-byte string type
 */
void pldmFwPrintStringType(uint8_t type);

/**
 * @brief Print a descriptor ID (Table 7) from two little-endian bytes.
 * @param low  LSB
 * @param high MSB
 */
void pldmFwPrintDescriptorID(uint8_t low, uint8_t high);

/**
 * @brief Print a component classification (Table 19) from two little-endian bytes.
 * @param low  LSB
 * @param high MSB
 */
void pldmFwPrintComponentClassification(uint8_t low, uint8_t high);

/**
 * @brief Returns the exponent if 'num' is a power-of-two, else counts how many times
 *        the number can be divided by 2 until it reaches 1.
 * @param num The integer to evaluate
 * @return The exponent, or -1 if num == 0
 */
int pldmFwGetPowerOfTwoExponent(int num);

/**
 * @brief Map PackageHeaderIdentifier UUID + format revision to DSP0267 major.minor
 *        encoded as 10=1.0, 11=1.1, 12=1.2, 13=1.3. Prefers UUID match; falls back
 *        to format revision (0x01..0x04). Returns -1 if unrecognized.
 */
int pldmFwDetectDsp0267Version(const uint8_t uuid[16], uint8_t formatRevision);

/** @brief Write "1.0".."1.3" into out (needs at least 4 bytes). */
void pldmFwDsp0267VersionString(int ver, char *out, size_t out_len);

int pldmFwHasDownstreamArea(int ver);       /* >= 1.1 */
int pldmFwHasComponentOpaque(int ver);      /* >= 1.2 */
int pldmFwHasReferenceManifest(int ver);    /* >= 1.3 */
int pldmFwTrailingChecksumBytes(int ver);   /* 8 for 1.3, else 4 */

/* --------------------------------------------------------------------------
 * 1. PLDM Firmware Package Header (Table 3)
 *
 *    Byte ordering for entire header is Little Endian per Clause 5.2
 *
 *    Fields in order:
 *    1)  UUID   (16 bytes): PackageHeaderIdentifier
 *    2)  uint8  : PackageHeaderFormatRevision
 *    3)  uint16 : PackageHeaderSize
 *    4)  timestamp104 (13 bytes): PackageReleaseDateTime
 *    5)  uint16 : ComponentBitmapBitLength
 *    6)  enum8  : PackageVersionStringType
 *    7)  uint8  : PackageVersionStringLength
 *    8)  Variable: PackageVersionString (up to 255 bytes)
 * -------------------------------------------------------------------------- */

/**
 * @brief PLDMFirmwarePackageHeader - Represents the package header area, excluding
 *        the variable PackageVersionString and final checksum. 
 * 
 *        If the specification calls for a 13-byte "timestamp104" for PackageReleaseDateTime,
 *        we store it as a fixed array. 
 * 
 *        The actual "PackageVersionString" (up to 255 bytes) will follow this structure in memory.
 *        Similarly, after the entire package header, a 4-byte CRC32 (PackageHeaderChecksum) is appended.
 */
typedef struct __attribute__((packed))
{
    uint8_t  packageHeaderIdentifier[16];    ///< 16 bytes (UUID)
    uint8_t  packageHeaderFormatRevision;    ///< 1 byte: 0x01=1.0 .. 0x04=1.3
    uint16_t packageHeaderSize;              ///< 2 bytes, LE, total size of the header
    uint8_t  packageReleaseDateTime[13];     ///< 13-byte timestamp(104)
    uint16_t componentBitmapBitLength;       ///< 2 bytes, LE, multiple of 8
    uint8_t  packageVersionStringType;       ///< 1 byte, see Table 20
    uint8_t  packageVersionStringLength;     ///< 1 byte, up to 255
    // Followed by [packageVersionStringLength] bytes => "PackageVersionString"
} PLDMFirmwarePackageHeader;

/* --------------------------------------------------------------------------
 * 2. Firmware Device Identification Area
 *
 *    - uint8 : DeviceIDRecordCount
 *    - (For each record) FirmwareDeviceIDRecord(s), see Table 4.
 * -------------------------------------------------------------------------- */

/**
 * @brief The count of device ID records (uint8) that follow in the package.
 *        Typically stored directly in the package after the package header info.
 */

/**
 * @brief PLDMFirmwareDeviceIdRecord - Represents a single "Firmware Device ID Record" (Table 4).
 * 
 *    1) uint16 : RecordLength (the total length in bytes of this record, includes descriptors, etc.)
 *    2) uint8  : DescriptorCount
 *    3) bitfield32_t (4 bytes, LE): DeviceUpdateOptionFlags
 *    4) enum8  : ComponentImageSetVersionStringType
 *    5) uint8  : ComponentImageSetVersionStringLength
 *    6) uint16 : FirmwareDevicePackageDataLength
 *    7) Variable fields (e.g., applicableComponents bitfield, version string, descriptor(s), etc.)
 */
typedef struct __attribute__((packed))
{
    uint16_t recordLength;        ///< 2 bytes, LE
    uint8_t  descriptorCount;     ///< 1 byte
    uint32_t deviceUpdateOptionFlags; ///< 4 bytes, LE (each bit = update option)
    uint8_t  componentImageSetVersionStringType;  ///< 1 byte (Table 20)
    uint8_t  componentImageSetVersionStringLength;///< 1 byte
    uint16_t firmwareDevicePackageDataLength;     ///< 2 bytes, LE
    // Then variable fields: Descriptors, bitmaps, strings, etc.
} PLDMFirmwareDeviceIdRecord;

/* --------------------------------------------------------------------------
 * 3. Component Image Information Area (Table 5)
 *
 *    - uint16 : ComponentImageCount
 *    - For each component image => repeated structure 
 *      (componentClassification, componentIdentifier, etc.)
 * -------------------------------------------------------------------------- */

/**
 * @brief PLDMComponentImageInfo - structure for each component image record in Table 5
 * 
 *    1) uint16 : ComponentClassification
 *    2) uint16 : ComponentIdentifier
 *    3) uint32 : ComponentComparisonStamp
 *    4) bitfield16 => ComponentOptions
 *    5) bitfield16 => RequestedComponentActivationMethod
 *    6) uint32 : ComponentLocationOffset
 *    7) uint32 : ComponentSize
 *    8) enum8  : ComponentVersionStringType
 *    9) uint8  : ComponentVersionStringLength
 *    10) Variable: ComponentVersionString
 */
typedef struct __attribute__((packed))
{
    uint16_t componentClassification;      ///< 2 bytes, LE
    uint16_t componentIdentifier;          ///< 2 bytes, LE
    uint32_t componentComparisonStamp;     ///< 4 bytes, LE
    uint16_t componentOptions;             ///< 2 bytes, LE
    uint16_t requestedCompActivationMethod;///< 2 bytes, LE
    uint32_t componentLocationOffset;      ///< 4 bytes, LE
    uint32_t componentSize;                ///< 4 bytes, LE
    uint8_t  componentVersionStringType;   ///< 1 byte (Table 20)
    uint8_t  componentVersionStringLength; ///< 1 byte, up to 255
    // Followed by [componentVersionStringLength] bytes => "ComponentVersionString"
} PLDMComponentImageInfo;

/* --------------------------------------------------------------------------
 * 4. Descriptor definition area (Table 6)
 *
 *    For the "RecordDescriptors" in a Firmware Device ID Record, each descriptor:
 *     - uint16 : InitialDescriptorType
 *     - uint16 : InitialDescriptorLength
 *     - Variable data: InitialDescriptorData
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((packed))
{
    uint16_t descriptorType;   ///< 2 bytes, LE
    uint16_t descriptorLength; ///< 2 bytes, LE
    // Followed by [descriptorLength] bytes => descriptor data
} PLDMDescriptorEntry;

/* --------------------------------------------------------------------------
 * 5. Additional fields from the specification
 *    - PackageHeaderChecksum (4 bytes, LE), typically a CRC-32
 * -------------------------------------------------------------------------- */

/**
 * @brief PLDMPackageHeaderChecksum - 4-byte CRC32 field appended after the entire package header.
 */
typedef struct __attribute__((packed))
{
    uint32_t checksum; ///< 4 bytes, LE (lowest byte stored first)
} PLDMPackageHeaderChecksum;

#endif // PLDM_FW_UPDATE_PARSE_H

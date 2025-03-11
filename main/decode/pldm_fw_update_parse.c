#include "pldm_fw_update_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/**
 * Descriptor Identifier Table (Table 7).
 * Indices in [0x0000..0x0105], plus 0xFFFF for vendor-defined if needed.
 */
#define MAX_DESCRIPTOR_ID     0x0105
#define DESCRIPTOR_TABLE_SIZE (MAX_DESCRIPTOR_ID + 1)

/*
 * Macro that places a descriptor name at a specific index.
 * For example, DESCRIPTOR_ID_MAP("PCI Vendor ID", 0x0000) => table[0x0000] = "PCI Vendor ID"
 */
#define DESCRIPTOR_ID_MAP(name, val) [val] = name

/**
 * descriptorIdentifierTable: an array storing strings for known descriptor IDs.
 * Indices beyond 0x0105 can be NULL or vendor-defined.
 */
static const char* const descriptorIdentifierTable[DESCRIPTOR_TABLE_SIZE] = {
    DESCRIPTOR_ID_MAP("PCI Vendor ID",               0x0000),
    DESCRIPTOR_ID_MAP("IANA Enterprise ID",          0x0001),
    DESCRIPTOR_ID_MAP("UUID",                        0x0002),
    DESCRIPTOR_ID_MAP("PnP Vendor ID",               0x0003),
    DESCRIPTOR_ID_MAP("ACPI Vendor ID",              0x0004),
    /* Additional known values for 0x0100..0x0105: */
    DESCRIPTOR_ID_MAP("PCI Device ID",               0x0100),
    DESCRIPTOR_ID_MAP("PCI Subsystem Vendor ID",     0x0101),
    DESCRIPTOR_ID_MAP("PCI Subsystem ID",            0x0102),
    DESCRIPTOR_ID_MAP("PCI Revision ID",             0x0103),
    DESCRIPTOR_ID_MAP("PnP Product Identifier",      0x0104),
    DESCRIPTOR_ID_MAP("ACPI Product Identifier",     0x0105)
};

/*
 * Component Classification Table (Table 19):
 *   0x0000..0x000D => standard
 *   0x8000..0xFFFF => vendor-defined
 */
static const char* const componentClassificationTable[] = {
    "Unknown",                 /* 0x0000 */
    "Other",                   /* 0x0001 */
    "Driver",                  /* 0x0002 */
    "Configuration Software",  /* 0x0003 */
    "Application Software",    /* 0x0004 */
    "Instrumentation",         /* 0x0005 */
    "Firmware/BIOS",           /* 0x0006 */
    "Diagnostic Software",     /* 0x0007 */
    "Operating System",        /* 0x0008 */
    "Middleware",              /* 0x0009 */
    "Firmware",                /* 0x000A */
    "BIOS/FCode",              /* 0x000B */
    "Support/Service Pack",    /* 0x000C */
    "Software Bundle"          /* 0x000D */
    /* 0x000E..0x7FFF => out-of-range, 0x8000..0xFFFF => vendor-defined */
};

/*
 * String Type Table (Table 20):
 *   0 => Unknown
 *   1 => ASCII
 *   2 => UTF-8
 *   3 => UTF-16
 *   4 => UTF-16LE
 *   5 => UTF-16BE
 */
static const char* const stringTypeTable[] = {
    "Unknown",   /* 0 */
    "ASCII",     /* 1 */
    "UTF-8",     /* 2 */
    "UTF-16",    /* 3 */
    "UTF-16LE",  /* 4 */
    "UTF-16BE"   /* 5 */
};

/* -------------------------------------------------------------------------
 * Implementation of the function prototypes declared in pldm_fw_update_parse.h
 * All are pure C (no C++ features).
 * ------------------------------------------------------------------------- */

/**
 * @brief Extract a 16-bit integer from two little-endian bytes.
 */
uint16_t pldmFwExtractUint16LE(uint8_t low, uint8_t high)
{
    return (uint16_t)(low) | ((uint16_t)(high) << 8);
}

/**
 * @brief Extract a 32-bit integer from four little-endian bytes.
 */
uint32_t pldmFwExtractUint32LE(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    return ((uint32_t)b0)
         | ((uint32_t)b1 << 8)
         | ((uint32_t)b2 << 16)
         | ((uint32_t)b3 << 24);
}

/**
 * @brief Print bytes in reversed hex order. Helpful for debugging endianness.
 */
void pldmFwPrintReversedHex(const uint8_t* data, size_t length)
{
    int i;
    if (!data || length == 0)
    {
        return;
    }
    for (i = (int)length - 1; i >= 0; i--)
    {
        printf("%02x", data[i]);
    }
    printf("\n");
}

/**
 * @brief Print a string type (Table 20) given a 1-byte code [0..5].
 */
void pldmFwPrintStringType(uint8_t type)
{
    size_t tblCount = sizeof(stringTypeTable) / sizeof(stringTypeTable[0]);
    if (type < tblCount)
    {
        printf("%s\n", stringTypeTable[type]);
    }
    else
    {
        printf("String Type 0x%02X is vendor-defined or out-of-range.\n", type);
    }
}

/**
 * @brief Print a descriptor ID (Table 7) from two little-endian bytes.
 */
void pldmFwPrintDescriptorID(uint8_t low, uint8_t high)
{
    uint16_t idVal = pldmFwExtractUint16LE(low, high);

    if (idVal <= MAX_DESCRIPTOR_ID && descriptorIdentifierTable[idVal] != NULL)
    {
        printf("%s\n", descriptorIdentifierTable[idVal]);
    }
    else if (idVal == 0xFFFF)
    {
        printf("Vendor-Defined Descriptor ID (0xFFFF)\n");
    }
    else
    {
        printf("Descriptor ID 0x%04X is not recognized in Table 7.\n", idVal);
    }
}

/**
 * @brief Print a component classification (Table 19) from two little-endian bytes.
 */
void pldmFwPrintComponentClassification(uint8_t low, uint8_t high)
{
    uint16_t classVal = pldmFwExtractUint16LE(low, high);
    size_t knownCount = sizeof(componentClassificationTable) /
                        sizeof(componentClassificationTable[0]);

    if (classVal < knownCount)
    {
        /* 0x0000..0x000D => standard range */
        printf("%s\n", componentClassificationTable[classVal]);
    }
    else if (classVal >= 0x8000)
    {
        printf("Vendor-Defined Classification (0x%04X)\n", classVal);
    }
    else
    {
        printf("Classification 0x%04X is not standard or vendor-defined.\n", classVal);
    }
}

/**
 * @brief Returns exponent if 'num' is a power-of-two, else how many divisions by 2 until it hits 1.
 */
int pldmFwGetPowerOfTwoExponent(int num)
{
    int exponent = 0;
    if (num == 0)
    {
        return -1;
    }
    while (num != 1)
    {
        num /= 2;
        exponent++;
    }
    return exponent;
}

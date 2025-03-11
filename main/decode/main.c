#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#include "pldm_fw_update_parse.h"

/**
 * @brief parseAndPrintTimestamp104
 *
 * Parses a 13-byte timestamp field with this layout:
 *   [0..1]: UTC_offset (2 bytes, LE)
 *   [2..4]: msec (3 bytes, 24-bit)
 *   [5]   : second
 *   [6]   : minute
 *   [7]   : hour
 *   [8]   : day
 *   [9]   : month
 *   [10..11]: year (LE)
 *   [12]  : UTC offset / special flag
 *
 * Prints the extracted fields in a human-readable format.
 */
static void parseAndPrintTimestamp104(const uint8_t timeField[13])
{
    /* 1) UTC_offset => 2 bytes, little-endian */
    uint16_t utcOffset = pldmFwExtractUint16LE(timeField[0], timeField[1]);

    /* 2) 3-byte msec => 24-bit */
    uint32_t msec = ((uint32_t)timeField[2] << 16) |
                    ((uint32_t)timeField[3] << 8) |
                     (uint32_t)timeField[4];

    /* 3) Single bytes for second, minute, hour, day, month */
    uint8_t sec = timeField[5];
    uint8_t min = timeField[6];
    uint8_t hr  = timeField[7];
    uint8_t day = timeField[8];
    uint8_t mon = timeField[9];

    /* 4) year => 2 bytes, LE => [10..11] */
    uint16_t yearVal = pldmFwExtractUint16LE(timeField[10], timeField[11]);

    /* 5) The last byte is the UTC offset or special flag. */
    uint8_t utcFlag = timeField[12];

    /* Print the results */
    printf("=== PLDM Timestamp(104) ===\n");
    printf("UTC_offset (LE): 0x%04x\n", utcOffset);
    printf("Milliseconds (msec): %u\n", msec);
    printf("Date: %04d/%02d/%02d\n", yearVal, mon, day);
    printf("Time: %02d:%02d:%02d\n", hr, min, sec);
    printf("UTC/Offset Flag: 0x%02x\n", utcFlag);
}

/*
 * Example of parsing and printing the entire PLDM update header:
 *   1) Read pkg_header_size (2 bytes) at offset=17
 *   2) Parse the Package Header (PLDMFirmwarePackageHeader)
 *   3) Parse the bitfield of applicableComponents for device records
 *   4) Parse descriptors, version strings, etc.
 *   5) Parse component images
 *   6) Parse final 4-byte checksum
 */
static int parseFile(const char* filePath)
{
    FILE* fp = fopen(filePath, "rb");
    if (!fp)
    {
        fprintf(stderr, "Error: cannot open pldm_update_header.bin\n");
        return 1;
    }

    /* Step 1: read pkg_header_size from offset=17 (2 bytes) */
    fseek(fp, 17, SEEK_SET);
    uint16_t pkgHeaderSize = 0;
    fread(&pkgHeaderSize, sizeof(pkgHeaderSize), 1, fp);

    /* Return to file start */
    fseek(fp, 0L, SEEK_SET);

    /* Read entire package header into a buffer */
    uint8_t* buffer = (uint8_t*)malloc(pkgHeaderSize);
    if (!buffer)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(fp);
        return 1;
    }
    fread(buffer, pkgHeaderSize, 1, fp);
    fclose(fp);

    /* 1) Interpret the buffer start as a PLDMFirmwarePackageHeader */
    PLDMFirmwarePackageHeader* hdr = (PLDMFirmwarePackageHeader*)buffer;
    int index = sizeof(PLDMFirmwarePackageHeader);

    printf("--------------------------------------------\n");
    printf("PLDM Firmware Update: Full Parsing\n");
    printf("--------------------------------------------\n");

    /* Print the 16-byte PackageHeaderIdentifier (UUID) */
    int i;
    printf("PackageHeaderIdentifier (UUID): ");
    for (i = 0; i < 16; i++)
    {
        printf("%02x", hdr->packageHeaderIdentifier[i]);
    }
    printf("\n");
    printf("PackageHeaderFormatRevision: 0x%02x\n", hdr->packageHeaderFormatRevision);
    printf("PackageHeaderSize: %d bytes\n", hdr->packageHeaderSize);

    printf("PackageReleaseDateTime:\n");
    parseAndPrintTimestamp104(hdr->packageReleaseDateTime);

    printf("ComponentBitmapBitLength: %d bits\n", hdr->componentBitmapBitLength);

    printf("PackageVersionStringType: ");
    pldmFwPrintStringType(hdr->packageVersionStringType);

    printf("PackageVersionStringLength: %d\n", hdr->packageVersionStringLength);

    /* Read the package version string from the buffer */
    uint8_t* pkgVerStr = &buffer[index];
    index += hdr->packageVersionStringLength;

    printf("PackageVersionString: ");
    for (i = 0; i < hdr->packageVersionStringLength; i++)
    {
        printf("%c", pkgVerStr[i]);
    }
    printf("\n\n");

    /*
     * 2) The next byte  store "DeviceIDRecordCount" 
     */
    uint8_t deviceIdRecordCount = buffer[index];
    index += 1;
    printf("--------------------------------------------\n");
    printf("Firmware Device ID Record Count: %d\n\n", deviceIdRecordCount);

    int rec;
    for (rec = 0; rec < deviceIdRecordCount; rec++)
    {
        PLDMFirmwareDeviceIdRecord* devRec = (PLDMFirmwareDeviceIdRecord*)&buffer[index];
        index += sizeof(PLDMFirmwareDeviceIdRecord);

        printf("=== Device ID Record #%d ===\n", rec);
        printf("  RecordLength: %d\n", devRec->recordLength);
        printf("  DescriptorCount: %d\n", devRec->descriptorCount);
        printf("  DeviceUpdateOptionFlags: 0x%08x\n", devRec->deviceUpdateOptionFlags);

        printf("  ComponentImageSetVersionStringType: ");
        pldmFwPrintStringType(devRec->componentImageSetVersionStringType);

        printf("  ComponentImageSetVersionStringLength: %d\n",
               devRec->componentImageSetVersionStringLength);

        uint16_t fwPkgDataLen = devRec->firmwareDevicePackageDataLength;

        /* parse "applicableComponents" bitfield of length compBitmapBytes if needed: */
        int compBitmapBytes = hdr->componentBitmapBitLength / 8;
        if (compBitmapBytes > 0)
        {
            uint8_t* bitfieldPtr = &buffer[index];
            index += compBitmapBytes;

            printf("  ApplicableComponents bitfield (size=%d bytes):\n", compBitmapBytes);
            /* for each byte, find set bits with pldmFwGetPowerOfTwoExponent */
            int b;
            for (b = 0; b < compBitmapBytes; b++)
            {
                uint8_t bits = bitfieldPtr[b];
                while (bits)
                {
                    uint8_t setBit = bits & (-bits);
                    bits ^= setBit; 
                    int bitPos = pldmFwGetPowerOfTwoExponent(setBit);
                    printf("    => Component #%d Image in the PLDM payload\n", bitPos + b*8);
                }
            }
        }

        /* parse the version string => devRec->componentImageSetVersionStringLength bytes */
        if (devRec->componentImageSetVersionStringLength > 0)
        {
            uint8_t* verStrPtr = &buffer[index];
            index += devRec->componentImageSetVersionStringLength;

            printf("  ComponentImageSetVersionString: ");
            for (i = 0; i < devRec->componentImageSetVersionStringLength; i++)
            {
                printf("%c", verStrPtr[i]);
            }
            printf("\n");
        }

        /* parse descriptors => devRec->descriptorCount each
           e.g. "PLDMDescriptorEntry" with (2 bytes type, 2 bytes length, variable data) */
        int d;
        for (d = 0; d < devRec->descriptorCount; d++)
        {
            PLDMDescriptorEntry* desc = (PLDMDescriptorEntry*)&buffer[index];
            index += sizeof(PLDMDescriptorEntry);

            printf("  Descriptor #%d:\n", d);
            printf("    Type: ");
            pldmFwPrintDescriptorID((uint8_t)(desc->descriptorType & 0xFF),
                                    (uint8_t)(desc->descriptorType >> 8));
            printf("    Length: %d bytes\n", desc->descriptorLength);

            if (desc->descriptorLength > 0)
            {
                int descLen = desc->descriptorLength;
                uint8_t* descData = &buffer[index];
                index += descLen;

                printf("    Descriptor Data(hex):");
                int x;
                for (x = 0; x < descLen; x++)
                {
                    printf(" %02x", descData[x]);
                }
                printf("\n");
            }
        }

        /* parse "firmwareDevicePackageData" => devRec->firmwareDevicePackageDataLength */
        if (fwPkgDataLen > 0)
        {
            printf("  FirmwareDevicePackageDataLength: %d bytes\n", fwPkgDataLen);
            uint8_t* pkgDataPtr = &buffer[index];
            index += fwPkgDataLen;

            printf("  FirmwareDevicePackageData (hex):");
            for (i = 0; i < fwPkgDataLen; i++)
            {
                printf(" %02x", pkgDataPtr[i]);
            }
            printf("\n");
        }
        else
        {
            printf("  FirmwareDevicePackageDataLength: 0 (none)\n");
        }
        printf("\n");
    }

    /* 3) parse the ComponentImageCount (2 bytes, LE) */
    uint16_t compImgCount = pldmFwExtractUint16LE(buffer[index], buffer[index+1]);
    index += 2;
    printf("--------------------------------------------\n");
    printf("Component Image Count: %d\n\n", compImgCount);

    /* parse each PLDMComponentImageInfo */
    int comp;
    for (comp = 0; comp < compImgCount; comp++)
    {
        PLDMComponentImageInfo* compInfo = (PLDMComponentImageInfo*)&buffer[index];
        index += sizeof(PLDMComponentImageInfo);

        printf("=== Component Image #%d ===\n", comp);
        printf("  Classification: ");
        pldmFwPrintComponentClassification(
            (uint8_t)(compInfo->componentClassification & 0xFF),
            (uint8_t)(compInfo->componentClassification >> 8));
        printf("  Identifier: 0x%04x\n", compInfo->componentIdentifier);
        printf("  ComparisonStamp: 0x%08x\n", compInfo->componentComparisonStamp);
        printf("  ComponentOptions: 0x%04x\n", compInfo->componentOptions);
        printf("  RequestedCompActivationMethod: 0x%04x\n",
               compInfo->requestedCompActivationMethod);
        printf("  ComponentLocationOffset: 0x%08x\n", compInfo->componentLocationOffset);
        printf("  ComponentSize: %d bytes\n", compInfo->componentSize);

        printf("  VersionStringType: ");
        pldmFwPrintStringType(compInfo->componentVersionStringType);

        printf("  VersionStringLength: %d\n", compInfo->componentVersionStringLength);

        if (compInfo->componentVersionStringLength > 0)
        {
            uint8_t* cVerPtr = &buffer[index];
            int vsLen = compInfo->componentVersionStringLength;
            index += vsLen;

            printf("  ComponentVersionString: ");
            for (i = 0; i < vsLen; i++)
            {
                printf("%c", cVerPtr[i]);
            }
            printf("\n");
        }
        printf("\n");
    }

    /* 4) parse the final 4-byte checksum (CRC32) if within range */
    if ((index + 4) <= pkgHeaderSize)
    {
        uint32_t sumVal = pldmFwExtractUint32LE(
            buffer[index], buffer[index+1],
            buffer[index+2], buffer[index+3]);
        index += 4;

        printf("--------------------------------------------\n");
        printf("Package Header Checksum (CRC32): 0x%08x\n", sumVal);
    }
    else
    {
        printf("--------------------------------------------\n");
        printf("No checksum found (index out of range)\n");
    }

    free(buffer);
    return 0;
}

int main(int argc, char** argv)
{
    /* ./parse_pldm my_fw_file.bin */
    if (argc < 2)
    {
        char filePath[256];
        printf("Please enter the file path to parse: ");
        if (!fgets(filePath, sizeof(filePath), stdin))
        {
            fprintf(stderr, "Error reading input.\n");
            return 1;
        }
        size_t len = strlen(filePath);
        if (len > 0 && filePath[len-1] == '\n')
        {
            filePath[len-1] = '\0';
        }
        return parseFile(filePath);
    }
    else
    {
        return parseFile(argv[1]);
    }
}

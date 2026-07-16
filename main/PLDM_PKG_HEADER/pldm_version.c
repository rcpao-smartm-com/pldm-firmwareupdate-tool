#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pldmlib.h"

/* Encoded as 10=1.0, 11=1.1, 12=1.2, 13=1.3 (DSP0267). */
int pldm_dsp0267_ver = 10;

int pldm_parse_spec_version(const char *s){
  if (!s)
    return -1;
  if (strcmp(s, "1.0") == 0)
    return 10;
  if (strcmp(s, "1.1") == 0)
    return 11;
  if (strcmp(s, "1.2") == 0)
    return 12;
  if (strcmp(s, "1.3") == 0)
    return 13;
  return -1;
}

void pldm_spec_version_string(char *out, size_t out_len){
  if (!out || out_len < 4)
    return;
  switch (pldm_dsp0267_ver){
  case 11:
    snprintf(out, out_len, "1.1");
    break;
  case 12:
    snprintf(out, out_len, "1.2");
    break;
  case 13:
    snprintf(out, out_len, "1.3");
    break;
  default:
    snprintf(out, out_len, "1.0");
    break;
  }
}

void pldm_get_package_header_id(uint8_t out[16]){
  static const uint8_t id_1_0[16] = {
      0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43,
      0x98, 0x00, 0xA0, 0x2F, 0x05, 0x9A, 0xCA, 0x02};
  static const uint8_t id_1_1[16] = {
      0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
      0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5A};
  static const uint8_t id_1_2[16] = {
      0x31, 0x19, 0xCE, 0x2F, 0xE8, 0x0A, 0x4A, 0x99,
      0xAF, 0x6D, 0x46, 0xF8, 0xB1, 0x21, 0xF6, 0xBF};
  static const uint8_t id_1_3[16] = {
      0x7B, 0x29, 0x1C, 0x99, 0x6D, 0xB6, 0x42, 0x08,
      0x80, 0x1B, 0x02, 0x02, 0x6E, 0x46, 0x3C, 0x78};
  const uint8_t *src = id_1_0;
  switch (pldm_dsp0267_ver){
  case 11: src = id_1_1; break;
  case 12: src = id_1_2; break;
  case 13: src = id_1_3; break;
  default: break;
  }
  memcpy(out, src, 16);
}

uint8_t pldm_get_package_header_revision(void){
  switch (pldm_dsp0267_ver){
  case 11: return 0x02;
  case 12: return 0x03;
  case 13: return 0x04;
  default: return 0x01;
  }
}

int pldm_trailing_checksum_bytes(void){
  return (pldm_dsp0267_ver >= 13) ? (CHECKSUM_SIZE * 2) : CHECKSUM_SIZE;
}

int pldm_has_downstream_area(void){
  return pldm_dsp0267_ver >= 11;
}

int pldm_has_component_opaque(void){
  return pldm_dsp0267_ver >= 12;
}

int pldm_has_reference_manifest(void){
  return pldm_dsp0267_ver >= 13;
}

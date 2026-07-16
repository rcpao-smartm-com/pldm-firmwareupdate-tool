#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <json-c/json.h>
#include "pldmlib.h"

json_pldm all;
json_layer1 l1;
json_layer2 l2;
json_layer3 l3;

int main(int argc, char *argv[]) {

  int pkg_info_size;
  int fd_id_size;
  int compo_info_size;
  uint16_t compo_bitmap_len;
  uint8_t *pkg_info_buf;
  uint8_t *fd_id_buf;
  uint8_t *compo_info_buf;
  char *img_file = NULL;
  char *fw_version = NULL;
  const char *spec_arg = "1.0";

  if (argc == 2){
    spec_arg = argv[1];
  } else if (argc == 4){
    img_file = argv[1];
    fw_version = argv[2];
    spec_arg = argv[3];
  } else if (argc != 1){
    printf("Usage: %s [<spec_version>]\n", argv[0]);
    printf("       %s <image_file> <firmware_version> <spec_version>\n", argv[0]);
    printf("  spec_version: 1.0 | 1.1 | 1.2 | 1.3 (DSP0267)\n");
    return 1;
  }
  int spec = pldm_parse_spec_version(spec_arg);
  if (spec < 0){
    printf("Invalid spec_version '%s' (use 1.0, 1.1, 1.2, or 1.3)\n", spec_arg);
    return 1;
  }
  pldm_dsp0267_ver = spec;

  int error = parsed_all();
  if (error == 1){
    printf("Parsing PLDM.json failed\n");
    return 1;
  }

  error = parsed_layer1();
  if (error == 1){
    printf("Parsing Package Header Information failed\n");
    return 1;
  }

  error = parsed_layer2();
  if (error == 1){
    printf("Parsing Firmware Device ID Record failed\n");
    return 1;
  }

  error = parsed_layer3();
  if (error == 1){
    printf("Parsing Component Image Information failed\n");
    return 1;
  }

  /* Optional CLI overrides for component image path / version string in PLDM.json */
  if (img_file && fw_version){
    if (json_object_array_length(l3.compo_img_list) < 1){
      printf("PLDM.json has no component images to override\n");
      return 1;
    }
    l3.compo_img = json_object_array_get_idx(l3.compo_img_list, 0);
    json_object_object_get_ex(l3.compo_img, "Component Image", &l3.compo_size);
    json_object_object_get_ex(l3.compo_img, "Component Version String", &l3.compo_ver_str);
    json_object_set_string(l3.compo_size, img_file);
    json_object_set_string(l3.compo_ver_str, fw_version);
  }

  fd_id_buf = FD_ID(&fd_id_size, &compo_bitmap_len);
  if (!fd_id_buf){
    printf("Firmware Device ID Record Encoding ERROR\n");
    return 1;
  }

  pkg_info_buf = pkg_info(&pkg_info_size, &compo_bitmap_len);
  if (!pkg_info_buf){
    printf("Package Header Information Encoding ERROR\n");
    free(fd_id_buf);
    return 1;
  }
  uint8_t downstream_count = 0;
  int downstream_bytes = pldm_has_downstream_area() ? 1 : 0;
  uint16_t cur_size = (uint16_t)(pkg_info_size + fd_id_size + downstream_bytes);

  compo_info_buf = compo(&compo_info_size, &cur_size);
  if (!compo_info_buf){
    printf("Component Image Information Encoding ERROR\n");
    free(pkg_info_buf);
    free(fd_id_buf);
    return 1;
  }
  int trail = pldm_trailing_checksum_bytes();
  cur_size = (uint16_t)(cur_size + compo_info_size + trail);

  /* Writing the whole package header size(included checksum field) into the PKG Header INFO AREA */
  memcpy(pkg_info_buf + PKG_HEADER_SIZE_OFFSET, &cur_size, sizeof(cur_size));
  FILE *b_file;
	b_file = fopen("pldm_update_header.bin","wb");
  if(!b_file){
    perror("Failed\n");
    free(compo_info_buf);
    free(pkg_info_buf);
    free(fd_id_buf);
    return 1;
  }
  printf("Create the pldm_update_header.bin successfully\n");

  fwrite(pkg_info_buf, pkg_info_size, 1, b_file);
  free(pkg_info_buf);
  fwrite(fd_id_buf, fd_id_size, 1, b_file);
  free(fd_id_buf);
  if (downstream_bytes)
    fwrite(&downstream_count, 1, 1, b_file);
  fwrite(compo_info_buf, compo_info_size, 1, b_file);
  free(compo_info_buf);
  fclose(b_file);

  /* Header CRC excludes PackageHeaderChecksum and (1.3) PackagePayloadChecksum */
  uint16_t header_crc_len = (uint16_t)(cur_size - trail);
	b_file = fopen("pldm_update_header.bin","rb");
  if(!b_file){
    perror("Failed\n");
    return 1;
  }

  unsigned char *pldm = malloc(header_crc_len * sizeof(unsigned char));
  if(!pldm){
    perror("Failed\n");
    fclose(b_file);
    return 1;
  }
  if (fread(pldm, 1, header_crc_len, b_file) != header_crc_len){
    perror("Failed reading header for CRC\n");
    free(pldm);
    fclose(b_file);
    return 1;
  }
	fclose(b_file);

  unsigned long crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, (const unsigned char*)pldm, header_crc_len);
  printf("Checksum: %lx\n",crc);
  free(pldm);

  /* Writing the checksum(4 Bytes) into the end of PLDM FW PKG Header*/
	b_file = fopen("pldm_update_header.bin","ab");
  if(!b_file){
    perror("Failed\n");
    return 1;
  }
  uint32_t header_crc32 = (uint32_t)crc;
  fwrite(&header_crc32, CHECKSUM_SIZE, 1, b_file);
  if (pldm_dsp0267_ver >= 13){
    FILE *payload = fopen("image_payload.bin", "rb");
    if(!payload){
      perror("Failed opening image_payload.bin\n");
      fclose(b_file);
      return 1;
    }
    unsigned long payload_crc = crc32(0L, Z_NULL, 0);
    unsigned char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), payload)) > 0)
      payload_crc = crc32(payload_crc, buf, (uInt)n);
    fclose(payload);
    uint32_t payload_crc32 = (uint32_t)payload_crc;
    fwrite(&payload_crc32, CHECKSUM_SIZE, 1, b_file);
  }
	fclose(b_file);

  char spec_str[8];
  pldm_spec_version_string(spec_str, sizeof(spec_str));
  char out_name[256];
  if (img_file){
    char *base = strrchr(img_file, '/');
    base = base ? base + 1 : img_file;
    snprintf(out_name, sizeof(out_name), "pldm%s-%s", spec_str, base);
  } else {
    snprintf(out_name, sizeof(out_name), "pldm%s-update_pkg.bin", spec_str);
  }
  char sys_cmd[1024];
  snprintf(sys_cmd, sizeof(sys_cmd),
           "cp pldm_update_header.bin %s && cat image_payload.bin >> %s", out_name, out_name);
  system(sys_cmd);
  printf("Wrote %s\n", out_name);

  return 0;
}

  // FILE *img;
	// img = fopen("img_2.bin","wb");
  // uint8_t *img_bin = malloc(60 * sizeof(uint8_t));
  // for(int i = 0; i < 20; i++)
  //       *(img_bin+i) = 3;
  // for(int i = 20; i < 40; i++)
  //       *(img_bin+i) = 0x33;
  // for(int i = 40; i < 60; i++)
  //       *(img_bin+i) = 3;
  // // printf("%d\n", *img_bin);
  // fwrite(img_bin, 60, 1, img);
  // fclose(img);

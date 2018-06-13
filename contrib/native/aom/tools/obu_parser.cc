/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <cstdio>
#include <string>

// TODO(tomfinegan): Remove the aom_config.h include as soon as possible. At
// present it's required because aom_config.h determines if the library writes
// OBUs.
#include "./aom_config.h"

#include "aom/aom_codec.h"
#include "aom/aom_integer.h"
#include "aom_ports/mem_ops.h"
#include "av1/decoder/obu.h"
#include "tools/obu_parser.h"

namespace aom_tools {

// Basic OBU syntax
// 4 bytes: length
// 8 bits: Header
//   7
//     forbidden bit
//   6,5,4,3
//     type bits
//   2,1
//     reserved bits
//   0
//     extension bit
const uint32_t kObuForbiddenBitMask = 0x1;
const uint32_t kObuForbiddenBitShift = 7;
const uint32_t kObuTypeBitsMask = 0xF;
const uint32_t kObuTypeBitsShift = 3;
const uint32_t kObuExtensionFlagBitMask = 0x1;
const uint32_t kObuExtensionFlagBitShift = 2;
const uint32_t kObuHasPayloadLengthFlagBitMask = 0x1;
const uint32_t kObuHasPayloadLengthFlagBitShift = 1;

// When extension bit is set:
// 8 bits: extension header
// 7,6,5
//   temporal ID
// 4,3
//   spatial ID
// 2,1
//   quality ID
// 0
//   reserved bit
const uint32_t kObuExtTemporalIdBitsMask = 0x7;
const uint32_t kObuExtTemporalIdBitsShift = 5;
const uint32_t kObuExtSpatialIdBitsMask = 0x3;
const uint32_t kObuExtSpatialIdBitsShift = 3;

bool ValidObuType(int obu_type) {
  switch (obu_type) {
    case OBU_SEQUENCE_HEADER:
    case OBU_TEMPORAL_DELIMITER:
    case OBU_FRAME_HEADER:
    case OBU_TILE_GROUP:
    case OBU_REDUNDANT_FRAME_HEADER:
    case OBU_FRAME:
    case OBU_METADATA:
    case OBU_PADDING: return true;
  }
  return false;
}

bool ParseObuHeader(uint8_t obu_header_byte, ObuHeader *obu_header) {
  const int forbidden_bit =
      (obu_header_byte >> kObuForbiddenBitShift) & kObuForbiddenBitMask;
  if (forbidden_bit) {
    fprintf(stderr, "Invalid OBU, forbidden bit set.\n");
    return false;
  }

  obu_header->type = static_cast<OBU_TYPE>(
      (obu_header_byte >> kObuTypeBitsShift) & kObuTypeBitsMask);
  if (!ValidObuType(obu_header->type)) {
    fprintf(stderr, "Invalid OBU type: %d.\n", obu_header->type);
    return false;
  }

  obu_header->has_extension =
      (obu_header_byte >> kObuExtensionFlagBitShift) & kObuExtensionFlagBitMask;
  obu_header->has_length_field =
      (obu_header_byte >> kObuHasPayloadLengthFlagBitShift) &
      kObuHasPayloadLengthFlagBitMask;
  return true;
}

bool ParseObuExtensionHeader(uint8_t ext_header_byte, ObuHeader *obu_header) {
  obu_header->temporal_layer_id =
      (ext_header_byte >> kObuExtTemporalIdBitsShift) &
      kObuExtTemporalIdBitsMask;
  obu_header->enhancement_layer_id =
      (ext_header_byte >> kObuExtSpatialIdBitsShift) & kObuExtSpatialIdBitsMask;

  return true;
}

void PrintObuHeader(const ObuHeader *header) {
  printf(
      "  OBU type:        %s\n"
      "      extension:   %s\n",
      aom_obu_type_to_string(static_cast<OBU_TYPE>(header->type)),
      header->has_extension ? "yes" : "no");
  if (header->has_extension) {
    printf(
        "      temporal_id: %d\n"
        "      spatial_id:  %d\n",
        header->temporal_layer_id, header->temporal_layer_id);
  }
}

bool DumpObu(const uint8_t *data, int length, int *obu_overhead_bytes) {
  const int kObuHeaderLengthSizeBytes = 1;
  const int kMinimumBytesRequired = 1 + kObuHeaderLengthSizeBytes;
  int consumed = 0;
  int obu_overhead = 0;
  ObuHeader obu_header;
  while (consumed < length) {
    const int remaining = length - consumed;
    if (remaining < kMinimumBytesRequired) {
      if (remaining > 0) {
        fprintf(stderr,
                "OBU parse error. Did not consume all data, %d bytes "
                "remain.\n",
                remaining);
      }
      return false;
    }

    size_t length_field_size = 0;
    int current_obu_length = 0;
    int obu_header_size = 0;

    obu_overhead += (int)length_field_size;

    if (current_obu_length > remaining) {
      fprintf(stderr,
              "OBU parsing failed at offset %d with bad length of %d "
              "and %d bytes left.\n",
              consumed, current_obu_length, remaining);
      return false;
    }
    consumed += (int)length_field_size;

    memset(&obu_header, 0, sizeof(obu_header));
    const uint8_t obu_header_byte = *(data + consumed);
    if (!ParseObuHeader(obu_header_byte, &obu_header)) {
      fprintf(stderr, "OBU parsing failed at offset %d.\n", consumed);
      return false;
    }

    ++obu_overhead;
    ++obu_header_size;

    if (obu_header.has_extension) {
      const uint8_t obu_ext_header_byte =
          *(data + consumed + kObuHeaderLengthSizeBytes);
      if (!ParseObuExtensionHeader(obu_ext_header_byte, &obu_header)) {
        fprintf(stderr, "OBU extension parsing failed at offset %d.\n",
                consumed);
        return false;
      }

      ++obu_overhead;
      ++obu_header_size;
    }

    PrintObuHeader(&obu_header);

    uint64_t obu_size = 0;
    aom_uleb_decode(data + consumed + obu_header_size, remaining, &obu_size,
                    &length_field_size);
    current_obu_length = static_cast<int>(obu_size);
    consumed += obu_header_size + (int)length_field_size + current_obu_length;
    printf("      length:      %d\n",
           static_cast<int>(obu_header_size + length_field_size +
                            current_obu_length));
  }

  if (obu_overhead_bytes != nullptr) *obu_overhead_bytes = obu_overhead;
  printf("  TU size: %d\n", consumed);

  return true;
}

}  // namespace aom_tools

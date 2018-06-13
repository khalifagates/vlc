/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AV1_COMMON_IDCT_H_
#define AV1_COMMON_IDCT_H_

#include "./aom_config.h"
#include "av1/common/blockd.h"
#include "av1/common/common.h"
#include "av1/common/enums.h"
#include "aom_dsp/inv_txfm.h"
#include "aom_dsp/txfm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*transform_1d)(const tran_low_t *, tran_low_t *);

typedef struct {
  transform_1d cols, rows;  // vertical and horizontal
} transform_2d;

#define MAX_TX_SCALE 1
int av1_get_tx_scale(const TX_SIZE tx_size);

void av1_inverse_transform_block(const MACROBLOCKD *xd,
                                 const tran_low_t *dqcoeff, int plane,
                                 TX_TYPE tx_type, TX_SIZE tx_size, uint8_t *dst,
                                 int stride, int eob, int reduced_tx_set);

void av1_highbd_inv_txfm_add_4x4(const tran_low_t *input, uint8_t *dest,
                                 int stride, const TxfmParam *param);
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AV1_COMMON_IDCT_H_

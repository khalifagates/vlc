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
#include "aom/aom_codec.h"
static const char* const cfg = "-G \"Unix Makefiles\" -DCMAKE_TOOLCHAIN_FILE=\"/home/boogie/vlc/contrib/native/toolchain.cmake\" -DCONFIG_DEPENDENCY_TRACKING=0 -DCONFIG_INSTALL_BINS=0 -DCONFIG_INSTALL_DOCS=0 -DCONFIG_PIC=1 -DCONFIG_RUNTIME_CPU_DETECT=1 -DCONFIG_UNIT_TESTS=0";
const char *aom_codec_build_config(void) {return cfg;}

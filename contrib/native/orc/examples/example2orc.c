
/* autogenerated from example2orc.orc */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _ORC_INTEGER_TYPEDEFS_
#define _ORC_INTEGER_TYPEDEFS_
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef int8_t orc_int8;
typedef int16_t orc_int16;
typedef int32_t orc_int32;
typedef int64_t orc_int64;
typedef uint8_t orc_uint8;
typedef uint16_t orc_uint16;
typedef uint32_t orc_uint32;
typedef uint64_t orc_uint64;
#define ORC_UINT64_C(x) UINT64_C(x)
#elif defined(_MSC_VER)
typedef signed __int8 orc_int8;
typedef signed __int16 orc_int16;
typedef signed __int32 orc_int32;
typedef signed __int64 orc_int64;
typedef unsigned __int8 orc_uint8;
typedef unsigned __int16 orc_uint16;
typedef unsigned __int32 orc_uint32;
typedef unsigned __int64 orc_uint64;
#define ORC_UINT64_C(x) (x##Ui64)
#define inline __inline
#else
#include <limits.h>
typedef signed char orc_int8;
typedef short orc_int16;
typedef int orc_int32;
typedef unsigned char orc_uint8;
typedef unsigned short orc_uint16;
typedef unsigned int orc_uint32;
#if INT_MAX == LONG_MAX
typedef long long orc_int64;
typedef unsigned long long orc_uint64;
#define ORC_UINT64_C(x) (x##ULL)
#else
typedef long orc_int64;
typedef unsigned long orc_uint64;
#define ORC_UINT64_C(x) (x##UL)
#endif
#endif
typedef union { orc_int16 i; orc_int8 x2[2]; } orc_union16;
typedef union { orc_int32 i; float f; orc_int16 x2[2]; orc_int8 x4[4]; } orc_union32;
typedef union { orc_int64 i; double f; orc_int32 x2[2]; float x2f[2]; orc_int16 x4[4]; } orc_union64;
#endif
#ifndef ORC_RESTRICT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define ORC_RESTRICT restrict
#elif defined(__GNUC__) && __GNUC__ >= 4
#define ORC_RESTRICT __restrict__
#else
#define ORC_RESTRICT
#endif
#endif

#ifndef ORC_INTERNAL
#if defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define ORC_INTERNAL __attribute__((visibility("hidden")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#define ORC_INTERNAL __hidden
#elif defined (__GNUC__)
#define ORC_INTERNAL __attribute__((visibility("hidden")))
#else
#define ORC_INTERNAL
#endif
#endif


#ifndef DISABLE_ORC
#include <orc/orc.h>
#endif
void audio_add_mono_to_stereo_scaled_s16 (short * ORC_RESTRICT d1, const short * ORC_RESTRICT s1, const short * ORC_RESTRICT s2, int p1, int n);


/* begin Orc C target preamble */
#define ORC_CLAMP(x,a,b) ((x)<(a) ? (a) : ((x)>(b) ? (b) : (x)))
#define ORC_ABS(a) ((a)<0 ? -(a) : (a))
#define ORC_MIN(a,b) ((a)<(b) ? (a) : (b))
#define ORC_MAX(a,b) ((a)>(b) ? (a) : (b))
#define ORC_SB_MAX 127
#define ORC_SB_MIN (-1-ORC_SB_MAX)
#define ORC_UB_MAX 255
#define ORC_UB_MIN 0
#define ORC_SW_MAX 32767
#define ORC_SW_MIN (-1-ORC_SW_MAX)
#define ORC_UW_MAX 65535
#define ORC_UW_MIN 0
#define ORC_SL_MAX 2147483647
#define ORC_SL_MIN (-1-ORC_SL_MAX)
#define ORC_UL_MAX 4294967295U
#define ORC_UL_MIN 0
#define ORC_CLAMP_SB(x) ORC_CLAMP(x,ORC_SB_MIN,ORC_SB_MAX)
#define ORC_CLAMP_UB(x) ORC_CLAMP(x,ORC_UB_MIN,ORC_UB_MAX)
#define ORC_CLAMP_SW(x) ORC_CLAMP(x,ORC_SW_MIN,ORC_SW_MAX)
#define ORC_CLAMP_UW(x) ORC_CLAMP(x,ORC_UW_MIN,ORC_UW_MAX)
#define ORC_CLAMP_SL(x) ORC_CLAMP(x,ORC_SL_MIN,ORC_SL_MAX)
#define ORC_CLAMP_UL(x) ORC_CLAMP(x,ORC_UL_MIN,ORC_UL_MAX)
#define ORC_SWAP_W(x) ((((x)&0xff)<<8) | (((x)&0xff00)>>8))
#define ORC_SWAP_L(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))
#define ORC_SWAP_Q(x) ((((x)&ORC_UINT64_C(0xff))<<56) | (((x)&ORC_UINT64_C(0xff00))<<40) | (((x)&ORC_UINT64_C(0xff0000))<<24) | (((x)&ORC_UINT64_C(0xff000000))<<8) | (((x)&ORC_UINT64_C(0xff00000000))>>8) | (((x)&ORC_UINT64_C(0xff0000000000))>>24) | (((x)&ORC_UINT64_C(0xff000000000000))>>40) | (((x)&ORC_UINT64_C(0xff00000000000000))>>56))
#define ORC_PTR_OFFSET(ptr,offset) ((void *)(((unsigned char *)(ptr)) + (offset)))
#define ORC_DENORMAL(x) ((x) & ((((x)&0x7f800000) == 0) ? 0xff800000 : 0xffffffff))
#define ORC_ISNAN(x) ((((x)&0x7f800000) == 0x7f800000) && (((x)&0x007fffff) != 0))
#define ORC_DENORMAL_DOUBLE(x) ((x) & ((((x)&ORC_UINT64_C(0x7ff0000000000000)) == 0) ? ORC_UINT64_C(0xfff0000000000000) : ORC_UINT64_C(0xffffffffffffffff)))
#define ORC_ISNAN_DOUBLE(x) ((((x)&ORC_UINT64_C(0x7ff0000000000000)) == ORC_UINT64_C(0x7ff0000000000000)) && (((x)&ORC_UINT64_C(0x000fffffffffffff)) != 0))
#ifndef ORC_RESTRICT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define ORC_RESTRICT restrict
#elif defined(__GNUC__) && __GNUC__ >= 4
#define ORC_RESTRICT __restrict__
#else
#define ORC_RESTRICT
#endif
#endif
/* end Orc C target preamble */



/* audio_add_mono_to_stereo_scaled_s16 */
#ifdef DISABLE_ORC
void
audio_add_mono_to_stereo_scaled_s16 (short * ORC_RESTRICT d1, const short * ORC_RESTRICT s1, const short * ORC_RESTRICT s2, int p1, int n){
  int i;
  orc_union32 * ORC_RESTRICT ptr0;
  const orc_union32 * ORC_RESTRICT ptr4;
  const orc_union16 * ORC_RESTRICT ptr5;
  orc_union16 var35;
  orc_union16 var36;
  orc_union32 var37;
  orc_union32 var38;
  orc_union32 var39;
  orc_union32 var40;
  orc_union16 var41;
  orc_union32 var42;

  ptr0 = (orc_union32 *)d1;
  ptr4 = (orc_union32 *)s1;
  ptr5 = (orc_union16 *)s2;

    /* 1: loadpw */
    var36.i = p1;

  for (i = 0; i < n; i++) {
    /* 0: loadw */
    var35 = ptr5[i];
    /* 2: mulswl */
    var39.i = var35.i * var36.i;
    /* 3: shrsl */
    var40.i = var39.i >> 12;
    /* 4: convssslw */
    var41.i = ORC_CLAMP_SW(var40.i);
    /* 5: mergewl */
    {
       orc_union32 _dest;
       _dest.x2[0] = var41.i;
       _dest.x2[1] = var41.i;
       var42.i = _dest.i;
    }
    /* 6: loadl */
    var37 = ptr4[i];
    /* 7: addssw */
    var38.x2[0] = ORC_CLAMP_SW(var37.x2[0] + var42.x2[0]);
    var38.x2[1] = ORC_CLAMP_SW(var37.x2[1] + var42.x2[1]);
    /* 8: storel */
    ptr0[i] = var38;
  }

}

#else
static void
_backup_audio_add_mono_to_stereo_scaled_s16 (OrcExecutor * ORC_RESTRICT ex)
{
  int i;
  int n = ex->n;
  orc_union32 * ORC_RESTRICT ptr0;
  const orc_union32 * ORC_RESTRICT ptr4;
  const orc_union16 * ORC_RESTRICT ptr5;
  orc_union16 var35;
  orc_union16 var36;
  orc_union32 var37;
  orc_union32 var38;
  orc_union32 var39;
  orc_union32 var40;
  orc_union16 var41;
  orc_union32 var42;

  ptr0 = (orc_union32 *)ex->arrays[0];
  ptr4 = (orc_union32 *)ex->arrays[4];
  ptr5 = (orc_union16 *)ex->arrays[5];

    /* 1: loadpw */
    var36.i = ex->params[24];

  for (i = 0; i < n; i++) {
    /* 0: loadw */
    var35 = ptr5[i];
    /* 2: mulswl */
    var39.i = var35.i * var36.i;
    /* 3: shrsl */
    var40.i = var39.i >> 12;
    /* 4: convssslw */
    var41.i = ORC_CLAMP_SW(var40.i);
    /* 5: mergewl */
    {
       orc_union32 _dest;
       _dest.x2[0] = var41.i;
       _dest.x2[1] = var41.i;
       var42.i = _dest.i;
    }
    /* 6: loadl */
    var37 = ptr4[i];
    /* 7: addssw */
    var38.x2[0] = ORC_CLAMP_SW(var37.x2[0] + var42.x2[0]);
    var38.x2[1] = ORC_CLAMP_SW(var37.x2[1] + var42.x2[1]);
    /* 8: storel */
    ptr0[i] = var38;
  }

}

void
audio_add_mono_to_stereo_scaled_s16 (short * ORC_RESTRICT d1, const short * ORC_RESTRICT s1, const short * ORC_RESTRICT s2, int p1, int n)
{
  OrcExecutor _ex, *ex = &_ex;
  static volatile int p_inited = 0;
  static OrcCode *c = 0;
  void (*func) (OrcExecutor *);

  if (!p_inited) {
    orc_once_mutex_lock ();
    if (!p_inited) {
      OrcProgram *p;

#if 1
    static const orc_uint8 bc[] = {
      1, 9, 35, 97, 117, 100, 105, 111, 95, 97, 100, 100, 95, 109, 111, 110, 
      111, 95, 116, 111, 95, 115, 116, 101, 114, 101, 111, 95, 115, 99, 97, 108, 
      101, 100, 95, 115, 49, 54, 11, 4, 4, 12, 4, 4, 12, 2, 2, 14, 
      4, 12, 0, 0, 0, 16, 2, 20, 4, 20, 2, 20, 4, 176, 32, 5, 
      24, 125, 32, 32, 16, 165, 33, 32, 195, 34, 33, 33, 21, 1, 71, 0, 
      4, 34, 2, 0, 
    };
    p = orc_program_new_from_static_bytecode (bc);
    orc_program_set_backup_function (p, _backup_audio_add_mono_to_stereo_scaled_s16);
#else
      p = orc_program_new ();
      orc_program_set_name (p, "audio_add_mono_to_stereo_scaled_s16");
      orc_program_set_backup_function (p, _backup_audio_add_mono_to_stereo_scaled_s16);
      orc_program_add_destination (p, 4, "d1");
      orc_program_add_source (p, 4, "s1");
      orc_program_add_source (p, 2, "s2");
      orc_program_add_constant (p, 4, 0x0000000c, "c1");
      orc_program_add_parameter (p, 2, "p1");
      orc_program_add_temporary (p, 4, "t1");
      orc_program_add_temporary (p, 2, "t2");
      orc_program_add_temporary (p, 4, "t3");

      orc_program_append_2 (p, "mulswl", 0, ORC_VAR_T1, ORC_VAR_S2, ORC_VAR_P1, ORC_VAR_D1);
      orc_program_append_2 (p, "shrsl", 0, ORC_VAR_T1, ORC_VAR_T1, ORC_VAR_C1, ORC_VAR_D1);
      orc_program_append_2 (p, "convssslw", 0, ORC_VAR_T2, ORC_VAR_T1, ORC_VAR_D1, ORC_VAR_D1);
      orc_program_append_2 (p, "mergewl", 0, ORC_VAR_T3, ORC_VAR_T2, ORC_VAR_T2, ORC_VAR_D1);
      orc_program_append_2 (p, "addssw", 1, ORC_VAR_D1, ORC_VAR_S1, ORC_VAR_T3, ORC_VAR_D1);
#endif

      orc_program_compile (p);
      c = orc_program_take_code (p);
      orc_program_free (p);
    }
    p_inited = TRUE;
    orc_once_mutex_unlock ();
  }
  ex->arrays[ORC_VAR_A2] = c;
  ex->program = 0;

  ex->n = n;
  ex->arrays[ORC_VAR_D1] = d1;
  ex->arrays[ORC_VAR_S1] = (void *)s1;
  ex->arrays[ORC_VAR_S2] = (void *)s2;
  ex->params[ORC_VAR_P1] = p1;

  func = c->exec;
  func (ex);
}
#endif



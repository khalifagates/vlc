/*
Copyright (c) 2006-2009, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
  /*! \file    KM_prng.cpp
    \version $Id$
    \brief   Fortuna pseudo-random number generator
  */

#include <KM_prng.h>
  /*! \file    KM_prng.cpp
    \version $Id$
    \brief   Fortuna pseudo-random number generator
  */

#include <KM_prng.h>
#include <KM_log.h>
#include <KM_mutex.h>
#include <string.h>
#include <assert.h>
#include <nettle/sha1.h>
#include <nettle/aes.h>
#include <nettle/bignum.h>

using namespace Kumu;


#ifdef KM_WIN32
# include <wincrypt.h>
#else // KM_WIN32
# include <KM_fileio.h>
const char* DEV_URANDOM = "/dev/urandom";
#endif // KM_WIN32


const ui32_t RNG_KEY_SIZE = 512UL;
const ui32_t RNG_KEY_SIZE_BITS = 256UL;
const ui32_t RNG_BLOCK_SIZE = 16UL;
const ui32_t MAX_SEQUENCE_LEN = 0x00040000UL;


// internal implementation class
class h__RNG
{
  KM_NO_COPY_CONSTRUCT(h__RNG);

public:
  struct aes128_ctx   m_Context;
  byte_t    m_ctr_buf[RNG_BLOCK_SIZE];
  Mutex     m_Lock;

  h__RNG()
  {
    memset(m_ctr_buf, 0, RNG_BLOCK_SIZE);
    byte_t rng_key[RNG_KEY_SIZE];

    { // this block scopes the following AutoMutex so that it will be
      // released before the call to set_key() below.
      AutoMutex Lock(m_Lock);

#ifdef KM_WIN32
      HCRYPTPROV hProvider = 0;
      CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
      CryptGenRandom(hProvider, RNG_KEY_SIZE, rng_key);
#else // KM_WIN32
      // on POSIX systems we simply read some seed from /dev/urandom
      FileReader URandom;

      Result_t result = URandom.OpenRead(DEV_URANDOM);

      if ( KM_SUCCESS(result) )
	{
	  ui32_t read_count;
	  result = URandom.Read(rng_key, RNG_KEY_SIZE, &read_count);
	}

      if ( KM_FAILURE(result) )
	DefaultLogSink().Error("Error opening random device: %s\n", DEV_URANDOM);

#endif // KM_WIN32
    } // end AutoMutex context

    set_key(rng_key);
  }
	
  //
  void
  set_key(const byte_t* key_fodder)
  {
    assert(key_fodder);
    byte_t sha_buf[SHA1_DIGEST_SIZE];
    struct sha1_ctx SHA;
    sha1_init(&SHA);

    sha1_update(&SHA, sizeof(m_Context), (byte_t*)&m_Context );
    sha1_update(&SHA, sizeof(key_fodder), key_fodder);
    sha1_digest(&SHA, sizeof(sha_buf), sha_buf);

    AutoMutex Lock(m_Lock);
    aes128_set_encrypt_key(&m_Context, sha_buf);
    *(ui32_t*)(m_ctr_buf + 12) = 1;
  }
	
  //
  void
  fill_rand(byte_t* buf, ui32_t len)
  {
    assert(len <= MAX_SEQUENCE_LEN);
    ui32_t gen_count = 0;
    AutoMutex Lock(m_Lock);

    while ( gen_count + RNG_BLOCK_SIZE <= len )
      {
	aes128_encrypt(&m_Context,sizeof(m_ctr_buf), buf + gen_count,  m_ctr_buf);
	*(ui32_t*)(m_ctr_buf + 12) += 1;
	gen_count += RNG_BLOCK_SIZE;
      }
			
    if ( len != gen_count ) // partial count needed?
      {
	byte_t tmp[RNG_BLOCK_SIZE];
	aes128_encrypt(&m_Context,sizeof(m_ctr_buf),  tmp, m_ctr_buf );
	memcpy(buf + gen_count, tmp, len - gen_count);
      }
  }
};


static h__RNG* s_RNG = 0;


//------------------------------------------------------------------------------------------
//
// Fortuna public interface

Kumu::FortunaRNG::FortunaRNG()
{
  if ( s_RNG == 0 )
    s_RNG = new h__RNG;
}

Kumu::FortunaRNG::~FortunaRNG() {}

//
const byte_t*
Kumu::FortunaRNG::FillRandom(byte_t* buf, ui32_t len)
{
  assert(buf);
  assert(s_RNG);
  const byte_t* front_of_buffer = buf;

  while ( len )
    {
      // 2^20 bytes max per seeding, use 2^19 to save
      // room for generating reseed values
      ui32_t gen_size = xmin(len, MAX_SEQUENCE_LEN);
      s_RNG->fill_rand(buf, gen_size);
      buf += gen_size;
      len -= gen_size;
	  
      // re-seed the generator
      byte_t rng_key[RNG_KEY_SIZE];
      s_RNG->fill_rand(rng_key, RNG_KEY_SIZE);
      s_RNG->set_key(rng_key);
  }
  
  return front_of_buffer;
}

//
const byte_t*
Kumu::FortunaRNG::FillRandom(Kumu::ByteString& Buffer)
{
  FillRandom(Buffer.Data(), Buffer.Capacity());
  Buffer.Length(Buffer.Capacity());
  return Buffer.Data();
}

//------------------------------------------------------------------------------------------

//
// FIPS 186-2 Sec. 3.1 as modified by Change 1, section entitled "General Purpose Random Number Generation"
void
Kumu::Gen_FIPS_186_Value(const byte_t* key, ui32_t key_size, byte_t* out_buf, ui32_t out_buf_len)
{
  byte_t sha_buf[SHA1_DIGEST_SIZE];
  ui32_t const xkey_len = 64; // 512/8
  byte_t xkey[xkey_len];


  if ( key_size > xkey_len )
    DefaultLogSink().Warn("Key too large for FIPS 186 seed, truncating to 64 bytes.\n");

  // init key
  memset(xkey, 0, xkey_len);
  memcpy(xkey, key, xmin<ui32_t>(key_size, xkey_len));

  if ( key_size < SHA1_DIGEST_SIZE )
    key_size = SHA1_DIGEST_SIZE; // pad short key ( b < 160 )

  // create the 2^b constant
  mpz_t c_2powb, c_2, c_b, c_mod;
  mpz_init(c_2powb);
  mpz_init_set_si(c_2, 2);
  mpz_init_set_si(c_b, key_size * 8);
  mpz_init_set_si(c_mod, 1);
  mpz_powm(c_2powb, c_2, c_b, c_mod);

  for (;;)
    {
     struct sha1_ctx SHA;

      // step c -- x = G(t,xkey)
      sha1_init(&SHA); // set t
      sha1_update(&SHA, xkey_len, xkey);

      ui32_t* buf_p = (ui32_t*)sha_buf;
      *buf_p++ = KM_i32_BE(SHA.state[0]);
      *buf_p++ = KM_i32_BE(SHA.state[1]);
      *buf_p++ = KM_i32_BE(SHA.state[2]);
      *buf_p++ = KM_i32_BE(SHA.state[3]);
      *buf_p++ = KM_i32_BE(SHA.state[4]);
      memcpy(out_buf, sha_buf, xmin<ui32_t>(out_buf_len, SHA1_DIGEST_SIZE));

      if ( out_buf_len <= SHA1_DIGEST_SIZE )
	break;

      out_buf_len -= SHA1_DIGEST_SIZE;
      out_buf += SHA1_DIGEST_SIZE;

      // step d -- XKEY = (1 + XKEY + x) mod 2^b
      mpz_t bn_tmp, bn_xkey, bn_x_n;
      mpz_init(bn_tmp);
      mpz_init(bn_xkey);
      mpz_init(bn_x_n);

      mpz_import (bn_xkey, key_size, 1, 1, 0, 0, xkey);
      mpz_import (bn_x_n, SHA1_DIGEST_SIZE, 1, 1, 0, 0, sha_buf);
      mpz_add_ui(bn_xkey,bn_xkey, 1);            // xkey += 1
      mpz_add(bn_xkey, bn_xkey, bn_x_n);       // xkey += x
      mpz_mod(bn_xkey, bn_xkey, c_2powb);  // xkey = xkey mod (2^b)

      memset(xkey, 0, xkey_len);
      ui32_t bn_buf_len = bn_xkey->_mp_size;
      ui32_t idx = ( bn_buf_len < key_size ) ? key_size - bn_buf_len : 0;
      mpz_export (xkey+idx, NULL, 1, 1, 0, 0, bn_xkey);

    }
}

//
// end KM_prng.cpp
//

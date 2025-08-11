#include <stdint.h>
#include <string.h>
#include <secp256k1.h>
#include "SECP256k1.h"
#include "Int.h"

static secp256k1_context* g_ctx = nullptr;

static void ensure_ctx() {
  if (!g_ctx) {
    g_ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  }
}

extern "C" bool secp_bridge_compute_pubkey(const Int &k, Point &out) {
  ensure_ctx();
  unsigned char seckey[32];
  // Int::GetByte is non-const; use a local copy
  Int tmp = k;
  for (int i = 0; i < 32; i++) seckey[i] = tmp.GetByte(i);
  if (!secp256k1_ec_seckey_verify(g_ctx, seckey)) return false;

  secp256k1_pubkey pub;
  if (!secp256k1_ec_pubkey_create(g_ctx, &pub, seckey)) return false;

  unsigned char out65[65]; size_t outlen = sizeof(out65);
  if (!secp256k1_ec_pubkey_serialize(g_ctx, out65, &outlen, &pub, SECP256K1_EC_UNCOMPRESSED)) return false;
  // out65: 0x04 | X[32] | Y[32]
  if (outlen != 65 || out65[0] != 0x04) return false;

  Int x, y;
  for (int i = 0; i < 32; i++) x.SetByte(31 - i, out65[1 + i]);
  for (int i = 0; i < 32; i++) y.SetByte(31 - i, out65[33 + i]);

  out.x = x;
  out.y = y;
  out.z.SetInt32(1);
  return true;
}



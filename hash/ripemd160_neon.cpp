// NEON 4-way RIPEMD-160 compression for single 64-byte block (SHA-256 digest 32B + padding)
#include <arm_neon.h>
#include <stdint.h>
#include <string.h>
#include "ripemd160.h"

// rotate-left by immediate constant n (0 < n < 32)
#define ROLV_IMM(x, n) \
  vorrq_u32(vshlq_n_u32((x), (n)), vshrq_n_u32((x), 32 - (n)))

static inline uint32x4_t F1(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
  return veorq_u32(veorq_u32(x, y), z);
}
static inline uint32x4_t F2(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
  return vorrq_u32(vandq_u32(x, y), vandq_u32(vmvnq_u32(x), z));
}
static inline uint32x4_t F3(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
  return veorq_u32(vorrq_u32(x, vmvnq_u32(y)), z);
}
static inline uint32x4_t F4(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
  return vorrq_u32(vandq_u32(x, z), vandq_u32(vmvnq_u32(z), y));
}
static inline uint32x4_t F5(uint32x4_t x, uint32x4_t y, uint32x4_t z) {
  return veorq_u32(x, vorrq_u32(y, vmvnq_u32(z)));
}

#define ROUND_STEP(a,b,c,d,e, f, x, k, r) do { \
  a = vaddq_u32(vaddq_u32(vaddq_u32(a, (f)), (x)), (k)); \
  a = ROLV_IMM(a, (r)); \
  a = vaddq_u32(a, (e)); \
  (c) = ROLV_IMM((c), 10); \
} while(0)

#define R11(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F1(b,c,d), x, vdupq_n_u32(0u), r)
#define R21(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F2(b,c,d), x, vdupq_n_u32(0x5A827999u), r)
#define R31(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F3(b,c,d), x, vdupq_n_u32(0x6ED9EBA1u), r)
#define R41(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F4(b,c,d), x, vdupq_n_u32(0x8F1BBCDCu), r)
#define R51(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F5(b,c,d), x, vdupq_n_u32(0xA953FD4Eu), r)
#define R12(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F5(b,c,d), x, vdupq_n_u32(0x50A28BE6u), r)
#define R22(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F4(b,c,d), x, vdupq_n_u32(0x5C4DD124u), r)
#define R32(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F3(b,c,d), x, vdupq_n_u32(0x6D703EF3u), r)
#define R42(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F2(b,c,d), x, vdupq_n_u32(0x7A6D76E9u), r)
#define R52(a,b,c,d,e,x,r) ROUND_STEP(a,b,c,d,e, F1(b,c,d), x, vdupq_n_u32(0u), r)

static inline uint32x4_t pack_u32x4(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  uint32x4_t v = {a, b, c, d};
  return v;
}

// 4-way single-block RIPEMD-160. Inputs are 32 bytes each, padding is implicit.
void ripemd160_4way_neon(const unsigned char *i0, const unsigned char *i1, const unsigned char *i2, const unsigned char *i3,
                         unsigned char *d0, unsigned char *d1, unsigned char *d2, unsigned char *d3) {

  // Prepare message schedule w[0..15] in vectors (lanes: 0=i0,1=i1,2=i2,3=i3)
  const uint32_t *w0 = (const uint32_t *)i0;
  const uint32_t *w1 = (const uint32_t *)i1;
  const uint32_t *w2 = (const uint32_t *)i2;
  const uint32_t *w3 = (const uint32_t *)i3;

  uint32x4_t w[16];
  // First 8 words from the 32-byte message
  for (int t = 0; t < 8; t++) {
    w[t] = pack_u32x4(w0[t], w1[t], w2[t], w3[t]);
  }
  // Padding: 0x80 byte at position 32 -> word index 8, low byte
  w[8]  = vdupq_n_u32(0x00000080u);
  w[9]  = vdupq_n_u32(0);
  w[10] = vdupq_n_u32(0);
  w[11] = vdupq_n_u32(0);
  w[12] = vdupq_n_u32(0);
  w[13] = vdupq_n_u32(0);
  // Message length in bits (32*8 = 256) at word 14; word 15 is 0
  w[14] = vdupq_n_u32(32u << 3);
  w[15] = vdupq_n_u32(0);

  // Initialize state vectors
  uint32x4_t A1 = vdupq_n_u32(0x67452301u), B1 = vdupq_n_u32(0xEFCDAB89u), C1 = vdupq_n_u32(0x98BADCFEu), D1 = vdupq_n_u32(0x10325476u), E1 = vdupq_n_u32(0xC3D2E1F0u);
  uint32x4_t A2 = A1, B2 = B1, C2 = C1, D2 = D1, E2 = E1;

  // Round 1/1'
  R11(A1, B1, C1, D1, E1, w[0], 11);  R12(A2, B2, C2, D2, E2, w[5], 8);
  R11(E1, A1, B1, C1, D1, w[1], 14);  R12(E2, A2, B2, C2, D2, w[14], 9);
  R11(D1, E1, A1, B1, C1, w[2], 15);  R12(D2, E2, A2, B2, C2, w[7], 9);
  R11(C1, D1, E1, A1, B1, w[3], 12);  R12(C2, D2, E2, A2, B2, w[0], 11);
  R11(B1, C1, D1, E1, A1, w[4], 5);   R12(B2, C2, D2, E2, A2, w[9], 13);
  R11(A1, B1, C1, D1, E1, w[5], 8);   R12(A2, B2, C2, D2, E2, w[2], 15);
  R11(E1, A1, B1, C1, D1, w[6], 7);   R12(E2, A2, B2, C2, D2, w[11], 15);
  R11(D1, E1, A1, B1, C1, w[7], 9);   R12(D2, E2, A2, B2, C2, w[4], 5);
  R11(C1, D1, E1, A1, B1, w[8], 11);  R12(C2, D2, E2, A2, B2, w[13], 7);
  R11(B1, C1, D1, E1, A1, w[9], 13);  R12(B2, C2, D2, E2, A2, w[6], 7);
  R11(A1, B1, C1, D1, E1, w[10], 14); R12(A2, B2, C2, D2, E2, w[15], 8);
  R11(E1, A1, B1, C1, D1, w[11], 15); R12(E2, A2, B2, C2, D2, w[8], 11);
  R11(D1, E1, A1, B1, C1, w[12], 6);  R12(D2, E2, A2, B2, C2, w[1], 14);
  R11(C1, D1, E1, A1, B1, w[13], 7);  R12(C2, D2, E2, A2, B2, w[10], 14);
  R11(B1, C1, D1, E1, A1, w[14], 9);  R12(B2, C2, D2, E2, A2, w[3], 12);
  R11(A1, B1, C1, D1, E1, w[15], 8);  R12(A2, B2, C2, D2, E2, w[12], 6);

  // Round 2/2'
  R21(E1, A1, B1, C1, D1, w[7], 7);   R22(E2, A2, B2, C2, D2, w[6], 9);
  R21(D1, E1, A1, B1, C1, w[4], 6);   R22(D2, E2, A2, B2, C2, w[11], 13);
  R21(C1, D1, E1, A1, B1, w[13], 8);  R22(C2, D2, E2, A2, B2, w[3], 15);
  R21(B1, C1, D1, E1, A1, w[1], 13);  R22(B2, C2, D2, E2, A2, w[7], 7);
  R21(A1, B1, C1, D1, E1, w[10], 11); R22(A2, B2, C2, D2, E2, w[0], 12);
  R21(E1, A1, B1, C1, D1, w[6], 9);   R22(E2, A2, B2, C2, D2, w[13], 8);
  R21(D1, E1, A1, B1, C1, w[15], 7);  R22(D2, E2, A2, B2, C2, w[5], 9);
  R21(C1, D1, E1, A1, B1, w[3], 15);  R22(C2, D2, E2, A2, B2, w[10], 11);
  R21(B1, C1, D1, E1, A1, w[12], 7);  R22(B2, C2, D2, E2, A2, w[14], 7);
  R21(A1, B1, C1, D1, E1, w[0], 12);  R22(A2, B2, C2, D2, E2, w[15], 7);
  R21(E1, A1, B1, C1, D1, w[9], 15);  R22(E2, A2, B2, C2, D2, w[8], 12);
  R21(D1, E1, A1, B1, C1, w[5], 9);   R22(D2, E2, A2, B2, C2, w[12], 7);
  R21(C1, D1, E1, A1, B1, w[2], 11);  R22(C2, D2, E2, A2, B2, w[4], 6);
  R21(B1, C1, D1, E1, A1, w[14], 7);  R22(B2, C2, D2, E2, A2, w[9], 15);
  R21(A1, B1, C1, D1, E1, w[11], 13); R22(A2, B2, C2, D2, E2, w[1], 13);
  R21(E1, A1, B1, C1, D1, w[8], 12);  R22(E2, A2, B2, C2, D2, w[2], 11);

  // Round 3/3'
  R31(D1, E1, A1, B1, C1, w[3], 11);  R32(D2, E2, A2, B2, C2, w[15], 9);
  R31(C1, D1, E1, A1, B1, w[10], 13); R32(C2, D2, E2, A2, B2, w[5], 7);
  R31(B1, C1, D1, E1, A1, w[14], 6);  R32(B2, C2, D2, E2, A2, w[1], 15);
  R31(A1, B1, C1, D1, E1, w[4], 7);   R32(A2, B2, C2, D2, E2, w[3], 11);
  R31(E1, A1, B1, C1, D1, w[9], 14);  R32(E2, A2, B2, C2, D2, w[7], 8);
  R31(D1, E1, A1, B1, C1, w[15], 9);  R32(D2, E2, A2, B2, C2, w[14], 6);
  R31(C1, D1, E1, A1, B1, w[8], 13);  R32(C2, D2, E2, A2, B2, w[6], 6);
  R31(B1, C1, D1, E1, A1, w[1], 15);  R32(B2, C2, D2, E2, A2, w[9], 14);
  R31(A1, B1, C1, D1, E1, w[2], 14);  R32(A2, B2, C2, D2, E2, w[11], 12);
  R31(E1, A1, B1, C1, D1, w[7], 8);   R32(E2, A2, B2, C2, D2, w[8], 13);
  R31(D1, E1, A1, B1, C1, w[0], 13);  R32(D2, E2, A2, B2, C2, w[12], 5);
  R31(C1, D1, E1, A1, B1, w[6], 6);   R32(C2, D2, E2, A2, B2, w[2], 14);
  R31(B1, C1, D1, E1, A1, w[13], 5);  R32(B2, C2, D2, E2, A2, w[10], 13);
  R31(A1, B1, C1, D1, E1, w[11], 12); R32(A2, B2, C2, D2, E2, w[0], 13);
  R31(E1, A1, B1, C1, D1, w[5], 7);   R32(E2, A2, B2, C2, D2, w[4], 7);
  R31(D1, E1, A1, B1, C1, w[12], 5);  R32(D2, E2, A2, B2, C2, w[13], 5);

  // Round 4/4'
  R41(C1, D1, E1, A1, B1, w[1], 11);  R42(C2, D2, E2, A2, B2, w[8], 15);
  R41(B1, C1, D1, E1, A1, w[9], 12);  R42(B2, C2, D2, E2, A2, w[6], 5);
  R41(A1, B1, C1, D1, E1, w[11], 14); R42(A2, B2, C2, D2, E2, w[4], 8);
  R41(E1, A1, B1, C1, D1, w[10], 15); R42(E2, A2, B2, C2, D2, w[1], 11);
  R41(D1, E1, A1, B1, C1, w[0], 14);  R42(D2, E2, A2, B2, C2, w[3], 14);
  R41(C1, D1, E1, A1, B1, w[8], 15);  R42(C2, D2, E2, A2, B2, w[11], 14);
  R41(B1, C1, D1, E1, A1, w[12], 9);  R42(B2, C2, D2, E2, A2, w[15], 6);
  R41(A1, B1, C1, D1, E1, w[4], 8);   R42(A2, B2, C2, D2, E2, w[0], 14);
  R41(E1, A1, B1, C1, D1, w[13], 9);  R42(E2, A2, B2, C2, D2, w[5], 6);
  R41(D1, E1, A1, B1, C1, w[3], 14);  R42(D2, E2, A2, B2, C2, w[12], 9);
  R41(C1, D1, E1, A1, B1, w[7], 5);   R42(C2, D2, E2, A2, B2, w[2], 12);
  R41(B1, C1, D1, E1, A1, w[15], 6);  R42(B2, C2, D2, E2, A2, w[13], 9);
  R41(A1, B1, C1, D1, E1, w[14], 8);  R42(A2, B2, C2, D2, E2, w[9], 12);
  R41(E1, A1, B1, C1, D1, w[5], 6);   R42(E2, A2, B2, C2, D2, w[7], 5);
  R41(D1, E1, A1, B1, C1, w[6], 5);   R42(D2, E2, A2, B2, C2, w[10], 15);
  R41(C1, D1, E1, A1, B1, w[2], 12);  R42(C2, D2, E2, A2, B2, w[14], 8);

  // Round 5/5'
  R51(B1, C1, D1, E1, A1, w[4], 9);   R52(B2, C2, D2, E2, A2, w[12], 8);
  R51(A1, B1, C1, D1, E1, w[0], 15);  R52(A2, B2, C2, D2, E2, w[15], 5);
  R51(E1, A1, B1, C1, D1, w[5], 5);   R52(E2, A2, B2, C2, D2, w[10], 12);
  R51(D1, E1, A1, B1, C1, w[9], 11);  R52(D2, E2, A2, B2, C2, w[4], 9);
  R51(C1, D1, E1, A1, B1, w[7], 6);   R52(C2, D2, E2, A2, B2, w[1], 12);
  R51(B1, C1, D1, E1, A1, w[12], 8);  R52(B2, C2, D2, E2, A2, w[5], 5);
  R51(A1, B1, C1, D1, E1, w[2], 13);  R52(A2, B2, C2, D2, E2, w[8], 14);
  R51(E1, A1, B1, C1, D1, w[10], 12); R52(E2, A2, B2, C2, D2, w[7], 6);
  R51(D1, E1, A1, B1, C1, w[14], 5);  R52(D2, E2, A2, B2, C2, w[6], 8);
  R51(C1, D1, E1, A1, B1, w[1], 12);  R52(C2, D2, E2, A2, B2, w[2], 13);
  R51(B1, C1, D1, E1, A1, w[3], 13);  R52(B2, C2, D2, E2, A2, w[13], 6);
  R51(A1, B1, C1, D1, E1, w[8], 14);  R52(A2, B2, C2, D2, E2, w[14], 5);
  R51(E1, A1, B1, C1, D1, w[11], 11); R52(E2, A2, B2, C2, D2, w[0], 15);
  R51(D1, E1, A1, B1, C1, w[6], 8);   R52(D2, E2, A2, B2, C2, w[3], 13);
  R51(C1, D1, E1, A1, B1, w[15], 5);  R52(C2, D2, E2, A2, B2, w[9], 11);

  // Combine
  uint32x4_t s0 = vdupq_n_u32(0x67452301u);
  uint32x4_t s1 = vdupq_n_u32(0xEFCDAB89u);
  uint32x4_t s2 = vdupq_n_u32(0x98BADCFEu);
  uint32x4_t s3 = vdupq_n_u32(0x10325476u);
  uint32x4_t s4 = vdupq_n_u32(0xC3D2E1F0u);

  uint32x4_t t = s0;
  s0 = vaddq_u32(vaddq_u32(s1, C1), D2);
  s1 = vaddq_u32(vaddq_u32(s2, D1), E2);
  s2 = vaddq_u32(vaddq_u32(s3, E1), A2);
  s3 = vaddq_u32(vaddq_u32(s4, A1), B2);
  s4 = vaddq_u32(vaddq_u32(t,  B1), C2);

  // Store results lane-wise
  uint32_t out0[5], out1[5], out2[5], out3[5];
  out0[0] = vgetq_lane_u32(s0, 0); out1[0] = vgetq_lane_u32(s0, 1); out2[0] = vgetq_lane_u32(s0, 2); out3[0] = vgetq_lane_u32(s0, 3);
  out0[1] = vgetq_lane_u32(s1, 0); out1[1] = vgetq_lane_u32(s1, 1); out2[1] = vgetq_lane_u32(s1, 2); out3[1] = vgetq_lane_u32(s1, 3);
  out0[2] = vgetq_lane_u32(s2, 0); out1[2] = vgetq_lane_u32(s2, 1); out2[2] = vgetq_lane_u32(s2, 2); out3[2] = vgetq_lane_u32(s2, 3);
  out0[3] = vgetq_lane_u32(s3, 0); out1[3] = vgetq_lane_u32(s3, 1); out2[3] = vgetq_lane_u32(s3, 2); out3[3] = vgetq_lane_u32(s3, 3);
  out0[4] = vgetq_lane_u32(s4, 0); out1[4] = vgetq_lane_u32(s4, 1); out2[4] = vgetq_lane_u32(s4, 2); out3[4] = vgetq_lane_u32(s4, 3);

  memcpy(d0, out0, 20);
  memcpy(d1, out1, 20);
  memcpy(d2, out2, 20);
  memcpy(d3, out3, 20);
}



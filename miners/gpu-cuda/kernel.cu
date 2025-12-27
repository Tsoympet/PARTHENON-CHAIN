#include <cuda.h>
#include <stdint.h>

// CUDA SHA-256d mining kernel tuned for higher throughput while
// keeping deterministic behavior. Tightened unrolling and target
// comparison improve effective hash rate on modern GPUs. Shared-memory
// caching of the static header improves occupancy on both NVIDIA and
// ROCm-compatible devices running CUDA.

constexpr int THREADS_PER_BLOCK = 256;

__device__ __forceinline__ uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}
__device__ __forceinline__ uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}
__device__ __forceinline__ uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}
__device__ __forceinline__ uint32_t big0(uint32_t x) { return rotr(x,2) ^ rotr(x,13) ^ rotr(x,22); }
__device__ __forceinline__ uint32_t big1(uint32_t x) { return rotr(x,6) ^ rotr(x,11) ^ rotr(x,25); }
__device__ __forceinline__ uint32_t sm0(uint32_t x) { return rotr(x,7) ^ rotr(x,18) ^ (x >> 3); }
__device__ __forceinline__ uint32_t sm1(uint32_t x) { return rotr(x,17) ^ rotr(x,19) ^ (x >> 10); }

__constant__ uint32_t K[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

__device__ __forceinline__ bool meets_target(const uint32_t* state, const uint32_t* target)
{
    // Compare full 256-bit value high-to-low
    for (int i = 7; i >= 0; --i) {
        uint32_t hv = state[i];
        uint32_t tv = __ldg(&target[i]);
        if (hv < tv) return true;
        if (hv > tv) return false;
    }
    return true;
}

__device__ void sha256_round(uint32_t state[8], const uint32_t* w)
{
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
    for (int i=0;i<64;++i) {
        uint32_t t1 = h + big1(e) + ch(e,f,g) + K[i] + w[i];
        uint32_t t2 = big0(a) + maj(a,b,c);
        h=g; g=f; f=e; e=d + t1; d=c; c=b; b=a; a=t1 + t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

extern "C" __global__ __launch_bounds__(THREADS_PER_BLOCK, 3) void sha256d_kernel(const uint8_t* __restrict__ header76,
                                                                    uint32_t nonceStart,
                                                                    const uint32_t* __restrict__ target,
                                                                    uint32_t* __restrict__ solution,
                                                                    int* found)
{
    uint32_t gid = blockIdx.x * blockDim.x + threadIdx.x;
    if (__ldg(found)) return;

    __shared__ uint8_t shHeader[76];
    if (threadIdx.x < 76)
        shHeader[threadIdx.x] = __ldg(&header76[threadIdx.x]);
    __syncthreads();

    uint8_t header[80];
    #pragma unroll
    for (int i=0;i<76;++i) header[i] = shHeader[i];
    uint32_t nonce = nonceStart + gid;
    header[76] = (nonce >> 24) & 0xff;
    header[77] = (nonce >> 16) & 0xff;
    header[78] = (nonce >> 8) & 0xff;
    header[79] = nonce & 0xff;

    uint32_t w0[64];
    #pragma unroll
    for (int i=0;i<16;++i)
        w0[i] = ((uint32_t)header[i*4] << 24) | ((uint32_t)header[i*4+1] << 16) | ((uint32_t)header[i*4+2] << 8) | ((uint32_t)header[i*4+3]);
    #pragma unroll 48
    for (int i=16;i<64;++i)
        w0[i] = sm1(w0[i-2]) + w0[i-7] + sm0(w0[i-15]) + w0[i-16];

    uint32_t state[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    sha256_round(state, w0);

    uint32_t w1[64];
    w1[0] = 0x80000000;
    #pragma unroll
    for (int i=1;i<64;++i) w1[i] = 0;
    w1[15] = 80 * 8;
    #pragma unroll 48
    for (int i=16;i<64;++i)
        w1[i] = sm1(w1[i-2]) + w1[i-7] + sm0(w1[i-15]) + w1[i-16];
    sha256_round(state, w1);

    uint32_t w2[64];
    for (int i=0;i<8;++i) w2[i] = state[i];
    w2[8] = 0x80000000;
    #pragma unroll
    for (int i=9;i<64;++i) w2[i] = 0;
    w2[15] = 32 * 8;
    #pragma unroll 48
    for (int i=16;i<64;++i)
        w2[i] = sm1(w2[i-2]) + w2[i-7] + sm0(w2[i-15]) + w2[i-16];

    uint32_t state2[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    sha256_round(state2, w2);

    uint32_t w3[64];
    #pragma unroll
    for (int i=0;i<64;++i) w3[i] = 0;
    w3[8] = 0x80000000;
    w3[15] = 32 * 8;
    #pragma unroll 48
    for (int i=16;i<64;++i)
        w3[i] = sm1(w3[i-2]) + w3[i-7] + sm0(w3[i-15]) + w3[i-16];
    sha256_round(state2, w3);

    if (meets_target(state2, target) && atomicCAS(found, 0, 1) == 0) {
        solution[0] = nonce;
        for (int i=0;i<8;++i)
            solution[i+1] = state2[i];
    }
}

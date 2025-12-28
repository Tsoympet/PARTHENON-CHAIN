// OpenCL SHA-256d kernel for mining
// Mirrors the CUDA implementation but uses portable OpenCL 1.2 constructs
// so it can run on AMD, Intel, and other devices.

__constant uint K[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

inline uint rotr(uint x, uint n) { return (x >> n) | (x << (32 - n)); }
inline uint ch(uint x, uint y, uint z) { return (x & y) ^ (~x & z); }
inline uint maj(uint x, uint y, uint z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint big0(uint x) { return rotr(x,2) ^ rotr(x,13) ^ rotr(x,22); }
inline uint big1(uint x) { return rotr(x,6) ^ rotr(x,11) ^ rotr(x,25); }
inline uint sm0(uint x)  { return rotr(x,7) ^ rotr(x,18) ^ (x >> 3); }
inline uint sm1(uint x)  { return rotr(x,17) ^ rotr(x,19) ^ (x >> 10); }

inline bool meets_target(const uint* state, const uint* target)
{
    for (int i = 7; i >= 0; --i) {
        uint hv = state[i];
        uint tv = target[i];
        if (hv < tv) return true;
        if (hv > tv) return false;
    }
    return true;
}

inline void sha256_round(uint state[8], const uint* w)
{
    uint a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
    for (int i = 0; i < 64; ++i) {
        uint t1 = h + big1(e) + ch(e,f,g) + K[i] + w[i];
        uint t2 = big0(a) + maj(a,b,c);
        h=g; g=f; f=e; e=d + t1; d=c; c=b; b=a; a=t1 + t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

__kernel void sha256d_kernel(__global const uchar* header76,
                             const uint nonceStart,
                             __global const uint* target,
                             __global uint* solution,
                             __global int* found)
{
    uint gid = get_global_id(0);
    if (atomic_or((volatile __global int*)found, 0) != 0) return;

    __local uchar shHeader[76];
    if (get_local_id(0) < 76) shHeader[get_local_id(0)] = header76[get_local_id(0)];
    barrier(CLK_LOCAL_MEM_FENCE);

    uchar header[80];
    for (int i = 0; i < 76; ++i) header[i] = shHeader[i];
    uint nonce = nonceStart + gid;
    header[76] = (nonce >> 24) & 0xff;
    header[77] = (nonce >> 16) & 0xff;
    header[78] = (nonce >> 8) & 0xff;
    header[79] = nonce & 0xff;

    uint w0[64];
    for (int i = 0; i < 16; ++i) {
        int o = i * 4;
        w0[i] = ((uint)header[o] << 24) | ((uint)header[o+1] << 16) | ((uint)header[o+2] << 8) | (uint)header[o+3];
    }
    for (int i = 16; i < 64; ++i) w0[i] = sm1(w0[i-2]) + w0[i-7] + sm0(w0[i-15]) + w0[i-16];

    uint state[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    sha256_round(state, w0);

    uint w1[64];
    w1[0] = 0x80000000; for (int i = 1; i < 64; ++i) w1[i] = 0; w1[15] = 80 * 8;
    for (int i = 16; i < 64; ++i) w1[i] = sm1(w1[i-2]) + w1[i-7] + sm0(w1[i-15]) + w1[i-16];
    sha256_round(state, w1);

    uint w2[64];
    for (int i = 0; i < 8; ++i) w2[i] = state[i];
    w2[8] = 0x80000000; for (int i = 9; i < 64; ++i) w2[i] = 0; w2[15] = 32 * 8;
    for (int i = 16; i < 64; ++i) w2[i] = sm1(w2[i-2]) + w2[i-7] + sm0(w2[i-15]) + w2[i-16];

    uint state2[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    sha256_round(state2, w2);

    uint w3[64];
    for (int i = 0; i < 64; ++i) w3[i] = 0;
    w3[8] = 0x80000000; w3[15] = 32 * 8;
    for (int i = 16; i < 64; ++i) w3[i] = sm1(w3[i-2]) + w3[i-7] + sm0(w3[i-15]) + w3[i-16];
    sha256_round(state2, w3);

    if (meets_target(state2, target)) {
        if (atomic_cmpxchg((volatile __global int*)found, 0, 1) == 0) {
            solution[0] = nonce;
            for (int i = 0; i < 8; ++i) solution[i+1] = state2[i];
        }
    }
}

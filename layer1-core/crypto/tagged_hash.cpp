#include "tagged_hash.h"
#include <cstring>
#include <vector>

uint256 TaggedHash(
    const std::string& tag,
    const uint8_t* data,
    size_t len)
{
    auto tagHash = SHA256(reinterpret_cast<const uint8_t*>(tag.data()), tag.size());

    // Build the preimage: tagHash || tagHash || data
    std::vector<uint8_t> preimage;
    preimage.reserve(tagHash.size() * 2 + len);
    preimage.insert(preimage.end(), tagHash.begin(), tagHash.end());
    preimage.insert(preimage.end(), tagHash.begin(), tagHash.end());
    preimage.insert(preimage.end(), data, data + len);

    return SHA256(preimage.data(), preimage.size());
}

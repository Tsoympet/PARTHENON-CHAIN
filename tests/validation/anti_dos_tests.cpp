#include "../../layer1-core/validation/anti_dos.h"
#include <cassert>
#include <thread>
#include <chrono>

int main()
{
    ValidationRateLimiter limiter(/*maxTokensPerMinute=*/60, /*burst=*/2);
    assert(limiter.Consume());
    assert(limiter.Consume());
    // Third immediate consume should fail due to burst cap.
    assert(!limiter.Consume());
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    assert(limiter.Consume());

    OrphanBuffer buffer(2);
    OrphanBlock a{}; a.hash.fill(0x01); a.parent.fill(0xAA);
    OrphanBlock b{}; b.hash.fill(0x02); b.parent.fill(0xBB);
    OrphanBlock c{}; c.hash.fill(0x03); c.parent.fill(0xAA);
    assert(!buffer.Add(a));
    assert(!buffer.Add(b));
    auto evicted = buffer.Add(c);
    assert(evicted.has_value());
    auto children = buffer.PopChildren(a.parent);
    assert(children.size() == 1);
    return 0;
}


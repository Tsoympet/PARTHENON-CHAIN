#pragma once

#include <cstdint>
#include <string>

#include "../runtime/types.h"

namespace sidechain::wasm {

struct GasSchedule {
    uint64_t nop{0};
    uint64_t const_i32{2};
    uint64_t add_i32{3};
    uint64_t load{4};
    uint64_t store{6};
    uint64_t return_top{1};
    uint64_t memory_byte{1};
};

class GasMeter {
public:
    GasMeter(uint64_t limit, GasSchedule schedule);

    bool Consume(OpCode op);
    bool ConsumeMemory(uint64_t bytes);

    uint64_t used() const { return used_; }
    uint64_t limit() const { return limit_; }
    const std::string& last_error() const { return error_; }

private:
    bool Add(uint64_t cost);

    uint64_t limit_;
    uint64_t used_{0};
    GasSchedule schedule_;
    std::string error_;
};

GasSchedule DefaultGasSchedule();

}  // namespace sidechain::wasm

#include "model.h"

#include <limits>

namespace sidechain::wasm {

namespace {
uint64_t CostForOp(const GasSchedule& schedule, OpCode op) {
    switch (op) {
        case OpCode::Nop:
            return schedule.nop;
        case OpCode::ConstI32:
            return schedule.const_i32;
        case OpCode::AddI32:
            return schedule.add_i32;
        case OpCode::Load:
            return schedule.load;
        case OpCode::Store:
            return schedule.store;
        case OpCode::ReturnTop:
            return schedule.return_top;
        default:
            return 0;
    }
}
}  // namespace

GasMeter::GasMeter(uint64_t limit, GasSchedule schedule)
    : limit_(limit), schedule_(schedule) {}

bool GasMeter::Add(uint64_t cost) {
    if (used_ >= limit_ || cost > limit_ - used_) {
        used_ = limit_;
        error_ = "out of gas";
        return false;
    }
    used_ += cost;
    return true;
}

bool GasMeter::Consume(OpCode op) {
    return Add(CostForOp(schedule_, op));
}

bool GasMeter::ConsumeMemory(uint64_t bytes) {
    if (schedule_.memory_byte != 0 &&
        bytes > std::numeric_limits<uint64_t>::max() / schedule_.memory_byte) {
        error_ = "out of gas";
        used_ = limit_;
        return false;
    }
    return Add(bytes * schedule_.memory_byte);
}

GasSchedule DefaultGasSchedule() {
    return GasSchedule{};
}

}  // namespace sidechain::wasm

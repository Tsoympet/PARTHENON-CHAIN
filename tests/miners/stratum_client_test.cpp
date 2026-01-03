#include <gtest/gtest.h>

#include "miners/stratum.h"

TEST(StratumClient, RejectsRemoteWithoutAllowFlag) {
    EXPECT_THROW(StratumClient("stratum+tcp://example.com:3333", "user", "pass", false),
                 std::runtime_error);
}

TEST(StratumClient, LocalhostAllowedWithoutNetworkDial) {
    StratumClient client("stratum+tcp://127.0.0.1:3333", "user", "pass", false);
    EXPECT_DOUBLE_EQ(1.0, client.CurrentDifficulty());
}

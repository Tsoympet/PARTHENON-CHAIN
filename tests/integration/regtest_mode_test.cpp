#include <gtest/gtest.h>
#include <cstdlib>

TEST(Integration, RegtestHarness)
{
    if (std::getenv("DRACHMA_RUN_DOCKER_TESTS") == nullptr) {
        GTEST_SKIP() << "Set DRACHMA_RUN_DOCKER_TESTS=1 to exercise dockerized regtest harness.";
    }

    // Bring up the compose stack and ensure containers start successfully.
    int up = std::system("docker compose up -d --build");
    EXPECT_EQ(up, 0);
    int down = std::system("docker compose down");
    EXPECT_EQ(down, 0);
}

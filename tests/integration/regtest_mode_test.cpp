#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

TEST(Integration, RegtestHarness)
{
    const bool runDocker = std::getenv("DRACHMA_RUN_DOCKER_TESTS") != nullptr;

    if (runDocker) {
        // Bring up the compose stack and ensure containers start successfully.
        int up = std::system("docker compose up -d --build");
        EXPECT_EQ(up, 0);
        int down = std::system("docker compose down");
        EXPECT_EQ(down, 0);
    } else {
        // Fallback: validate the compose file exists and has the expected services stanza
        // so the harness can be invoked in constrained environments without Docker.
        namespace fs = std::filesystem;
        fs::path repoRoot = fs::path(__FILE__).parent_path().parent_path().parent_path();
        fs::path composePath = repoRoot / "docker-compose.yml";
        std::ifstream compose(composePath);
        ASSERT_TRUE(compose.good()) << "docker-compose.yml missing at " << composePath;

        std::string line;
        bool hasServices = false;
        while (std::getline(compose, line)) {
            if (line.find("services:") != std::string::npos) {
                hasServices = true;
                break;
            }
        }
        EXPECT_TRUE(hasServices) << "docker-compose.yml should declare services";
    }
}

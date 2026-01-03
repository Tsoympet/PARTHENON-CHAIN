#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "layer3-app/qt/nft_ui_helpers.h"

using nft::ui::HeaderLabels;
using nft::ui::ResolveIconPath;

namespace {
std::string repo_assets()
{
    namespace fs = std::filesystem;
    const fs::path root = fs::path(__FILE__).parent_path().parent_path().parent_path();
    return (root / "assets").string();
}
}  // namespace

TEST(NftUiHelpers, HeadersExcludeTln)
{
    const auto headers = HeaderLabels();
    for (const auto& h : headers) {
        EXPECT_EQ(h.find("TLN"), std::string::npos);
    }
}

TEST(NftUiHelpers, ResolvesCategoryIcon)
{
    const auto base = repo_assets();
    const auto icon = ResolveIconPath(base, "mythology");
    EXPECT_NE(icon.find("nft-mythology.svg"), std::string::npos);
    EXPECT_TRUE(std::filesystem::exists(icon));
}

TEST(NftUiHelpers, FallsBackWhenIconMissing)
{
    namespace fs = std::filesystem;
    const fs::path temp_dir = fs::temp_directory_path() / "nft_icon_fallback";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "nft-icons");
    const fs::path fallback = temp_dir / "nft-icons" / "nft-default.svg";
    std::ofstream(fallback.string()) << "<svg></svg>";

    const auto resolved = ResolveIconPath(temp_dir.string(), "unknown-category");
    EXPECT_EQ(fs::canonical(fallback), fs::canonical(resolved));
}

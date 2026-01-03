#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

namespace nft::ui {

inline std::string NormalizeCategory(std::string category)
{
    for (auto& c : category) {
        if (c == ' ' || c == '_') c = '-';
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return category;
}

inline std::string ResolveIconPath(const std::string& assets_root, const std::string& canon_category)
{
    namespace fs = std::filesystem;
    const fs::path base = fs::path(assets_root) / "nft-icons";
    const fs::path fallback = base / "nft-default.svg";
    const fs::path candidate = base / ("nft-" + NormalizeCategory(canon_category) + ".svg");
    if (fs::exists(candidate)) return candidate.string();
    return fallback.string();
}

inline std::vector<std::string> HeaderLabels()
{
    return {"Title", "Canon category", "Creator", "Royalty (bps)", "Owner", "Bids", "Last sale"};
}

}  // namespace nft::ui

#include "Internal.hpp"

using namespace GView::Utils;
using namespace AppCUI::Graphics;

struct ZonesListContext {
    std::vector<Zone> zones{};
    std::vector<Zone> cache{};
};

ZonesList::ZonesList()
{
    context = new ZonesListContext;
}

ZonesList::~ZonesList()
{
    if (context != nullptr) {
        delete reinterpret_cast<ZonesListContext*>(context);
    }
}

bool ZonesList::Add(uint64 s, uint64 e, ColorPair c, std::string_view txt)
{
    CHECK(context != nullptr, false, "");
    auto ctx = reinterpret_cast<ZonesListContext*>(this->context);
    ctx->zones.emplace_back(s, e, c, txt);
    return true;
}

const std::optional<Zone> ZonesList::OffsetToZone(uint64 position) const
{
    CHECK(context != nullptr, std::nullopt, "");
    auto ctx = reinterpret_cast<ZonesListContext*>(this->context);

    for (const auto& zone : ctx->cache) {
        if (zone.interval.low <= position && position <= zone.interval.high) {
            return zone;
        }
    }

    return std::nullopt;
}

bool ZonesList::SetCache(const Zone::Interval& interval)
{
    CHECK(context != nullptr, false, "");
    auto ctx = reinterpret_cast<ZonesListContext*>(this->context);

    ctx->cache.clear();

    for (const auto& zone : ctx->zones) {
        if ((zone.interval.low >= interval.low && zone.interval.low <= interval.high) ||
            interval.low >= zone.interval.low && interval.low <= zone.interval.high) {
            ctx->cache.push_back(zone);
        }
    }

    std::sort(ctx->cache.begin(), ctx->cache.end(), [](const Zone& a, const Zone& b) {
        if (a.interval.low == b.interval.low) {
            return a.interval.high < b.interval.high;
        }
        return a.interval.low > b.interval.low;
    });

    return true;
}

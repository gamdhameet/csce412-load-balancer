/** @file IpFilter.h @brief IPv4 CIDR admission filter. */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace codeharbor {

/** Rejects source addresses in configured IPv4 CIDR ranges. */
class IpFilter {
public:
    /** Add one CIDR range; throws std::invalid_argument for malformed input. */
    void add_range(const std::string& cidr);

    /** Return true if the source address falls in any blocked range. */
    bool blocks(const std::string& ip) const;

    /** Number of configured ranges. */
    std::size_t range_count() const { return ranges_.size(); }

private:
    struct Range { std::uint32_t network; std::uint32_t mask; };
    std::vector<Range> ranges_;
};

/** Parse dotted-decimal IPv4; throws std::invalid_argument on invalid input. */
std::uint32_t parse_ipv4(const std::string& text);

} // namespace codeharbor

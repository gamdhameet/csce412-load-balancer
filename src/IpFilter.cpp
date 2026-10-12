/** @file IpFilter.cpp @brief CIDR parsing and source filtering. */
#include "IpFilter.h"

#include <stdexcept>

namespace codeharbor {

std::uint32_t parse_ipv4(const std::string& text) {
    std::uint32_t value = 0;
    std::size_t start = 0;
    for (int part = 0; part < 4; ++part) {
        const auto end = text.find('.', start);
        if ((part < 3 && end == std::string::npos) ||
            (part == 3 && end != std::string::npos)) {
            throw std::invalid_argument("invalid IPv4 address: " + text);
        }
        const std::string octet = text.substr(start, end - start);
        if (octet.empty() || octet.size() > 3 ||
            octet.find_first_not_of("0123456789") != std::string::npos) {
            throw std::invalid_argument("invalid IPv4 address: " + text);
        }
        const unsigned long number = std::stoul(octet);
        if (number > 255) throw std::invalid_argument("invalid IPv4 address: " + text);
        value = (value << 8) | static_cast<std::uint32_t>(number);
        start = end + 1;
    }
    return value;
}

void IpFilter::add_range(const std::string& cidr) {
    const auto slash = cidr.find('/');
    if (slash == std::string::npos || slash == cidr.size() - 1 ||
        cidr.find('/', slash + 1) != std::string::npos) {
        throw std::invalid_argument("invalid CIDR range: " + cidr);
    }
    const std::string prefix_text = cidr.substr(slash + 1);
    if (prefix_text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument("invalid CIDR range: " + cidr);
    }
    const unsigned long prefix = std::stoul(prefix_text);
    if (prefix > 32) throw std::invalid_argument("invalid CIDR range: " + cidr);
    const std::uint32_t mask = prefix == 0 ? 0u : UINT32_MAX << (32 - prefix);
    ranges_.push_back({parse_ipv4(cidr.substr(0, slash)) & mask, mask});
}

bool IpFilter::blocks(const std::string& ip) const {
    const std::uint32_t address = parse_ipv4(ip);
    for (const auto& range : ranges_) {
        if ((address & range.mask) == range.network) return true;
    }
    return false;
}

} // namespace codeharbor

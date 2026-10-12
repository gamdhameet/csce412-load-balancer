/** @file Request.cpp @brief Random request generation. */
#include "Request.h"

#include <sstream>

namespace codeharbor {

RequestGenerator::RequestGenerator(std::uint32_t seed, int min_time, int max_time,
                                   double processing_fraction, double blocked_fraction)
    : engine_(seed), address_(0, UINT32_MAX), time_(min_time, max_time),
      processing_(processing_fraction), blocked_(blocked_fraction) {}

Request RequestGenerator::next() {
    // Sampling from the blocked test range makes firewall activity visible in a short run.
    const std::uint32_t source = blocked_(engine_)
        ? 0xCB007100u + (address_(engine_) & 0xffu) // 203.0.113.0/24
        : address_(engine_);
    return {ipv4_to_string(source), ipv4_to_string(address_(engine_)),
            time_(engine_), processing_(engine_) ? 'P' : 'S'};
}

bool RequestGenerator::arrives(double probability) {
    return std::bernoulli_distribution(probability)(engine_);
}

std::string ipv4_to_string(std::uint32_t address) {
    std::ostringstream out;
    out << ((address >> 24) & 0xffu) << '.' << ((address >> 16) & 0xffu)
        << '.' << ((address >> 8) & 0xffu) << '.' << (address & 0xffu);
    return out.str();
}

} // namespace codeharbor

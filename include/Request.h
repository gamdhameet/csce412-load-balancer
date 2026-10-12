/** @file Request.h @brief One simulated web request and request generation. */
#pragma once

#include <cstdint>
#include <random>
#include <string>

namespace codeharbor {

/** A request carries an origin, destination, task length, and job type. */
struct Request {
    std::string ip_in;
    std::string ip_out;
    int time = 0;
    char type = 'P';
};

/** Builds repeatable random requests from a seeded engine. */
class RequestGenerator {
public:
    /** Configure request lengths, job mix, and blocked-source sampling. */
    RequestGenerator(std::uint32_t seed, int min_time, int max_time,
                     double processing_fraction, double blocked_fraction);

    /** Return a new request; each call advances the engine. */
    Request next();

    /** Return true with the configured arrival probability. */
    bool arrives(double probability);

private:
    std::mt19937 engine_;
    std::uniform_int_distribution<std::uint32_t> address_;
    std::uniform_int_distribution<int> time_;
    std::bernoulli_distribution processing_;
    std::bernoulli_distribution blocked_;
};

/** Convert a 32-bit IPv4 address to dotted decimal notation. */
std::string ipv4_to_string(std::uint32_t address);

} // namespace codeharbor

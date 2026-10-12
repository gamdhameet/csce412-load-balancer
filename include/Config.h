/** @file Config.h @brief Text-file settings for a reproducible simulation. */
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace codeharbor {

/** Values loaded from config.txt; user inputs for servers and cycles stay separate. */
struct Config {
    int min_task_cycles = 10;
    int max_task_cycles = 50;
    double arrival_probability = 0.35;
    double blocked_source_probability = 0.02;
    double processing_fraction = 0.50;
    unsigned scale_interval = 100;
    std::size_t min_servers_per_pool = 1;
    std::size_t max_servers_per_pool = 100;
    std::size_t max_queue_per_pool = 20000;
    std::vector<std::string> blocked_cidrs;
};

/** Read key=value settings and reject invalid or unknown entries. */
Config load_config(const std::string& path);

} // namespace codeharbor

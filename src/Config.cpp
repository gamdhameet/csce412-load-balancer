/** @file Config.cpp @brief Configuration parsing and validation. */
#include "Config.h"

#include "IpFilter.h"

#include <fstream>
#include <cstdint>
#include <set>
#include <stdexcept>

namespace codeharbor {
namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

template <typename T>
T number(const std::string& value, const std::string& key);

template <>
int number<int>(const std::string& value, const std::string& key) {
    std::size_t used = 0;
    const int result = std::stoi(value, &used);
    if (used != value.size()) throw std::invalid_argument("invalid " + key);
    return result;
}

template <>
unsigned number<unsigned>(const std::string& value, const std::string& key) {
    std::size_t used = 0;
    const unsigned long result = std::stoul(value, &used);
    if (used != value.size() || value[0] == '-' || result > UINT32_MAX)
        throw std::invalid_argument("invalid " + key);
    return static_cast<unsigned>(result);
}

template <>
std::size_t number<std::size_t>(const std::string& value, const std::string& key) {
    std::size_t used = 0;
    const unsigned long long result = std::stoull(value, &used);
    if (used != value.size() || value[0] == '-')
        throw std::invalid_argument("invalid " + key);
    return static_cast<std::size_t>(result);
}

template <>
double number<double>(const std::string& value, const std::string& key) {
    std::size_t used = 0;
    const double result = std::stod(value, &used);
    if (used != value.size()) throw std::invalid_argument("invalid " + key);
    return result;
}

void validate(const Config& config) {
    if (config.min_task_cycles < 1 || config.max_task_cycles < config.min_task_cycles)
        throw std::invalid_argument("task cycle range must be positive and ordered");
    for (const double p : {config.arrival_probability,
                           config.blocked_source_probability, config.processing_fraction}) {
        if (!(p >= 0.0 && p <= 1.0))
            throw std::invalid_argument("probabilities must be between 0 and 1");
    }
    if (config.scale_interval == 0 || config.min_servers_per_pool == 0 ||
        config.max_servers_per_pool < config.min_servers_per_pool ||
        config.max_queue_per_pool == 0 || config.max_servers_per_pool > SIZE_MAX / 80) {
        throw std::invalid_argument("scale interval, server bounds, and queue limit must be positive");
    }
    IpFilter filter;
    for (const auto& cidr : config.blocked_cidrs) filter.add_range(cidr);
}

} // namespace

Config load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open configuration: " + path);
    Config config;
    std::set<std::string> seen;
    std::string line;
    unsigned line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) continue;
        const auto equal = line.find('=');
        if (equal == std::string::npos)
            throw std::invalid_argument("config line " + std::to_string(line_number) + " needs =");
        const std::string key = trim(line.substr(0, equal));
        const std::string value = trim(line.substr(equal + 1));
        if (key.empty() || value.empty())
            throw std::invalid_argument("empty key or value on config line " + std::to_string(line_number));
        if (key != "blocked_cidr" && !seen.insert(key).second)
            throw std::invalid_argument("duplicate config key: " + key);

        if (key == "min_task_cycles") config.min_task_cycles = number<int>(value, key);
        else if (key == "max_task_cycles") config.max_task_cycles = number<int>(value, key);
        else if (key == "arrival_probability") config.arrival_probability = number<double>(value, key);
        else if (key == "blocked_source_probability") config.blocked_source_probability = number<double>(value, key);
        else if (key == "processing_fraction") config.processing_fraction = number<double>(value, key);
        else if (key == "scale_interval") config.scale_interval = number<unsigned>(value, key);
        else if (key == "min_servers_per_pool") config.min_servers_per_pool = number<std::size_t>(value, key);
        else if (key == "max_servers_per_pool") config.max_servers_per_pool = number<std::size_t>(value, key);
        else if (key == "max_queue_per_pool") config.max_queue_per_pool = number<std::size_t>(value, key);
        else if (key == "blocked_cidr") config.blocked_cidrs.push_back(value);
        else throw std::invalid_argument("unknown config key: " + key);
    }
    validate(config);
    return config;
}

} // namespace codeharbor

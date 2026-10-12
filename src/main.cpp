/** @file main.cpp @brief Command-line driver and simulation summary. */
#include "Config.h"
#include "IpFilter.h"
#include "Request.h"
#include "Switch.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

using namespace codeharbor;

namespace {

struct Options {
    std::string config_path = "config.txt";
    std::string log_path;
    std::string mode = "quiet";
    std::size_t servers = 0;
    unsigned cycles = 0;
    std::uint32_t seed = 412;
};

void print_terminal_block(const std::string& block) {
    if (isatty(STDOUT_FILENO)) {
        const auto end = block.find('\n');
        std::cout << "\033[1;36m" << block.substr(0, end) << "\033[0m"
                  << block.substr(end);
    } else {
        std::cout << block;
    }
}

unsigned long long positive_integer(const std::string& text, const std::string& label) {
    if (text.empty() || text[0] == '-' || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument(label + " must be a positive integer");
    std::size_t used = 0;
    const unsigned long long value = std::stoull(text, &used);
    if (used != text.size() || value == 0)
        throw std::invalid_argument(label + " must be a positive integer");
    return value;
}

Options parse_options(int argc, char* argv[]) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help") {
            std::cout << "Usage: ./load-balancer [--servers N] [--cycles N] [--seed N] "
                         "[--mode quiet|verbose] [--config FILE] [--log FILE]\n"
                         "Missing server count or cycle count is requested interactively.\n";
            std::exit(0);
        }
        if (i + 1 >= argc) throw std::invalid_argument("missing value after " + key);
        const std::string value = argv[++i];
        if (key == "--servers") options.servers = positive_integer(value, "servers");
        else if (key == "--cycles") {
            const auto n = positive_integer(value, "cycles");
            if (n >= std::numeric_limits<unsigned>::max()) throw std::invalid_argument("cycles too large");
            options.cycles = static_cast<unsigned>(n);
        } else if (key == "--seed") {
            if (value.empty() || value[0] == '-' || value.find_first_not_of("0123456789") != std::string::npos)
                throw std::invalid_argument("seed must be a nonnegative integer");
            const auto n = std::stoull(value);
            if (n > UINT32_MAX) throw std::invalid_argument("seed too large");
            options.seed = static_cast<std::uint32_t>(n);
        } else if (key == "--mode") options.mode = value;
        else if (key == "--config") options.config_path = value;
        else if (key == "--log") options.log_path = value;
        else throw std::invalid_argument("unknown option: " + key);
    }
    if (options.mode != "quiet" && options.mode != "verbose")
        throw std::invalid_argument("mode must be quiet or verbose");
    return options;
}

void prompt_if_missing(Options& options) {
    std::string line;
    if (options.servers == 0) {
        std::cout << "Initial servers (minimum 2): " << std::flush;
        if (!std::getline(std::cin, line)) throw std::runtime_error("server input ended");
        options.servers = positive_integer(line, "servers");
    }
    if (options.cycles == 0) {
        std::cout << "Clock cycles: " << std::flush;
        if (!std::getline(std::cin, line)) throw std::runtime_error("cycle input ended");
        const auto n = positive_integer(line, "cycles");
        if (n >= std::numeric_limits<unsigned>::max()) throw std::invalid_argument("cycles too large");
        options.cycles = static_cast<unsigned>(n);
    }
}

void append_pool_summary(std::ostream& out, const LoadBalancer& pool) {
    const auto& stats = pool.stats();
    out << '\n' << pool.name() << " pool\n"
        << "  queue end: " << pool.waiting() << " | peak: " << stats.peak_queue << '\n'
        << "  servers end: " << pool.server_count() << " | busy: " << pool.busy_count()
        << " | idle: " << pool.idle_count() << '\n'
        << "  servers added: " << stats.servers_added << " | removed: " << stats.servers_removed
        << " | peak: " << stats.peak_servers << " | minimum: " << stats.minimum_servers << '\n'
        << "  requests accepted: " << stats.accepted << " | assigned: " << stats.assigned
        << " | completed: " << stats.completed << " | queue-full discarded: " << stats.discarded << '\n';
    out << "  scale history:";
    if (pool.scale_events().empty()) out << " none\n";
    else {
        out << '\n';
        for (const auto& event : pool.scale_events()) {
            out << "    cycle " << event.cycle << ": " << event.action
                << " -> " << event.servers << " servers, " << event.waiting << " waiting\n";
        }
    }
}

std::string summary(const Switch& router, const Options& options, const Config& config,
                    std::size_t starting_queue, std::size_t initial_generated) {
    std::ostringstream out;
    const auto& processing = router.processing();
    const auto& streaming = router.streaming();
    out << "CodeHarbor load balancer | run summary\n"
        << "Mode: " << options.mode << " | seed: " << options.seed
        << " | clock cycles: " << options.cycles << '\n'
        << "Initial servers: " << options.servers << " (processing "
        << (options.servers + 1) / 2 << ", streaming " << options.servers / 2 << ")\n"
        << "Task time range: " << config.min_task_cycles << "-" << config.max_task_cycles << " cycles\n"
        << "Starting queue size: " << starting_queue << '\n'
        << "Ending queue size: " << router.waiting() << '\n'
        << "Generated requests: " << router.generated_count() << " (initial "
        << initial_generated << ", during run " << router.generated_count() - initial_generated << ")\n"
        << "IP-filter rejected: " << router.blocked_count() << '\n'
        << "Queue-full discarded: " << processing.stats().discarded + streaming.stats().discarded << '\n'
        << "Completed requests: " << processing.stats().completed + streaming.stats().completed << '\n'
        << "Active servers at end: " << processing.busy_count() + streaming.busy_count() << '\n'
        << "Inactive servers at end: " << processing.idle_count() + streaming.idle_count() << '\n'
        << "Allocated servers at end: " << processing.server_count() + streaming.server_count() << '\n';
    append_pool_summary(out, processing);
    append_pool_summary(out, streaming);
    return out.str();
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        Options options = parse_options(argc, argv);
        const Config config = load_config(options.config_path);
        prompt_if_missing(options);
        if (options.servers < 2 * config.min_servers_per_pool ||
            (options.servers + 1) / 2 > config.max_servers_per_pool ||
            options.servers / 2 > config.max_servers_per_pool)
            throw std::invalid_argument("initial server count is outside the two pool limits");

        IpFilter filter;
        for (const auto& cidr : config.blocked_cidrs) filter.add_range(cidr);
        Switch router(options.servers, config.min_servers_per_pool,
                      config.max_servers_per_pool, config.max_queue_per_pool,
                      config.scale_interval, filter);
        RequestGenerator generator(options.seed, config.min_task_cycles, config.max_task_cycles,
                                   config.processing_fraction, config.blocked_source_probability);

        const std::size_t target_processing = ((options.servers + 1) / 2) * 100;
        const std::size_t target_streaming = (options.servers / 2) * 100;
        if (target_processing > config.max_queue_per_pool || target_streaming > config.max_queue_per_pool)
            throw std::invalid_argument("max_queue_per_pool is too small for the initial queue");

        std::ofstream log_file;
        if (!options.log_path.empty()) {
            const auto parent = std::filesystem::path(options.log_path).parent_path();
            if (!parent.empty()) std::filesystem::create_directories(parent);
            log_file.open(options.log_path);
            if (!log_file) throw std::runtime_error("cannot write log: " + options.log_path);
        }
        std::ostream* verbose = options.mode == "verbose"
            ? (log_file ? static_cast<std::ostream*>(&log_file) : &std::cout) : nullptr;

        // Seed each queue to 100 requests per initial server. Blocked attempts are replaced.
        const std::size_t max_initial_attempts = 100 * (target_processing + target_streaming);
        while (router.processing().waiting() < target_processing) {
            if (router.generated_count() >= max_initial_attempts)
                throw std::runtime_error("could not fill initial queue; check blocked source settings");
            Request request = generator.next();
            request.type = 'P';
            router.route(request, 0, nullptr);
        }
        while (router.streaming().waiting() < target_streaming) {
            if (router.generated_count() >= max_initial_attempts)
                throw std::runtime_error("could not fill initial queue; check blocked source settings");
            Request request = generator.next();
            request.type = 'S';
            router.route(request, 0, nullptr);
        }
        const std::size_t starting_queue = router.waiting();
        const std::size_t initial_generated = router.generated_count();
        std::ostringstream start;
        start << "CodeHarbor load balancer | start\n"
              << "Initial servers: " << options.servers << " | clock cycles: " << options.cycles
              << " | starting queue size: " << starting_queue << '\n'
              << "Task time range: " << config.min_task_cycles << "-" << config.max_task_cycles
              << " cycles | blocked CIDR ranges: " << filter.range_count() << "\n\n";
        print_terminal_block(start.str());
        if (log_file) log_file << start.str();
        if (verbose) *verbose << "cycle 0 initial queues loaded: processing="
                              << router.processing().waiting() << " streaming="
                              << router.streaming().waiting() << '\n';
        for (unsigned cycle = 1; cycle <= options.cycles; ++cycle) {
            if (generator.arrives(config.arrival_probability))
                router.route(generator.next(), cycle, verbose);
            router.tick(cycle, verbose);
        }
        const std::string report = summary(router, options, config, starting_queue, initial_generated);
        print_terminal_block(report);
        if (log_file) log_file << report;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

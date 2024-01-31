// ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
// Copyright 2018 Pawel Bylica.
// Licensed under the Apache License, Version 2.0. See the LICENSE file.

#include <ethash/ethash.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>


using namespace std::chrono;
using timer = std::chrono::steady_clock;

namespace
{
class ethash_interface
{
public:
    virtual ~ethash_interface() noexcept = default;

    virtual void search(const ethash::hash256& header_hash, uint64_t nonce, size_t iterations) const
        noexcept = 0;
};

class ethash_light : public ethash_interface
{
    const ethash::epoch_context& context;

public:
    explicit ethash_light(int epoch_number)
      : context(ethash::get_global_epoch_context(epoch_number))
    {}

    void search(const ethash::hash256& header_hash, uint64_t nonce, size_t iterations) const
        noexcept override
    {
        ethash::search_light(context, header_hash, {}, nonce, iterations);
    }
};

class ethash_full : public ethash_interface
{
    const ethash::epoch_context_full& context;

public:
    explicit ethash_full(int epoch_number)
      : context(ethash::get_global_epoch_context_full(epoch_number))
    {}

    void search(const ethash::hash256& header_hash, uint64_t nonce, size_t iterations) const
        noexcept override
    {
        ethash::search(context, header_hash, {}, nonce, iterations);
    }
};


std::atomic<int> shared_block_number{0};
std::atomic<int> num_hashes{0};

void worker(bool light, const ethash::hash256& header_hash, uint64_t start_nonce, int batch_size)
{
    int current_epoch = -1;
    std::unique_ptr<ethash_interface> ei;
    uint64_t i = 0;
    size_t w = static_cast<size_t>(batch_size);
    while (true)
    {
        int block_number = shared_block_number.load(std::memory_order_relaxed);
        if (block_number < 0)
            break;

        int e = ethash::get_epoch_number(block_number);

        if (current_epoch != e)
        {
            ei.reset(
                light ? static_cast<ethash_interface*>(new ethash_light{e}) : new ethash_full{e});
            current_epoch = e;
        }

        ei->search(header_hash, start_nonce + i, w);
        num_hashes.fetch_add(batch_size, std::memory_order_relaxed);
        i += w;
    }
}
}  // namespace

int main(int argc, const char* argv[])
{
    int num_blocks = 10;
    int start_block_number = 0;
    int block_time = 6;
    int work_size = 100;
    int num_threads = static_cast<int>(std::thread::hardware_concurrency());
    uint64_t start_nonce = 0;
    bool light = false;

    for (int i = 0; i < argc; ++i)
    {
        const std::string arg{argv[i]};

        if (arg == "--light")
            light = true;
        else if (arg == "-i" && i + 1 < argc)
            num_blocks = std::stoi(argv[++i]);
        else if (arg == "-b" && i + 1 < argc)
            start_block_number = std::stoi(argv[++i]);
        else if (arg == "-t" && i + 1 < argc)
            num_threads = std::stoi(argv[++i]);
        else if (arg == "-n" && i + 1 < argc)
            start_nonce = std::stoul(argv[++i]);
    }

    auto flags = std::cout.flags();
    std::cout << std::fixed << std::setprecision(2);

    // clang-format off
    std::cout << "Fakeminer Benchmark\n\nParameters:"
              << "\n  dataset:     " << (light ? "light" : "full")
              << "\n  threads:     " << num_threads
              << "\n  blocks:      " << num_blocks
              << "\n  block time:  " << block_time
              << "\n  batch size:  " << work_size
              << "\n  start nonce: " << start_nonce
              << "\n\n";
    // clang-format on

    const uint64_t divisor = static_cast<uint64_t>(num_threads);
    const uint64_t nonce_space_per_thread = std::numeric_limits<uint64_t>::max() / divisor;

    const ethash::hash256 header_hash{};

    shared_block_number.store(start_block_number, std::memory_order_relaxed);
    std::vector<std::future<void>> futures;

    for (int t = 0; t < num_threads; ++t)
    {
        futures.emplace_back(
            std::async(std::launch::async, worker, light, header_hash, start_nonce, work_size));
        start_nonce += nonce_space_per_thread;
    }

    std::cout << "Progress:\n"
              << "                  |-----    hashrate    -----|  |-----    bandwidth   -----|\n"
              << "  epoch    block        current       average         current       average\n";

    int all_hashes = 0;
    auto start_time = timer::now();
    auto time = start_time;
    static constexpr int khps_mbps_ratio =
        ethash::num_dataset_accesses * ethash::full_dataset_item_size / 1024;

    double current_duration = 0;
    double all_duration = 0;
    double current_khps = 0;
    double average_khps = 0;
    double current_bandwidth = 0;
    double average_bandwidth = 0;

    const milliseconds block_time_ms{block_time * 1000};
    milliseconds sleep_time = block_time_ms;
    const int end_block_number = start_block_number + num_blocks;
    for (int block_number = start_block_number; block_number < end_block_number; ++block_number)
    {
        std::this_thread::sleep_for(sleep_time);
        int current_hashes = num_hashes.exchange(0, std::memory_order_relaxed);
        all_hashes += current_hashes;

        auto now = timer::now();
        current_duration = double(duration_cast<milliseconds>(now - time).count());
        all_duration = double(duration_cast<milliseconds>(now - start_time).count());
        time = now;

        shared_block_number.store(block_number + 1, std::memory_order_relaxed);

        int e = ethash::get_epoch_number(block_number);

        current_khps = double(current_hashes) / current_duration;
        average_khps = double(all_hashes) / all_duration;
        current_bandwidth = double(current_hashes * khps_mbps_ratio) / 1024 / current_duration;
        average_bandwidth = double(all_hashes * khps_mbps_ratio) / 1024 / all_duration;

        std::cout << std::setw(7) << e << std::setw(9) << block_number << std::setw(10)
                  << current_khps << " kh/s" << std::setw(9) << average_khps << " kh/s"
                  << std::setw(10) << current_bandwidth << " GiB/s" << std::setw(8)
                  << average_bandwidth << " GiB/s\n";

        sleep_time = block_time_ms - duration_cast<milliseconds>(timer::now() - now);
    }

    shared_block_number.store(-1, std::memory_order_relaxed);
    for (auto& future : futures)
        future.wait();

    auto total_seconds = all_duration / 1000;

    std::cout << "\nSummary:\n  time:                     " << std::setw(7) << total_seconds
              << " s\n  latest hashrate:          " << std::setw(7) << current_khps
              << " kh/s\n  average hashrate:         " << std::setw(7) << average_khps
              << " kh/s\n  latest memory bandwitdh:  " << std::setw(7) << current_bandwidth
              << " GiB/s\n  average memory bandwitdh: " << std::setw(7) << average_bandwidth
              << " GiB/s\n";

    std::cout.flags(flags);
    return 0;
}

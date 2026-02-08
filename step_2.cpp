#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <cstdint>
#include <cmath>
#include <set>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <windows.h>

class HashFuncGen {
public:
    explicit HashFuncGen(uint32_t seed = 0) : seed_(seed) {}

    uint32_t operator()(const std::string& key) const {
        return murmur3_32(key.c_str(), (int)key.size(), seed_);
    }

private:
    uint32_t seed_;

    static uint32_t murmur3_32(const void* key, int len, uint32_t seed) {
        const uint8_t* data = (const uint8_t*)key;
        const int nblocks = len / 4;
        uint32_t h1 = seed;
        const uint32_t c1 = 0xcc9e2d51;
        const uint32_t c2 = 0x1b873593;
        const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

        for (int i = -nblocks; i; i++) {
            uint32_t k1 = blocks[i];
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> 17);
            k1 *= c2;
            h1 ^= k1;
            h1 = (h1 << 13) | (h1 >> 19);
            h1 = h1 * 5 + 0xe6546b64;
        }

        const uint8_t* tail = (data + nblocks * 4);
        uint32_t k1 = 0;
        switch (len & 3) {
            case 3:
                k1 ^= tail[2] << 16;
            case 2:
                k1 ^= tail[1] << 8;
            case 1:
                k1 ^= tail[0];
                k1 *= c1; k1 = (k1 << 15) | (k1 >> 17); k1 *= c2; h1 ^= k1;
        }

        h1 ^= len;
        h1 ^= h1 >> 16;
        h1 *= 0x85ebca6b;
        h1 ^= h1 >> 13;
        h1 *= 0xc2b2ae35;
        h1 ^= h1 >> 16;
        return h1;
    }
};

class RandomStreamGen {
public:
    RandomStreamGen() : rng_(std::random_device{}()) {
        alphabet_ = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-";
        dist_ = std::uniform_int_distribution<size_t>(0, alphabet_.size() - 1);
    }

    std::string next_string() {
        std::string res;
        res.reserve(30);
        for (int i = 0; i < 30; ++i) {
            res += alphabet_[dist_(rng_)];
        }
        return res;
    }

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<size_t> dist_;
    std::string alphabet_;
};

class HyperLogLog {
public:
    HyperLogLog(int b, uint32_t seed) : b_(b), m_(1 << b), hasher_(seed) {
        registers_.resize(m_, 0);
        if (m_ == 16) {
            alpha_m_ = 0.673;
        }
        else if (m_ == 32) {
            alpha_m_ = 0.697;
        }
        else if (m_ == 64) {
            alpha_m_ = 0.709;
        }
        else {
            alpha_m_ = 0.7213 / (1.0 + 1.079 / m_);
        }
    }

    void add(const std::string& item) {
        uint32_t hash = hasher_(item);
        uint32_t j = hash >> (32 - b_);
        uint32_t w = hash << b_;
        uint8_t rho = get_leading_zeros(w) + 1;

        if (rho > registers_[j]) {
            registers_[j] = rho;
        }
    }

    double estimate() const {
        double sum = 0.0;
        for (int val : registers_) {
            sum += std::pow(2.0, -val);
        }

        double E = alpha_m_ * m_ * m_ / sum;

        if (E <= 2.5 * m_) {
            double V = 0;
            for (int val : registers_) {
                if (val == 0) V++;
            }
            if (V > 0) {
                E = m_ * std::log(static_cast<double>(m_) / V);
            }
        }
        return E;
    }

private:
    int b_;
    int m_;
    double alpha_m_;
    std::vector<uint8_t> registers_;
    HashFuncGen hasher_;

    uint8_t get_leading_zeros(uint32_t x) const {
        if (x == 0) return 32 - b_;
        uint8_t zeros = 0;
        for (int i = 31; i >= 0; --i) {
            if ((x >> i) & 1) {
                break;
            }
            zeros++;
        }
        return zeros;
    }
};

int main() {
    SetConsoleOutputCP(CP_UTF8);
    const int B_BITS = 12;
    const int NUM_RUNS = 10;
    const size_t STREAM_SIZE = 100000;
    const size_t RECORD_STEP = 500;

    std::cout << "B=" << B_BITS << ", Runs=" << NUM_RUNS << ", Stream=" << STREAM_SIZE << std::endl;

    std::vector<std::vector<double>> run_estimates(NUM_RUNS);
    std::vector<size_t> exact_counts_reference;

    for (int run = 0; run < NUM_RUNS; ++run) {
        RandomStreamGen streamGen;
        HyperLogLog hll(B_BITS, 42 + run);
        std::set<std::string> unique_set;

        std::cout << "Processing Run " << (run + 1) << "..." << std::endl;

        for (size_t i = 0; i < STREAM_SIZE; ++i) {
            std::string item = streamGen.next_string();

            hll.add(item);
            unique_set.insert(item);

            if ((i + 1) % RECORD_STEP == 0) {
                run_estimates[run].push_back(hll.estimate());

                if (run == 0) {
                    exact_counts_reference.push_back(unique_set.size());
                }
            }
        }
    }

    std::ofstream file("hll_results.csv");
    file << "Step,Exact_Ref,Avg_Est,Std_Dev,Lower_Bound,Upper_Bound\n";

    size_t num_points = exact_counts_reference.size();

    for (size_t i = 0; i < num_points; ++i) {
        double sum = 0.0;
        for (int r = 0; r < NUM_RUNS; ++r) {
            sum += run_estimates[r][i];
        }
        double mean = sum / NUM_RUNS;

        double sq_sum = 0.0;
        for (int r = 0; r < NUM_RUNS; ++r) {
            sq_sum += std::pow(run_estimates[r][i] - mean, 2);
        }
        double std_dev = std::sqrt(sq_sum / NUM_RUNS);

        file << ((i + 1) * RECORD_STEP) << ","
             << exact_counts_reference[i] << ","
             << mean << ","
             << std_dev << ","
             << (mean - std_dev) << ","
             << (mean + std_dev) << "\n";
    }

    file.close();
    std::cout << "Запись в файл 'hll_results.csv' завершена" << std::endl;

    return 0;
}

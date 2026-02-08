#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <cstdint>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <map>
#include <locale>
#include <windows.h>

class HashFuncGen {
public:
    explicit HashFuncGen(uint32_t seed = 0) : seed_(seed) {}

    uint32_t operator()(const std::string& key) const {
        return murmur3_32(key.c_str(), key.size(), seed_);
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

        const uint8_t* tail = data + nblocks * 4;
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

    std::vector<std::string> generate_stream(size_t total_size) {
        std::vector<std::string> stream;
        stream.reserve(total_size);
        for (size_t i = 0; i < total_size; ++i) {
            stream.push_back(next_string());
        }
        return stream;
    }

private:
    std::mt19937 rng_;
    std::uniform_int_distribution<size_t> dist_;
    std::string alphabet_;
};

int main() {
    SetConsoleOutputCP(CP_UTF8);

    const size_t TEST_SIZE = 100000;
    const size_t BUCKETS = 10;

    RandomStreamGen gen;
    HashFuncGen hasher(123);

    std::map<uint32_t, int> collisions;

    std::vector<int> distribution(BUCKETS, 0);

    std::cout << "Тестируем на " << TEST_SIZE << " элементах..." << std::endl;

    for(size_t i = 0; i < TEST_SIZE; ++i) {
        std::string s = gen.next_string();
        uint32_t h = hasher(s);

        collisions[h]++;

        size_t bucket_idx = (size_t)((double)h / 4294967296.0 * BUCKETS);
        if (bucket_idx < BUCKETS) {
            distribution[bucket_idx]++;
        }
    }

    int collision_count = 0;
    for(auto const& [hash, count] : collisions) {
        if (count > 1) {
            collision_count++;
        }
    }

    std::cout << "Коллизий найдено: " << collision_count << std::endl;
    std::cout << "Распределение (Цель ~" << TEST_SIZE / BUCKETS << "):" << std::endl;

    for(int i = 0; i < BUCKETS; ++i) {
        std::cout << "Корзины " << i << ": " << distribution[i] << std::endl;
    }

    return 0;
}
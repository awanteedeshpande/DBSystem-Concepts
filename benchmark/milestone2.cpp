#include "BPlusTree.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>


namespace {

#ifndef NDEBUG
constexpr std::size_t NUM_TUPLES = 5e6;
constexpr std::size_t NUM_POINT_LOOKUPS = 1e5;
#else
constexpr std::size_t NUM_TUPLES = 5e7;
constexpr std::size_t NUM_POINT_LOOKUPS = 5e5;
#endif

}

std::size_t no_dead_code;


template<typename Generator>
std::vector<int32_t> gen_data(Generator &g)
{
    std::vector<int32_t> vec;
    vec.reserve(NUM_TUPLES);

    std::uniform_int_distribution<> dist_repetition(10, 100);

    int32_t current = 0;
    for (;;) {
        std::uniform_int_distribution<> dist_value(current + 1, current + 10);
        current = dist_value(g);

        std::size_t n = dist_repetition(g);

        while (n--) {
            vec.emplace_back(current);
            if (vec.size() == NUM_TUPLES)
                goto exit;
        }
    }
exit:

    assert(vec.size() == NUM_TUPLES);
    return vec;
};

template<typename Generator>
std::vector<int32_t> gen_misses(Generator &g, const std::vector<int32_t> &keys)
{
    using std::begin, std::end;

    std::vector<int32_t> vec;
    vec.reserve(NUM_POINT_LOOKUPS);

    std::uniform_int_distribution<> dist_value(*keys.begin() - 10, *keys.rbegin() + 10);
    std::uniform_int_distribution<> dist_repetition(10, 100);

    for (;;) {
        /* Find a key that is not in `keys`. */
        int32_t k;
        do
            k = dist_value(g);
        while (std::binary_search(begin(keys), end(keys), k));

        /* Append multiple repetitions of that key. */
        std::size_t n = dist_repetition(g);
        while (n--) {
            vec.emplace_back(k);
            if (vec.size() == NUM_POINT_LOOKUPS)
                goto exit;
        }
    }
exit:

    assert(vec.size() == NUM_POINT_LOOKUPS);
    return vec;
}


int main()
{
    using std::begin, std::end;
    using btree_type = BPlusTree<int32_t, int32_t>;

    std::mt19937 g(0);

    auto keys = gen_data(g);
    assert(std::is_sorted(keys.begin(), keys.end()));

    std::vector<btree_type::value_type> data;
    for (auto k : keys)
        data.emplace_back(k, 2*k);

    using namespace std::chrono;

    /* Evaluate bulkload performance. */
    auto t_bulkload_begin = steady_clock::now();
    auto tree = btree_type::Bulkload(data);
    auto t_bulkload_end = steady_clock::now();
    std::cout << "milestone2,bulkload," << duration_cast<milliseconds>(t_bulkload_end - t_bulkload_begin).count()
              << '\n';

    /* Generate missing keys used for lookups. */
    auto missing_keys = gen_misses(g, keys);
    std::shuffle(begin(missing_keys), end(missing_keys), g);

    /* Generate key sets for lookups with varying hit/miss ratios. */
#define PREPARE_KEYS(HIT, MISS) \
    constexpr std::size_t NUM_KEYS_##HIT##_##MISS = NUM_POINT_LOOKUPS * HIT / (HIT + MISS); \
    std::vector<typename btree_type::key_type> keys_##HIT##_##MISS; \
    keys_##HIT##_##MISS .reserve(NUM_KEYS_##HIT##_##MISS); \
    std::sample(begin(keys), end(keys), std::back_inserter(keys_##HIT##_##MISS), NUM_KEYS_##HIT##_##MISS, g); \
    std::copy_n(begin(missing_keys), (NUM_POINT_LOOKUPS - NUM_KEYS_##HIT##_##MISS), \
                std::back_inserter(keys_##HIT##_##MISS)); \
    std::shuffle(begin(keys_##HIT##_##MISS), end(keys_##HIT##_##MISS), g)

    PREPARE_KEYS(10, 90);
    PREPARE_KEYS(50, 50);
    PREPARE_KEYS(90, 10);

#undef PREPARE_KEYS

    /* Benchmark point lookups. */
#define BENCH_LOOKUP_POINT(HIT, MISS) { \
    auto t_lookup_begin = steady_clock::now(); \
    for (auto k : keys_##HIT##_##MISS) { \
        no_dead_code += tree.find(k) != tree.end(); \
    } \
    auto t_lookup_end = steady_clock::now(); \
    std::cout << "milestone2,lookup_point_" #HIT "_" #MISS "," \
              << duration_cast<milliseconds>(t_lookup_end - t_lookup_begin).count() << '\n'; \
}

    BENCH_LOOKUP_POINT(10, 90);
    BENCH_LOOKUP_POINT(50, 50);
    BENCH_LOOKUP_POINT(90, 10);

#undef BENCH_LOOKUP_POINT

    /* Benchmark range lookups. */
    std::vector<std::size_t> key_pos;
    for (std::size_t i = 0; i != 50; ++i)
        key_pos.emplace_back(keys.size() / 100 * i);
    const std::size_t RANGE_WIDTH = NUM_TUPLES / 50; // 2%

    auto t_lookup_begin = steady_clock::now();
    for (auto pos : key_pos) {
        assert(pos + RANGE_WIDTH < keys.size());
        const auto k_lo = keys[pos];
        const auto k_hi = keys[pos + RANGE_WIDTH];

        auto range = tree.in_range(k_lo, k_hi);
        for (auto v : range)
            no_dead_code += v.first;
    }
    auto t_lookup_end = steady_clock::now();
    std::cout << "milestone2,lookup_range," << duration_cast<milliseconds>(t_lookup_end - t_lookup_begin).count()
              << '\n';
}

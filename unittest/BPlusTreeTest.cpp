#include "catch.hpp"

#include "BPlusTree.hpp"
// #include "BPlusTree-todo.hpp"
#include <array>
#include <typeinfo>
#include <vector>


#define BTREE_SECTION(KEY_TYPE, VALUE_TYPE) \
    DYNAMIC_SECTION(#KEY_TYPE "  -->  " #VALUE_TYPE)


namespace {

template<typename key_type, typename value_type>
void __test_bulkload()
{
    using btree_type = BPlusTree<key_type, value_type>;

    SECTION("empty")
    {
        std::array<typename btree_type::value_type, 0> data;
        auto tree = btree_type::Bulkload(data);

        CHECK(tree.size() == 0);
        CHECK(tree.height() == 0);

        auto it = tree.begin();
        CHECK(it == tree.end());
    }

    SECTION("one")
    {
        std::array<typename btree_type::value_type, 1> data = { {
            { 42, 13 },
        } };
        auto tree = btree_type::Bulkload(data);

        CHECK(tree.size() == 1);
        CHECK(tree.height() == 0);

        auto it = tree.begin();
        REQUIRE(it != tree.end());
        CHECK(it->first  == 42);
        CHECK(it->second == 13);
        ++it;
        CHECK(it == tree.end());
    }
}

template<typename key_type, typename value_type>
void __test_point_lookup()
{
    using btree_type = BPlusTree<key_type, value_type>;

    SECTION("empty")
    {
        std::array<typename btree_type::value_type, 0> data;
        auto tree = btree_type::Bulkload(data);

        auto it = tree.find(42);
        CHECK(it == tree.end());
    }

    SECTION("one")
    {
        std::array<typename btree_type::value_type, 1> data = { {
            { 42, 13 },
        } };
        auto tree = btree_type::Bulkload(data);

        {
            auto it = tree.find(42);
            REQUIRE(it != tree.end());
            CHECK(it->first == 42);
            CHECK(it->second == 13);
        }

        {
            auto it = tree.find(0);
            CHECK(it == tree.end());
        }

        {
            auto it = tree.find(137);
            CHECK(it == tree.end());
        }
    }

    SECTION("two")
    {
        std::array<typename btree_type::value_type, 2> data = { {
            {  42, 13 },
            { 137, 16 },
        } };
        auto tree = btree_type::Bulkload(data);

        {
            auto it = tree.find(0);
            CHECK(it == tree.end());
        }

        {
            auto it = tree.find(42);
            REQUIRE(it != tree.end());
            CHECK(it->first == 42);
            CHECK(it->second == 13);
        }

        {
            auto it = tree.find(64);
            CHECK(it == tree.end());
        }

        {
            auto it = tree.find(137);
            REQUIRE(it != tree.end());
            CHECK(it->first == 137);
            CHECK(it->second == 16);
        }

        {
            auto it = tree.find(1024);
            CHECK(it == tree.end());
        }
    }

    SECTION("consecutive")
    {
        std::vector<typename btree_type::value_type> data;
        for (typename btree_type::key_type i = 0; i != 100; ++i) {
            data.emplace_back(i, i*i);
        }
        auto tree = btree_type::Bulkload(data);

        for (typename btree_type::key_type i = 0; i != 100; ++i) {
            auto it = tree.find(i);
            REQUIRE(it != tree.end());
            CHECK(it->first == i);
            CHECK(it->second == i*i);
        }

        {
            auto it = tree.find(-1);
            CHECK(it == tree.end());
        }

        {
            auto it = tree.find(100);
            CHECK(it == tree.end());
        }
    }
}

template<typename key_type, typename value_type>
void __test_range_lookup()
{
    using btree_type = BPlusTree<key_type, value_type>;

    SECTION("empty")
    {
        std::array<typename btree_type::value_type, 0> data;
        auto tree = btree_type::Bulkload(data);

        auto range = tree.in_range(0, 42);
        CHECK(range.empty());
    }

    SECTION("one")
    {
        std::array<typename btree_type::value_type, 1> data = { {
            { 42, 13 },
        } };
        auto tree = btree_type::Bulkload(data);

        {
            auto range = tree.in_range(0, 42);
            CHECK(range.empty());
        }

        {
            auto range = tree.in_range(42, 43);
            REQUIRE_FALSE(range.empty());
            auto it = range.begin();
            CHECK(it->first == 42);
            CHECK(it->second == 13);
            ++it;
            CHECK(it == range.end());
        }

        {
            auto range = tree.in_range(43, 100);
            REQUIRE(range.empty());
        }
    }

    SECTION("two")
    {
        std::array<typename btree_type::value_type, 2> data = { {
            {  42, 13 },
            { 137, 16 },
        } };
        auto tree = btree_type::Bulkload(data);

        {
            auto range = tree.in_range(0, 42);
            CHECK(range.empty());
        }

        {
            auto range = tree.in_range(42, 43);
            REQUIRE_FALSE(range.empty());
            auto it = range.begin();
            CHECK(it->first == 42);
            CHECK(it->second == 13);
            ++it;
            CHECK(it == range.end());
        }

        {
            auto range = tree.in_range(43, 137);
            CHECK(range.empty());
        }

        {
            auto range = tree.in_range(137, 138);
            REQUIRE_FALSE(range.empty());
            auto it = range.begin();
            CHECK(it->first == 137);
            CHECK(it->second == 16);
            ++it;
            CHECK(it == range.end());
        }

        {
            auto range = tree.in_range(138, 200);
            CHECK(range.empty());
        }

        {
            auto range = tree.in_range(42, 138);
            REQUIRE_FALSE(range.empty());
            auto it = range.begin();
            CHECK(it->first == 42);
            CHECK(it->second == 13);
            ++it;
            REQUIRE(it != range.end());
            CHECK(it->first == 137);
            CHECK(it->second == 16);
            ++it;
            CHECK(it == range.end());
        }
    }

    SECTION("consecutive")
    {
        std::vector<typename btree_type::value_type> data;
        for (typename btree_type::key_type i = 0; i != 100; ++i) {
            data.emplace_back(i, i*i);
        }
        auto tree = btree_type::Bulkload(data);

        for (typename btree_type::key_type i = 0; i != 100; ++i) {
            auto range = tree.in_range(i, i+1);
            REQUIRE_FALSE(range.empty());
            auto it = range.begin();
            REQUIRE(it != range.end());
            CHECK(it->first == i);
            CHECK(it->second == i*i);
            ++it;
            CHECK(it == range.end());
        }

        {
            auto range = tree.in_range(-100, 0);
            REQUIRE(range.empty());
        }

        {
            auto range = tree.in_range(100, 200);
            REQUIRE(range.empty());
        }

        {
            auto range = tree.in_range(0, 100);
            REQUIRE_FALSE(range.empty());

            auto it = range.begin();
            for (typename btree_type::key_type i = 0; i != 100; ++i, ++it) {
                REQUIRE(it != range.end());
                CHECK(it->first == i);
                CHECK(it->second == i*i);
            }
            CHECK(it == range.end());
        }
    }
}

}


TEST_CASE("BPlusTree/node size", "[milestone2]")
{
#define TEST(KEY_TYPE, VALUE_TYPE) { \
        using btree_type = BPlusTree<KEY_TYPE, VALUE_TYPE>; \
        CHECK(sizeof(btree_type::inner_node) <= 64); \
        CHECK(sizeof(btree_type::leaf_node) <= 64); \
    }

    TEST(int32_t, int32_t);
    TEST(int32_t, int64_t);
    TEST(int64_t, int32_t);
    TEST(int64_t, int64_t);

#undef TEST
}

TEST_CASE("BPlusTree/c'tor", "[milestone2]")
{
#define TEST(KEY_TYPE, VALUE_TYPE) \
    BTREE_SECTION(KEY_TYPE, VALUE_TYPE) { __test_bulkload<KEY_TYPE, VALUE_TYPE>(); }

    TEST(int32_t, int32_t);
    TEST(int32_t, int64_t);
    TEST(int64_t, int32_t);
    TEST(int64_t, int64_t);

#undef TEST
}

TEST_CASE("BPlusTree/point lookup", "[milestone2]")
{
#define TEST(KEY_TYPE, VALUE_TYPE) \
    BTREE_SECTION(KEY_TYPE, VALUE_TYPE) { __test_point_lookup<KEY_TYPE, VALUE_TYPE>(); }

    TEST(int32_t, int32_t);
    TEST(int32_t, int64_t);
    TEST(int64_t, int32_t);
    TEST(int64_t, int64_t);

#undef TEST
}

TEST_CASE("BPlusTree/range lookup", "[milestone2]")
{
#define TEST(KEY_TYPE, VALUE_TYPE) \
    BTREE_SECTION(KEY_TYPE, VALUE_TYPE) { __test_range_lookup<KEY_TYPE, VALUE_TYPE>(); }

    TEST(int32_t, int32_t);
    TEST(int32_t, int64_t);
    TEST(int64_t, int32_t);
    TEST(int64_t, int64_t);

#undef TEST
}

TEST_CASE("BPlusTree/ISAM", "[milestone2]")
{
    using btree_type = BPlusTree<int32_t, int32_t>;

    SECTION("empty")
    {
        std::array<typename btree_type::value_type, 0> data;
        auto tree = btree_type::Bulkload(data);

        auto leaf_it = tree.leaves_begin();
        REQUIRE(leaf_it != tree.leaves_end());
        CHECK(leaf_it->next() == nullptr);
        CHECK(leaf_it->empty());

        auto entry_it = leaf_it->begin();
        CHECK(entry_it == leaf_it->end());

        ++leaf_it;
        CHECK(leaf_it == tree.leaves_end());
    }

    SECTION("one")
    {
        std::array<typename btree_type::value_type, 1> data = { {
            { 42, 13 },
        } };
        auto tree = btree_type::Bulkload(data);

        auto leaf_it = tree.leaves_begin();
        REQUIRE(leaf_it != tree.leaves_end());
        CHECK(leaf_it->next() == nullptr);
        REQUIRE_FALSE(leaf_it->empty());

        auto entry_it = leaf_it->begin();
        REQUIRE(entry_it != leaf_it->end());
        CHECK(entry_it->first == 42);
        CHECK(entry_it->second == 13);
        ++entry_it;
        CHECK(entry_it == leaf_it->end());

        ++leaf_it;
        CHECK(leaf_it == tree.leaves_end());
    }

    SECTION("consecutive")
    {
        std::vector<typename btree_type::value_type> data;
        for (typename btree_type::key_type i = 0; i != 100; ++i) {
            data.emplace_back(i, i*i);
        }
        auto tree = btree_type::Bulkload(data);

        typename btree_type::key_type runner = 0;
        for (auto leaf_it = tree.leaves_begin(), leaf_end = tree.leaves_end(); leaf_it != leaf_end; ++leaf_it) {
            for (auto e : *leaf_it) {
                REQUIRE(e.first == runner);
                CHECK(e.second == runner * runner);
                ++runner;
            }
        }
    }
}

#include "catch.hpp"

#include "MyPlanEnumerator.hpp"
#include <memory>
#include <sstream>
#include <string>


using namespace m;


void setup_tables()
{
    auto &C = Catalog::Get();
    auto &DB = C.add_database(C.pool("test_db"));
    C.set_database_in_use(DB);

    /* Create 20 tables with foreign keys to all other tables. */
    std::ostringstream oss;
    for (unsigned i = 0; i != 20; ++i) {
        oss.str("");
        oss << "T" << i;
        Table &T = DB.add_table(C.pool(oss.str().c_str()));
        T.push_back(C.pool("id"),  Type::Get_Integer(Type::TY_Vector, 4));

        for (unsigned j = 0; j != 20; ++j) {
            if (j == i) continue;
            oss.str("");
            oss << "fid_T" << j;
            T.push_back(C.pool(oss.str().c_str()), Type::Get_Integer(Type::TY_Vector, 4));
        }
    }
}

PlanTable get_plan_table(const QueryGraph &G)
{
    static constexpr std::size_t NUM_ROWS[20] = {
        5, 10, 8, 12,
        3, 4, 7, 20,
        1, 19, 8, 10,
        10, 13, 12, 7,
        20, 18, 5, 17
    };
    const std::size_t num_sources = G.sources().size();
    PlanTable PT(num_sources);
    for (auto ds : G.sources()) {
        QueryGraph::Subproblem s;
        s.set(ds->id());
        PT[s].cost = 0;
        PT[s].size = NUM_ROWS[ds->id()];
    }
    return PT;
}

void CHECK_PLANTABLES_EQUAL(const PlanTable &expected, const PlanTable &actual)
{
    REQUIRE(expected.num_sources() == actual.num_sources());

    for (SmallBitset s(1), end(1UL << expected.num_sources()); s != end; s = SmallBitset(uint64_t(s) + 1)) {
        auto &expected_entry = expected[s];
        auto &actual_entry = actual[s];

        SmallBitset expected_left, expected_right, actual_left, actual_right;
        if (uint64_t(expected_entry.left) < uint64_t(expected_entry.right)) {
            expected_left = expected_entry.left;
            expected_right = expected_entry.right;
        } else {
            expected_left = expected_entry.right;
            expected_right = expected_entry.left;
        }

        if (uint64_t(actual_entry.left) < uint64_t(actual_entry.right)) {
            actual_left = actual_entry.left;
            actual_right = actual_entry.right;
        } else {
            actual_left = actual_entry.right;
            actual_right = actual_entry.left;
        }


        CHECK(expected_entry.cost == actual_entry.cost);
        CHECK(expected_left == actual_left);
        CHECK(expected_right == actual_right);
    }
}


/*======================================================================================================================
 * TEST CASES
 *====================================================================================================================*/

TEST_CASE("MyPlanEnumerator/1", "[milestone3]")
{
    using Subproblem = QueryGraph::Subproblem;
    Catalog::Clear();
    Catalog &C = Catalog::Get();
    std::ostringstream out, err;
    Diagnostic diag(false, out, err);

    setup_tables();
    const std::string query_str = "SELECT * FROM T0;";
    auto stmt = m::statement_from_string(diag, query_str);
    auto G = QueryGraph::Build(*stmt);

    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });

    /* Define expected plan table */
    PlanTable expected_plan_table(1);
    expected_plan_table.at(Subproblem(1)) = { Subproblem(0), Subproblem(0), 5, 0 }; // T0

    /* Run MyPlanEnumerator. */
    auto PT = get_plan_table(*G);
    MyPlanEnumerator{}(*G, CF, PT);
    CHECK_PLANTABLES_EQUAL(expected_plan_table, PT);
}

TEST_CASE("MyPlanEnumerator/2", "[milestone3]")
{
    using Subproblem = QueryGraph::Subproblem;
    Catalog::Clear();
    Catalog &C = Catalog::Get();
    std::ostringstream out, err;
    Diagnostic diag(false, out, err);

    setup_tables();
    const std::string query_str = "\
SELECT * \
FROM T0, T1 \
WHERE T0.id = T1.fid_T0;";
    auto stmt = m::statement_from_string(diag, query_str);
    auto G = QueryGraph::Build(*stmt);

    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });

    /* Define expected plan table */
    PlanTable expected_plan_table(2);
    expected_plan_table.at(Subproblem(1)) = { Subproblem(0), Subproblem(0),  5,  0 }; // T0
    expected_plan_table.at(Subproblem(2)) = { Subproblem(0), Subproblem(0), 10,  0 }; // T1
    expected_plan_table.at(Subproblem(3)) = { Subproblem(1), Subproblem(2), 50, 15 }; // T0 ⋈  T1

    /* Run MyPlanEnumerator. */
    auto PT = get_plan_table(*G);
    MyPlanEnumerator{}(*G, CF, PT);
    CHECK_PLANTABLES_EQUAL(expected_plan_table, PT);
}

TEST_CASE("MyPlanEnumerator/3-chain", "[milestone3]")
{
    using Subproblem = QueryGraph::Subproblem;
    Catalog::Clear();
    Catalog &C = Catalog::Get();
    std::ostringstream out, err;
    Diagnostic diag(false, out, err);

    setup_tables();
    const std::string query_str = "\
SELECT * \
FROM T0, T1, T2 \
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1;";
    auto stmt = m::statement_from_string(diag, query_str);
    auto G = QueryGraph::Build(*stmt);

    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });

    /* Define expected plan table */
    PlanTable expected_plan_table(3);
    expected_plan_table.at(Subproblem(1)) = { Subproblem(0), Subproblem(0),   5,  0 }; // T0
    expected_plan_table.at(Subproblem(2)) = { Subproblem(0), Subproblem(0),  10,  0 }; // T1
    expected_plan_table.at(Subproblem(3)) = { Subproblem(1), Subproblem(2),  50, 15 }; // T0 ⋈  T1
    expected_plan_table.at(Subproblem(4)) = { Subproblem(0), Subproblem(0),   8,  0 }; // T2
    expected_plan_table.at(Subproblem(6)) = { Subproblem(2), Subproblem(4),  80, 18 }; // T1 ⋈  T2
    expected_plan_table.at(Subproblem(7)) = { Subproblem(3), Subproblem(4), 400, 73 }; // (T0 ⋈  T1) ⋈  T2

    /* Run MyPlanEnumerator. */
    auto PT = get_plan_table(*G);
    MyPlanEnumerator{}(*G, CF, PT);
    CHECK_PLANTABLES_EQUAL(expected_plan_table, PT);
}

TEST_CASE("MyPlanEnumerator/3-cycle", "[milestone3]")
{
    using Subproblem = QueryGraph::Subproblem;
    Catalog::Clear();
    Catalog &C = Catalog::Get();
    std::ostringstream out, err;
    Diagnostic diag(false, out, err);

    setup_tables();
    const std::string query_str = "\
SELECT * \
FROM T0, T1, T2 \
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1 AND T0.id = T2.fid_T0;";
    auto stmt = m::statement_from_string(diag, query_str);
    auto G = QueryGraph::Build(*stmt);

    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });

    /* Define expected plan table */
    PlanTable expected_plan_table(3);
    expected_plan_table.at(Subproblem(1)) = { Subproblem(0), Subproblem(0),   5,  0 }; // T0
    expected_plan_table.at(Subproblem(2)) = { Subproblem(0), Subproblem(0),  10,  0 }; // T1
    expected_plan_table.at(Subproblem(3)) = { Subproblem(1), Subproblem(2),  50, 15 }; // T0 ⋈  T1
    expected_plan_table.at(Subproblem(4)) = { Subproblem(0), Subproblem(0),   8,  0 }; // T2
    expected_plan_table.at(Subproblem(5)) = { Subproblem(1), Subproblem(4),  40, 13 }; // T0 ⋈  T2
    expected_plan_table.at(Subproblem(6)) = { Subproblem(2), Subproblem(4),  80, 18 }; // T1 ⋈  T2
    expected_plan_table.at(Subproblem(7)) = { Subproblem(2), Subproblem(5), 400, 63 }; // T1 ⋈  (T0 ⋈  T2)

    /* Run MyPlanEnumerator. */
    auto PT = get_plan_table(*G);
    MyPlanEnumerator{}(*G, CF, PT);
    CHECK_PLANTABLES_EQUAL(expected_plan_table, PT);
}

TEST_CASE("MyPlanEnumerator/4-chain", "[milestone3]")
{
    using Subproblem = QueryGraph::Subproblem;
    Catalog::Clear();
    Catalog &C = Catalog::Get();
    std::ostringstream out, err;
    Diagnostic diag(false, out, err);

    setup_tables();
    const std::string query_str = "\
SELECT * \
FROM T0, T1, T2, T3 \
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1 AND T2.id = T3.fid_T2;";
    auto stmt = m::statement_from_string(diag, query_str);
    auto G = QueryGraph::Build(*stmt);

    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });

    /* Define expected plan table */
    PlanTable expected_plan_table(4);
    expected_plan_table.at(Subproblem(1))  = { Subproblem(0),  Subproblem(0),    5,   0 }; // T0
    expected_plan_table.at(Subproblem(2))  = { Subproblem(0),  Subproblem(0),   10,   0 }; // T1
    expected_plan_table.at(Subproblem(3))  = { Subproblem(1),  Subproblem(2),   50,  15 }; // T0 ⋈  T1
    expected_plan_table.at(Subproblem(4))  = { Subproblem(0),  Subproblem(0),    8,   0 }; // T2
    expected_plan_table.at(Subproblem(6))  = { Subproblem(2),  Subproblem(4),   80,  18 }; // T1 ⋈  T2
    expected_plan_table.at(Subproblem(7))  = { Subproblem(3),  Subproblem(4),  400,  73 }; // (T0 ⋈  T1) ⋈  T2
    expected_plan_table.at(Subproblem(8))  = { Subproblem(0),  Subproblem(0),   12,   0 }; // T3
    expected_plan_table.at(Subproblem(12)) = { Subproblem(4),  Subproblem(8),   96,  20 }; // T2 ⋈  T3
    expected_plan_table.at(Subproblem(14)) = { Subproblem(6),  Subproblem(8),  960, 110 }; // (T1 ⋈  T2) ⋈  T3
    expected_plan_table.at(Subproblem(15)) = { Subproblem(3), Subproblem(12), 4800, 181 }; // (T0 ⋈  T1) ⋈  (T2 ⋈  T3)

    /* Run MyPlanEnumerator. */
    auto PT = get_plan_table(*G);
    MyPlanEnumerator{}(*G, CF, PT);
    CHECK_PLANTABLES_EQUAL(expected_plan_table, PT);
}

TEST_CASE("MyPlanEnumerator/4-cycle", "[milestone3]")
{
    using Subproblem = QueryGraph::Subproblem;
    Catalog::Clear();
    Catalog &C = Catalog::Get();
    std::ostringstream out, err;
    Diagnostic diag(false, out, err);

    setup_tables();
    const std::string query_str = "\
SELECT * \
FROM T0, T1, T2, T3 \
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1 AND T2.id = T3.fid_T2 AND T0.id = T3.fid_T0;";
    auto stmt = m::statement_from_string(diag, query_str);
    auto G = QueryGraph::Build(*stmt);

    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });

    /* Define expected plan table */
    PlanTable expected_plan_table(4);
    expected_plan_table.at(Subproblem(1))  = { Subproblem(0), Subproblem(0),    5,   0 }; // T0
    expected_plan_table.at(Subproblem(2))  = { Subproblem(0), Subproblem(0),   10,   0 }; // T1
    expected_plan_table.at(Subproblem(3))  = { Subproblem(1), Subproblem(2),   50,  15 }; // T0 ⋈  T1
    expected_plan_table.at(Subproblem(4))  = { Subproblem(0), Subproblem(0),    8,   0 }; // T2
    expected_plan_table.at(Subproblem(6))  = { Subproblem(2), Subproblem(4),   80,  18 }; // T1 ⋈  T2
    expected_plan_table.at(Subproblem(7))  = { Subproblem(3), Subproblem(4),  400,  73 }; // (T0 ⋈  T1) ⋈  T2
    expected_plan_table.at(Subproblem(8))  = { Subproblem(0), Subproblem(0),   12,   0 }; // T3
    expected_plan_table.at(Subproblem(9))  = { Subproblem(1), Subproblem(8),   60,  17 }; // T0 ⋈  T3
    expected_plan_table.at(Subproblem(11)) = { Subproblem(3), Subproblem(8),  600,  77 }; // (T0 ⋈  T1) ⋈  T3
    expected_plan_table.at(Subproblem(12)) = { Subproblem(4), Subproblem(8),   96,  20 }; // T2 ⋈  T3
    expected_plan_table.at(Subproblem(13)) = { Subproblem(4), Subproblem(9),  480,  85 }; // T2 ⋈  (T0 ⋈  T3)
    expected_plan_table.at(Subproblem(14)) = { Subproblem(6), Subproblem(8),  960, 110 }; // (T1 ⋈  T2) ⋈  T3
    expected_plan_table.at(Subproblem(15)) = { Subproblem(6), Subproblem(9), 4800, 175 }; // (T1 ⋈  T2) ⋈  (T0 ⋈  T3)

    /* Run MyPlanEnumerator. */
    auto PT = get_plan_table(*G);
    MyPlanEnumerator{}(*G, CF, PT);
    CHECK_PLANTABLES_EQUAL(expected_plan_table, PT);
}

TEST_CASE("MyPlanEnumerator/cost-overflow", "[milestone3]")
{
    using Subproblem = QueryGraph::Subproblem;
    Catalog::Clear();
    Catalog &C = Catalog::Get();
    std::ostringstream out, err;
    Diagnostic diag(false, out, err);

    setup_tables();

    std::string query_str;
    Subproblem T0, T1, T2;

    SECTION("T0 T1 T2")
    {
        query_str = "\
SELECT * \
FROM T0, T1, T2 \
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1;";
        T0 = Subproblem(1);
        T1 = Subproblem(2);
        T2 = Subproblem(4);
    }

    SECTION("T0 T2 T1")
    {
        query_str = "\
SELECT * \
FROM T0, T2, T1 \
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1;";
        T0 = Subproblem(1);
        T2 = Subproblem(2);
        T1 = Subproblem(4);
    }

    SECTION("T1 T0 T2")
    {
        query_str = "\
SELECT * \
FROM T1, T0, T2 \
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1;";
        T1 = Subproblem(1);
        T0 = Subproblem(2);
        T2 = Subproblem(4);
    }

    auto stmt = m::statement_from_string(diag, query_str);
    auto G = QueryGraph::Build(*stmt);

    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });

    /* Define subproblems. */
    const Subproblem T0xT2(T0 | T2);
    const Subproblem All(T0 | T1 | T2);

    /* Initialize plan table. */
    PlanTable PT(3);
    PT[T0].cost = 0;
    PT[T0].size = (1UL << 63) + 42;
    PT[T1].cost = 0;
    PT[T1].size = (1UL << 63) + 1337;
    PT[T2].cost = 0;
    PT[T2].size = (1UL << 63) + 314;

    /* Run MyPlanEnumerator. */
    MyPlanEnumerator{}(*G, CF, PT);

    CHECK_FALSE(PT.has_plan(T0xT2)); // infeasible subproblem
    CHECK((PT[All].left != T0xT2 and PT[All].right != T0xT2)); // infeasible subproblem part of plan
}

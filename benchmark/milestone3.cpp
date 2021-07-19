#include "MyPlanEnumerator.hpp"
#include <chrono>
#include <memory>
#include <mutable/mutable.hpp>
#include <sstream>
#include <utility>


using namespace m;


static constexpr std::size_t NUM_TABLES = 20;
static constexpr std::size_t NUM_RUNS = 3;


/* Create a clique of tables. */
void setup_tables()
{
    auto &C = Catalog::Get();
    auto &DB = C.add_database(C.pool("dbsys20"));
    C.set_database_in_use(DB);

    /* Create 20 tables with foreign keys to all other tables. */
    std::ostringstream oss;
    for (unsigned i = 0; i != NUM_TABLES; ++i) {
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

        T.store(C.create_store(T));
    }
}

/* Create and initialize a plan table. */
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

/* Generate chain query with `n` relations. */
std::string gen_chain_query(std::size_t n)
{
    std::ostringstream oss;

    oss << "SELECT *\nFROM ";
    for (std::size_t i = 0; i != n; ++i) {
        if (i != 0) oss << ", ";
        oss << "T" << i;
    }
    oss << "\nWHERE\n    ";
    for (std::size_t i = 0; i != n-1; ++i) {
        if (i != 0) oss << " AND\n    ";
        oss << "T" << i << ".id = T" << (i+1) << ".fid_T" << i;
    }
    oss << ";\n";
    return oss.str();
}

/* Generate cycle query with `n` relations. */
std::string gen_cycle_query(std::size_t n)
{
    std::ostringstream oss;

    oss << "SELECT *\nFROM ";
    for (std::size_t i = 0; i != n; ++i) {
        if (i != 0) oss << ", ";
        oss << "T" << i;
    }
    oss << "\nWHERE\n    ";
    for (std::size_t i = 0; i != n-1; ++i) {
        oss << "T" << i << ".id = T" << (i+1) << ".fid_T" << i << " AND\n    ";
    }
    oss << "T" << (n-1) << ".id = T0.fid_T" << (n-1) << ";\n";
    return oss.str();
}

/* Generate star query with `n` relations. */
std::string gen_star_query(std::size_t n)
{
    std::ostringstream oss;

    oss << "SELECT *\nFROM ";
    for (std::size_t i = 0; i != n; ++i) {
        if (i != 0) oss << ", ";
        oss << "T" << i;
    }
    oss << "\nWHERE\n    ";
    for (std::size_t i = 1; i != n; ++i) {
        if (i != 1) oss << " AND\n    ";
        oss << "T0.fid_T" << i << " = T" << i << ".id";
    }
    oss << ";\n";
    return oss.str();
}

/* Generate clique query with `n` relations. */
std::string gen_clique_query(std::size_t n)
{
    std::ostringstream oss;

    oss << "SELECT *\nFROM ";
    for (std::size_t i = 0; i != n; ++i) {
        if (i != 0) oss << ", ";
        oss << "T" << i;
    }
    oss << "\nWHERE\n    ";
    for (std::size_t i = 0; i != n-1; ++i) {
        if (i != 0) oss << " AND\n    ";
        for (std::size_t j = i + 1; j != n; ++j) {
            if (j != i + 1) oss << " AND ";
            oss << "T" << i << ".id = T" << j << ".fid_T" << i;
        }
    }
    oss << ";\n";
    return oss.str();
}

/* Benchmark plan enumeration for the given query.  Report median of NUM_RUNS runs. */
void benchmark(const CostFunction &CF, const PlanEnumerator &PE, const std::string &name, const QueryGraph &G)
{
    using namespace std::chrono;

    std::vector<steady_clock::duration> times;
    times.reserve(NUM_RUNS);

    for (std::size_t i = 0; i != NUM_RUNS; ++i) {
        auto PT = get_plan_table(G);
        auto t_begin = steady_clock::now();
        PE(G, CF, PT);
        auto t_end = steady_clock::now();
        times.push_back(t_end - t_begin);
    }
    std::sort(times.begin(), times.end());
    auto median = (times[(times.size() - 1) / 2] + times[times.size() / 2]) / 2;

    std::cout << "milestone3," << name << ',' << duration_cast<microseconds>(median).count() << '\n';
}

int main(int argc, char**)
{
    /* Check the number of parameters. */
    if (argc != 1) {
        std::cerr << "Unexpected parameters." << std::endl;
        exit(EXIT_FAILURE);
    }

    /* Setup. */
    setup_tables();
    std::ostringstream out, err;
    Diagnostic diag(true, out, err);
    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });
    MyPlanEnumerator PE;
    std::ostringstream oss;

#define BENCHMARK(KIND) \
    for (std::size_t n : {5, 10, 15, 18}) { \
        auto query = gen_##KIND##_query(n); \
        auto stmt = statement_from_string(diag, query); \
        auto G = QueryGraph::Build(*stmt); \
        oss.str(""); \
        oss << #KIND "-" << n; \
        benchmark(CF, PE, oss.str(), *G); \
    }

    BENCHMARK(chain);
    BENCHMARK(cycle);
    BENCHMARK(star);
    BENCHMARK(clique);

#undef BENCHMARK
}

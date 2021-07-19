/*
Checks the implementation of the query optimiser
*/

#include "MyPlanEnumerator.hpp"
#include <iostream>
#include <memory>
#include <mutable/mutable.hpp>
#include <sstream>
#include <utility>


using namespace m;


static constexpr std::size_t NUM_TABLES = 5;


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

PlanTable get_plan_table(const QueryGraph &G)
{
    static constexpr std::size_t NUM_ROWS[NUM_TABLES] = { 5, 10, 8, 12, 3 };
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

int main(int argc, char**)
{
    /* Check the number of parameters. */
    if (argc != 1) {
        std::cerr << "Unexpected parameters." << std::endl;
        exit(EXIT_FAILURE);
    }

    setup_tables();

    /* Create a `Diagnostic` object. */
    Diagnostic diag(true, std::cout, std::cerr);

    /* Read input until first semicolon. */
    std::ostringstream oss;
    {
        char c;
        while (std::cin.get(c)) {
            oss << c;
            if (c == ';') break;
        }
    }

    /* Convert input to query. */
    auto stmt = statement_from_string(diag, oss.str());
    auto G = QueryGraph::Build(*stmt);

    {
        DotTool dot(diag);
        G->dot(dot.stream());
        dot.show("graph", true, "fdp");
    }

    /* Define cost function and optimizer. */
    CostFunction CF([](CostFunction::Subproblem left, CostFunction::Subproblem right, int, const PlanTable &T) {
        return sum_wo_overflow(T[left].cost, T[right].cost, T[left].size, T[right].size);
    });
    MyPlanEnumerator PE;
    Optimizer O(PE, CF);

    /* Create base plans. */
    auto PT = get_plan_table(*G);
    Producer **sources = new Producer*[G->sources().size()];
    for (auto ds : G->sources()) {
        auto bt = as<const BaseTable>(ds);
        sources[ds->id()] = new ScanOperator(bt->table().store(), bt->alias());
    }

    /* Perform optimization. */
    O.optimize_locally(*G, PT);

    /* Construct the plan for the query. */
    auto plan = O.construct_plan(*G, PT, sources);
    plan->minimize_schema();

    /* Show the plan and plan table. */
    plan->dump(std::cout);
    PT.dump(std::cout);

    {
        DotTool dot(diag);
        plan->dot(dot.stream());
        dot.show("plan", true);
    }

    delete[] sources;
}

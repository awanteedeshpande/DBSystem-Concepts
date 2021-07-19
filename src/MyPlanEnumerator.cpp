#include "MyPlanEnumerator.hpp"

using namespace m;

static std::vector<QueryGraph::Subproblem> all_subsets(std::vector<size_t> sources, const AdjacencyMatrix &M) {
    std::vector<QueryGraph::Subproblem> ss;
    for (int i = 1; i < pow(2, sources.size()); i++) {
        SmallBitset problems(i);
        QueryGraph::Subproblem subproblem;
        for (size_t x = 0; x < sources.size(); x++) {
            if (problems.contains(x)) subproblem.set(sources[x]);
        }
        if (M.is_connected(subproblem)) ss.push_back(subproblem);
    }
    return ss;
}

static std::vector<QueryGraph::Subproblem> subsets(std::vector<size_t> source, size_t subsetSize, const AdjacencyMatrix &M) {
    std::vector<QueryGraph::Subproblem> ss;
    for (QueryGraph::Subproblem sp : all_subsets(source, M)) {
        if (sp.size() == subsetSize && M.is_connected(sp)) {
            ss.push_back(sp);
        }
    }
    return ss;
}

static std::vector<QueryGraph::Subproblem> subsets(QueryGraph::Subproblem source, const AdjacencyMatrix &M) {
    std::vector<size_t> active;
    for (size_t i = 0; i < source.capacity(); i++) {
        if (source.contains(i)) active.push_back(i);
    }
    auto res = all_subsets(active, M);
    return res;
}

void MyPlanEnumerator::operator()(const QueryGraph &G, const CostFunction &CF, PlanTable &PT) const
{
    //DPsub(G, CF, PT);
    const AdjacencyMatrix M(G); // compute the adjacency matrix for graph G

    // Implement an algorithm for plan enumeration
    int n = G.sources().size();

    // get the relation ids
    std::vector<size_t> sourceIds;
    for (auto ds : G.sources()) {
        sourceIds.push_back(ds->id());
    }

    // go through all subplans of size 2 to size n
    for(int planSize = 2; planSize <= n; planSize++) {
        // inspect each subset S of size planSize
        std::vector<QueryGraph::Subproblem> S = subsets(sourceIds, planSize, M);
        for(auto subsetS : S) {
            // inspect each subset O of S
            std::vector<QueryGraph::Subproblem> O = subsets(subsetS, M);
            for(auto subsetO : O) {
                // diff = subsetS\subsetO
                QueryGraph::Subproblem diff(subsetS & ~subsetO);
                // merge subsetO and diff together (use PlanTable::update(subsetO, diff))

                if(PT.has_plan(subsetO) && PT.has_plan(diff)) {
                    if(M.is_connected(subsetO | diff)) PT.update(CF, subsetO, diff, 0);
                }
            }
        }
    }
}

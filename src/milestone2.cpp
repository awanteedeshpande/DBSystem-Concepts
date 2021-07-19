/*
Checks the implementation of the B+ Tree
*/

#include "BPlusTree.hpp"
#include <memory>
#include <mutable/mutable.hpp>
#include <utility>


int main(int argc, char **argv)
{
    /* Check the number of parameters. */
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <CSV-File> <SIZE-MIN> <SIZE-MAX>" << std::endl;
        exit(EXIT_FAILURE);
    }

    const int64_t size_min = strtol(argv[2], nullptr, 10);
    const int64_t size_max = strtol(argv[3], nullptr, 10);

    if (size_min > size_max) {
        std::cerr << "SIZE-MIN must not be greater than SIZE-MAX" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* Get a handle on the catalog. */
    auto &C = m::Catalog::Get();

    /* Create a `m::Diagnostic` object. */
    m::Diagnostic diag(true, std::cout, std::cerr);

    /* Create database 'dbsys20' and select it. */
    auto &DB = C.add_database(C.pool("dbsys20"));
    C.set_database_in_use(DB);

    /* Create table 'packages'. */
    auto &T = DB.add_table(C.pool("packages"));
    T.push_back(C.pool("id"),           m::Type::Get_Integer(m::Type::TY_Vector, 4));
    T.push_back(C.pool("repo"),         m::Type::Get_Char(m::Type::TY_Vector, 10));
    T.push_back(C.pool("pkg_name"),     m::Type::Get_Char(m::Type::TY_Vector, 32));
    T.push_back(C.pool("pkg_ver"),      m::Type::Get_Char(m::Type::TY_Vector, 20));
    T.push_back(C.pool("description"),  m::Type::Get_Char(m::Type::TY_Vector, 80));
    T.push_back(C.pool("licenses"),     m::Type::Get_Char(m::Type::TY_Vector, 32));
    T.push_back(C.pool("size"),         m::Type::Get_Integer(m::Type::TY_Vector, 8));
    T.push_back(C.pool("packager"),     m::Type::Get_Char(m::Type::TY_Vector, 32));

    /* Back the table with our store. */
    T.store(C.create_store(T));

    /* Load CSV file into table 'T'. */
    m::load_from_CSV(diag, T, argv[1], std::numeric_limits<std::size_t>::max(), true, false);

    if (diag.num_errors())
        exit(EXIT_FAILURE);

    /* Collect all (size,id) pairs. */
    std::vector<std::pair<int64_t, int32_t>> size2id;

    /* Query the table for all package sizes. */
    auto stmt = m::statement_from_string(diag, "SELECT size, id FROM packages;");
    std::unique_ptr<m::SelectStmt> query(static_cast<m::SelectStmt*>(stmt.release()));

    /* Insert all package sizes as (size,id) pairs into `size2id`. */
    auto callback = std::make_unique<m::CallbackOperator>([&](const m::Schema&, const m::Tuple &t) {
        int64_t size = t[0].as_i();
        int32_t id = t[1].as_i();
        size2id.emplace_back(size, id);
    });
    m::execute_query(diag, *query, std::move(callback));

    /* Sort all (size,id) pairs by size. */
    std::sort(size2id.begin(), size2id.end(), [](auto left, auto right) { return left.first < right.first; });

    /* Bulkload the sorted (size,id) pairs into a B+-tree. */
    using tree_type = BPlusTree<int64_t, int32_t>;
    auto btree = tree_type::Bulkload(size2id);

    /* Query the B+-tree for packages with a size between SIZE-MIN and SIZE-MAX. */
    for (auto elem : btree.in_range(size_min, size_max))
        std::cout << "Package with id " << elem.second << " is " << elem.first << " bytes.\n";
}

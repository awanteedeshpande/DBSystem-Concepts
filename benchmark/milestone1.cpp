#include "RowStore.hpp"
#include "ColumnStore.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <mutable/util/macro.hpp>


namespace {

#define store_t(X) X(row), X(column),
DECLARE_ENUM(store_t);
const char *store2str[] = { ENUM_TO_STR(store_t) };
#undef STORE

#ifndef NDEBUG
constexpr int32_t NUM_TUPLES_RW = 2e6;
#else
constexpr int32_t NUM_TUPLES_RW = 2e7;
#endif

}

void benchmark_store(store_t st)
{
    /* Clear the catalog before starting a new benchmark. */
    m::Catalog::Clear();

    /* Get a handle on the catalog. */
    auto &C = m::Catalog::Get();

    /* Create a `m::Diagnostic` object. */
    m::Diagnostic diag(true, std::cout, std::cerr);

    /* Register our store and set as default store. */
    if (st == store_t::row) {
        C.register_store<RowStore>(C.pool("MyRowStore"));
        C.default_store(C.pool("MyRowStore"));
    } else if (st == store_t::column) {
        C.register_store<ColumnStore>(C.pool("MyColumnStore"));
        C.default_store(C.pool("MyColumnStore"));
    } else {
        assert(false and "invalid store");
    }

    /* Create database 'dbsys20' and select it. */
    auto &DB = C.add_database(C.pool("dbsys20"));
    C.set_database_in_use(DB);

    /* Evaluate memory layout. */
    {
        /* Create a wide table to evaluate padding and alignment. */
        auto &tbl_wide = DB.add_table(C.pool("wide"));
        tbl_wide.push_back(C.pool("a_i4"),     m::Type::Get_Integer(m::Type::TY_Vector, 4));
        tbl_wide.push_back(C.pool("b_b"),      m::Type::Get_Boolean(m::Type::TY_Vector));
        tbl_wide.push_back(C.pool("c_c3"),     m::Type::Get_Char(m::Type::TY_Vector, 3));
        tbl_wide.push_back(C.pool("d_b"),      m::Type::Get_Boolean(m::Type::TY_Vector));
        tbl_wide.push_back(C.pool("e_d"),      m::Type::Get_Double(m::Type::TY_Vector));
        tbl_wide.push_back(C.pool("f_i1"),     m::Type::Get_Integer(m::Type::TY_Vector, 1));
        tbl_wide.push_back(C.pool("g_f"),      m::Type::Get_Float(m::Type::TY_Vector));
        tbl_wide.push_back(C.pool("h_c5"),     m::Type::Get_Char(m::Type::TY_Vector, 5));
        tbl_wide.push_back(C.pool("i_b"),      m::Type::Get_Boolean(m::Type::TY_Vector));
        tbl_wide.push_back(C.pool("j_i2"),     m::Type::Get_Integer(m::Type::TY_Vector, 2));
        tbl_wide.push_back(C.pool("k_b"),      m::Type::Get_Boolean(m::Type::TY_Vector));
        tbl_wide.push_back(C.pool("l_i2"),     m::Type::Get_Integer(m::Type::TY_Vector, 2));
        tbl_wide.store(C.create_store(tbl_wide));

        auto &store = tbl_wide.store();
        auto &lin = store.linearization();

        /* Sum up the stride of the sequences in the root `linearization`. */
        std::size_t bits_per_row = 0;
        for (auto seq : lin) {
            if (seq.is_linearization()) {
                auto &l = seq.as_linearization();
                bits_per_row += seq.stride * 8 / l.num_tuples();
            } else {
                bits_per_row += seq.stride;
            }
        }

        std::cout << "milestone1," << store2str[st] << ",size," << bits_per_row << '\n';
    }

    /* Evaluate read/write performance. */
    {
        /* Create a simple table to evaluate read/write performance. */
        auto &tbl_short = DB.add_table(C.pool("short"));
        tbl_short.push_back(C.pool("id_a"),     m::Type::Get_Integer(m::Type::TY_Vector, 4));
        tbl_short.push_back(C.pool("id_b"),     m::Type::Get_Integer(m::Type::TY_Vector, 4));
        tbl_short.store(C.create_store(tbl_short));

        /* Get a handle on the backing store, create a writer, and an I/O tuple. */
        auto &store = tbl_short.store();
        m::StoreWriter W(store);
        m::Tuple tup(W.schema());

        using namespace std::chrono;

        auto t_write_begin = steady_clock::now();
        for (int32_t i = 0; i != NUM_TUPLES_RW; ++i) {
            /* Set tuple data (i, 2*i). */
            tup.set(0, i);
            tup.set(1, i<<1);
            W.append(tup);
        }
        auto t_write_end = steady_clock::now();

        auto stmt = m::statement_from_string(diag, "SELECT id_a, id_b FROM short;");
        std::unique_ptr<m::SelectStmt> query(static_cast<m::SelectStmt*>(stmt.release()));

        auto op = std::make_unique<m::CallbackOperator>([](const m::Schema&, const m::Tuple&){});

        auto t_read_begin = steady_clock::now();
        m::execute_query(diag, *query, std::move(op));
        auto t_read_end = steady_clock::now();

        std::cout << "milestone1," << store2str[st] << ",write," << duration_cast<milliseconds>(t_write_end - t_write_begin).count() << '\n'
                  << "milestone1," << store2str[st] << ",read," << duration_cast<milliseconds>(t_read_end - t_read_begin).count() << '\n';
    }
}

int main()
{
    benchmark_store(store_t::row);
    benchmark_store(store_t::column);
}

#include "catch.hpp"

#include "ColumnStore.hpp"
#include <mutable/mutable.hpp>
#include <sstream>
#include <utility>


TEST_CASE("ColumnStore/c'tor", "[milestone1]")
{
    m::Catalog::Clear();
    auto &C = m::Catalog::Get();
    auto &DB = C.add_database(C.pool("test_db"));
    auto &table = DB.add_table(C.pool("test"));
    table.push_back(C.pool("a"), m::Type::Get_Integer(m::Type::TY_Vector, 4));
    table.store(std::make_unique<ColumnStore>(table));
    auto &lin = table.store().linearization();
    /* Root must be an infinite sequence of rows. */
    CHECK(lin.num_tuples() == 0); // infinite sequence
    REQUIRE(lin.num_sequences() == 2); // attribute 'a' and null bitmap
}

TEST_CASE("ColumnStore/access", "[milestone1]")
{
    m::Catalog::Clear();
    auto &C = m::Catalog::Get();

    auto &DB = C.add_database(C.pool("test_db"));
    auto &table = DB.add_table(C.pool("test"));

    const std::pair<const char*, const m::PrimitiveType*> Attributes[] = {
        { "a_i4",   m::Type::Get_Integer(m::Type::TY_Vector, 4) },
        { "b_f",    m::Type::Get_Float(m::Type::TY_Vector) },
        { "c_i2",   m::Type::Get_Integer(m::Type::TY_Vector, 2) },
        { "d_b",    m::Type::Get_Boolean(m::Type::TY_Vector) },
        { "e_d",    m::Type::Get_Double(m::Type::TY_Vector) },
        { "f_b",    m::Type::Get_Boolean(m::Type::TY_Vector) },
        { "g_c",    m::Type::Get_Char(m::Type::TY_Vector, 7) },
        { "h_b",    m::Type::Get_Boolean(m::Type::TY_Vector) },
        { "i_b",    m::Type::Get_Boolean(m::Type::TY_Vector) },
    };
    for (auto &attr : Attributes)
        table.push_back(C.pool(attr.first), attr.second);

    /* Create and set store. */
    table.store(std::make_unique<ColumnStore>(table));

    /* Process queries. */
    C.set_database_in_use(DB);

    std::ostringstream out, err;
    m::Diagnostic diag(false, out, err);

    {
        auto stmt = m::statement_from_string(diag, "SELECT * FROM test;");
        REQUIRE(diag.num_errors() == 0);
        REQUIRE(err.str().empty());

        std::size_t num_tuples = 0;
        auto callback = std::make_unique<m::CallbackOperator>([&](const m::Schema&, const m::Tuple&) {
            ++num_tuples;
        });

        std::unique_ptr<m::SelectStmt> select_stmt(static_cast<m::SelectStmt*>(stmt.release()));
        m::execute_query(diag, *select_stmt, std::move(callback));
        REQUIRE(diag.num_errors() == 0);
        REQUIRE(err.str().empty());
        REQUIRE(num_tuples == 0);
    }

    {
        auto insertions = m::statement_from_string(diag, "INSERT INTO test VALUES \
                ( 42, 3.14, 1337, TRUE, 2.71828, FALSE, \"female\", TRUE, NULL ), \
                ( NULL, 6.62607015, -137, NULL, 6.241509074, FALSE, NULL, TRUE, FALSE );");
        m::execute_statement(diag, *insertions);

        auto stmt = m::statement_from_string(diag, "SELECT * FROM test;");
        REQUIRE(diag.num_errors() == 0);
        REQUIRE(err.str().empty());

#define IDX(ATTR) S[C.pool(ATTR)].first
#define CHECK_VALUE(ATTR, TYPE, VALUE) \
    REQUIRE_FALSE(T.is_null(IDX(ATTR))); \
    CHECK((VALUE) == T.get(IDX(ATTR)).as_##TYPE())
#define CHECK_CHAR(ATTR, VALUE) \
    REQUIRE_FALSE(T.is_null(IDX(ATTR))); \
    CHECK(std::string(VALUE) == reinterpret_cast<char*>(T.get(IDX(ATTR)).as_p()))

        std::size_t num_tuples = 0;
        auto callback = std::make_unique<m::CallbackOperator>([&](const m::Schema &S, const m::Tuple &T) {
            switch (num_tuples) {
                case 0: {
                    CHECK_VALUE("a_i4", i, 42);
                    CHECK_VALUE("b_f",  f, 3.14f);
                    CHECK_VALUE("c_i2", i, 1337);
                    CHECK_VALUE("d_b",  b, true);
                    CHECK_VALUE("e_d",  d, 2.71828);
                    CHECK_VALUE("f_b",  b, false);
                    CHECK_VALUE("h_b",  b, true);
                    CHECK_CHAR("g_c", "female");
                    CHECK(T.is_null(IDX("i_b")));
                    break;
                }

                case 1: {
                    CHECK(T.is_null(IDX("a_i4")));
                    CHECK_VALUE("b_f",  f, 6.62607015f);
                    CHECK_VALUE("c_i2", i, -137);
                    CHECK(T.is_null(IDX("d_b")));
                    CHECK_VALUE("e_d",  d, 6.241509074);
                    CHECK_VALUE("f_b",  b, false);
                    CHECK(T.is_null(IDX("g_c")));
                    CHECK_VALUE("h_b",  b, true);
                    CHECK_VALUE("i_b", b, false);
                    break;
                }

                default:
                    REQUIRE(false);
            }
            ++num_tuples;
        });

        std::unique_ptr<m::SelectStmt> select_stmt(static_cast<m::SelectStmt*>(stmt.release()));
        m::execute_query(diag, *select_stmt, std::move(callback));
        REQUIRE(diag.num_errors() == 0);
        REQUIRE(err.str().empty());
        REQUIRE(num_tuples == 2);
    }
}

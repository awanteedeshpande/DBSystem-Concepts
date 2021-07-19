#include "catch.hpp"

#include "RowStore.hpp"
#include <mutable/mutable.hpp>
#include <sstream>
#include <utility>


TEST_CASE("RowStore/c'tor", "[milestone1]")
{
    m::Catalog::Clear();
    auto &C = m::Catalog::Get();

    auto &DB = C.add_database(C.pool("test_db"));
    auto &table = DB.add_table(C.pool("test"));

    SECTION("INT(4)")
    {
        table.push_back(C.pool("a"), m::Type::Get_Integer(m::Type::TY_Vector, 4));
        table.store(std::make_unique<RowStore>(table));

        auto &lin = table.store().linearization();

        /* Root must be an infinite sequence of rows. */
        CHECK(lin.num_tuples() == 0); // infinite sequence
        REQUIRE(lin.num_sequences() == 1); // of single row

        /* Root must have a single sequence of a linearization. */
        auto root_it = lin.begin();
        const auto &seq_row = *root_it;
        REQUIRE(seq_row.is_linearization());
        CHECK(seq_row.offset != 0); // address of first row
        CHECK(seq_row.stride == 8); // 4 byte INT, 1 bit null bitmap, padding

        const auto &row = seq_row.as_linearization();
        CHECK(row.num_tuples() == 1); // single row
        REQUIRE(row.num_sequences() == 2); // attribute 'a' and null bitmap

        /* Row must contain attribute 'a' and null bitmap. */
        auto row_it = row.begin();
        const auto &seq_a = *row_it;
        REQUIRE(seq_a.is_attribute());
        auto &a = seq_a.as_attribute();
        CHECK(seq_a.offset == 0); // attribute 'a' is located at the beginning of a row
        CHECK(seq_a.stride == 0); // there is no stride within a row
        CHECK(a.name == C.pool("a"));
        CHECK(a.table.name == C.pool("test"));

        const auto &null_bitmap = *++row_it;
        REQUIRE(null_bitmap.is_null_bitmap());
        CHECK(null_bitmap.offset == 32); // null bitmap located after attribute 'a', that is at offset 32 bits
        CHECK(null_bitmap.stride == 0); // there is no stride within a row
    }

    SECTION("DOUBLE")
    {
        table.push_back(C.pool("a"), m::Type::Get_Double(m::Type::TY_Vector));
        table.store(std::make_unique<RowStore>(table));

        auto &lin = table.store().linearization();

        /* Root must be an infinite sequence of rows. */
        CHECK(lin.num_tuples() == 0); // infinite sequence
        REQUIRE(lin.num_sequences() == 1); // of single row

        /* Root must have a single sequence of a linearization. */
        auto root_it = lin.begin();
        const auto &seq_row = *root_it;
        REQUIRE(seq_row.is_linearization());
        CHECK(seq_row.offset != 0); // address of first row
        CHECK(seq_row.stride == 16); // 8 byte INT, 1 bit null bitmap, padding

        const auto &row = seq_row.as_linearization();
        CHECK(row.num_tuples() == 1); // single row
        REQUIRE(row.num_sequences() == 2); // attribute 'a' and null bitmap

        /* Row must contain attribute 'a' and null bitmap. */
        auto row_it = row.begin();
        const auto &seq_a = *row_it;
        REQUIRE(seq_a.is_attribute());
        auto &a = seq_a.as_attribute();
        CHECK(seq_a.offset == 0); // attribute 'a' is located at the beginning of a row
        CHECK(seq_a.stride == 0); // there is no stride within a row
        CHECK(a.name == C.pool("a"));
        CHECK(a.table.name == C.pool("test"));

        const auto &null_bitmap = *++row_it;
        REQUIRE(null_bitmap.is_null_bitmap());
        CHECK(null_bitmap.offset == 64); // null bitmap located after attribute 'a', that is at offset 64 bits
        CHECK(null_bitmap.stride == 0); // there is no stride within a row
    }

    SECTION("INT(2)")
    {
        table.push_back(C.pool("a"), m::Type::Get_Integer(m::Type::TY_Vector, 2));
        table.store(std::make_unique<RowStore>(table));

        auto &lin = table.store().linearization();

        /* Root must be an infinite sequence of rows. */
        CHECK(lin.num_tuples() == 0); // infinite sequence
        REQUIRE(lin.num_sequences() == 1); // of single row

        /* Root must have a single sequence of a linearization. */
        auto root_it = lin.begin();
        const auto &seq_row = *root_it;
        REQUIRE(seq_row.is_linearization());
        CHECK(seq_row.offset != 0); // address of first row
        CHECK(seq_row.stride == 4); // 2 byte INT, 1 bit null bitmap, padding

        const auto &row = seq_row.as_linearization();
        CHECK(row.num_tuples() == 1); // single row
        REQUIRE(row.num_sequences() == 2); // attribute 'a' and null bitmap

        /* Row must contain attribute 'a' and null bitmap. */
        auto row_it = row.begin();
        const auto &seq_a = *row_it;
        REQUIRE(seq_a.is_attribute());
        auto &a = seq_a.as_attribute();
        CHECK(seq_a.offset == 0); // attribute 'a' is located at the beginning of a row
        CHECK(seq_a.stride == 0); // there is no stride within a row
        CHECK(a.name == C.pool("a"));
        CHECK(a.table.name == C.pool("test"));

        const auto &null_bitmap = *++row_it;
        REQUIRE(null_bitmap.is_null_bitmap());
        CHECK(null_bitmap.offset == 16); // null bitmap located after attribute 'a', that is at offset 16 bits
        CHECK(null_bitmap.stride == 0); // there is no stride within a row
    }

    SECTION("CHAR(4)")
    {
        table.push_back(C.pool("a"), m::Type::Get_Char(m::Type::TY_Vector, 4));
        table.store(std::make_unique<RowStore>(table));

        auto &lin = table.store().linearization();

        /* Root must be an infinite sequence of rows. */
        CHECK(lin.num_tuples() == 0); // infinite sequence
        REQUIRE(lin.num_sequences() == 1); // of single row

        /* Root must have a single sequence of a linearization. */
        auto root_it = lin.begin();
        const auto &seq_row = *root_it;
        REQUIRE(seq_row.is_linearization());
        CHECK(seq_row.offset != 0); // address of first row
        CHECK(seq_row.stride == 5); // 4 byte CHAR(4), 1 bit null bitmap, padding

        const auto &row = seq_row.as_linearization();
        CHECK(row.num_tuples() == 1); // single row
        REQUIRE(row.num_sequences() == 2); // attribute 'a' and null bitmap

        /* Row must contain attribute 'a' and null bitmap. */
        auto row_it = row.begin();
        const auto &seq_a = *row_it;
        REQUIRE(seq_a.is_attribute());
        auto &a = seq_a.as_attribute();
        CHECK(seq_a.offset == 0); // attribute 'a' is located at the beginning of a row
        CHECK(seq_a.stride == 0); // there is no stride within a row
        CHECK(a.name == C.pool("a"));
        CHECK(a.table.name == C.pool("test"));

        const auto &null_bitmap = *++row_it;
        REQUIRE(null_bitmap.is_null_bitmap());
        CHECK(null_bitmap.offset == 32); // null bitmap located after attribute 'a', that is at offset 32 bits
        CHECK(null_bitmap.stride == 0); // there is no stride within a row
    }

    SECTION("BOOL")
    {
        table.push_back(C.pool("a"), m::Type::Get_Boolean(m::Type::TY_Vector));
        table.store(std::make_unique<RowStore>(table));

        auto &lin = table.store().linearization();

        /* Root must be an infinite sequence of rows. */
        CHECK(lin.num_tuples() == 0); // infinite sequence
        REQUIRE(lin.num_sequences() == 1); // of single row

        /* Root must have a single sequence of a linearization. */
        auto root_it = lin.begin();
        const auto &seq_row = *root_it;
        REQUIRE(seq_row.is_linearization());
        CHECK(seq_row.offset != 0); // address of first row
        CHECK(seq_row.stride == 1); // 1 bit BOOL, 1 bit null bitmap, padding

        const auto &row = seq_row.as_linearization();
        CHECK(row.num_tuples() == 1); // single row
        REQUIRE(row.num_sequences() == 2); // attribute 'a' and null bitmap

        /* Row must contain attribute 'a' and null bitmap. */
        auto row_it = row.begin();
        const auto &seq_a = *row_it;
        REQUIRE(seq_a.is_attribute());
        auto &a = seq_a.as_attribute();
        CHECK(seq_a.offset == 0); // attribute 'a' is located at the beginning of a row
        CHECK(seq_a.stride == 0); // there is no stride within a row
        CHECK(a.name == C.pool("a"));
        CHECK(a.table.name == C.pool("test"));

        const auto &null_bitmap = *++row_it;
        REQUIRE(null_bitmap.is_null_bitmap());
        CHECK(null_bitmap.offset == 1); // null bitmap located after attribute 'a', that is at offset 1 bits
        CHECK(null_bitmap.stride == 0); // there is no stride within a row
    }

    SECTION("five booleans")
    {
        table.push_back(C.pool("a"), m::Type::Get_Boolean(m::Type::TY_Vector));
        table.push_back(C.pool("b"), m::Type::Get_Boolean(m::Type::TY_Vector));
        table.push_back(C.pool("c"), m::Type::Get_Boolean(m::Type::TY_Vector));
        table.push_back(C.pool("d"), m::Type::Get_Boolean(m::Type::TY_Vector));
        table.push_back(C.pool("e"), m::Type::Get_Boolean(m::Type::TY_Vector));
        table.store(std::make_unique<RowStore>(table));

        auto &lin = table.store().linearization();

        /* Root must be an infinite sequence of rows. */
        CHECK(lin.num_tuples() == 0); // infinite sequence
        REQUIRE(lin.num_sequences() == 1); // of single row

        /* Root must have a single sequence of a linearization. */
        auto root_it = lin.begin();
        const auto &seq_row = *root_it;
        REQUIRE(seq_row.is_linearization());
        CHECK(seq_row.offset != 0); // address of first row
        CHECK(seq_row.stride == 2); // 5 bit BOOL, 5 bit null bitmap, padding

        const auto &row = seq_row.as_linearization();
        CHECK(row.num_tuples() == 1); // single row
        REQUIRE(row.num_sequences() == 6); // five attributes and null bitmap

        auto row_it = row.begin();
        {
            /* Check attribute 'a'. */
            const auto &seq_a = *row_it++;
            REQUIRE(seq_a.is_attribute());
            auto &a = seq_a.as_attribute();
            CHECK(seq_a.offset == 0);
            CHECK(seq_a.stride == 0); // there is no stride within a row
            CHECK(a.name == C.pool("a"));
            CHECK(a.table.name == C.pool("test"));
        }

        {
            /* Check attribute 'b'. */
            const auto &seq_b = *row_it++;
            REQUIRE(seq_b.is_attribute());
            auto &b = seq_b.as_attribute();
            CHECK(seq_b.offset == 1);
            CHECK(seq_b.stride == 0); // there is no stride within a row
            CHECK(b.name == C.pool("b"));
            CHECK(b.table.name == C.pool("test"));
        }

        {
            /* Check attribute 'c'. */
            const auto &seq_c = *row_it++;
            REQUIRE(seq_c.is_attribute());
            auto &c = seq_c.as_attribute();
            CHECK(seq_c.offset == 2);
            CHECK(seq_c.stride == 0); // there is no stride within a row
            CHECK(c.name == C.pool("c"));
            CHECK(c.table.name == C.pool("test"));
        }

        {
            /* Check attribute 'd'. */
            const auto &seq_d = *row_it++;
            REQUIRE(seq_d.is_attribute());
            auto &d = seq_d.as_attribute();
            CHECK(seq_d.offset == 3);
            CHECK(seq_d.stride == 0); // there is no stride within a row
            CHECK(d.name == C.pool("d"));
            CHECK(d.table.name == C.pool("test"));
        }

        {
            /* Check attribute 'e'. */
            const auto &seq_e = *row_it++;
            REQUIRE(seq_e.is_attribute());
            auto &e = seq_e.as_attribute();
            CHECK(seq_e.offset == 4);
            CHECK(seq_e.stride == 0); // there is no stride within a row
            CHECK(e.name == C.pool("e"));
            CHECK(e.table.name == C.pool("test"));
        }

        {
            /* Check null bitmap. */
            const auto &null_bitmap = *row_it++;
            REQUIRE(null_bitmap.is_null_bitmap());
            CHECK(null_bitmap.offset == 5); // null bitmap located after attribute 'e', that is at offset 5 bits
            CHECK(null_bitmap.stride == 0); // there is no stride within a row
        }
    }
}

TEST_CASE("RowStore/access", "[milestone1]")
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
    table.store(std::make_unique<RowStore>(table));

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

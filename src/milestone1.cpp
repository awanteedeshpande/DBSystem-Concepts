/*
Checks the implementation of the row and colum store layouts
*/

#include "ColumnStore.hpp"
#include "RowStore.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutable/mutable.hpp>


int main(int argc, const char **argv)
{   
    /* Check the number of parameters. */
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << " <layout> <csv-file> <sql-file>" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* Get a handle on the catalog. */
    auto &C = m::Catalog::Get();

    /* Create a `m::Diagnostic` object. */
    m::Diagnostic diag(true, std::cout, std::cerr);

    /* Register our store(s) and set the default store. */
    C.register_store<RowStore>(C.pool("MyRowStore"));
    C.register_store<ColumnStore>(C.pool("MyColStore"));

    if (streq(argv[1], "row"))
        C.default_store(C.pool("MyRowStore"));
    else if (streq(argv[1], "column"))
        C.default_store(C.pool("MyColStore"));
    else {
        std::cerr << "Unknown data layout '" << argv[1] << '\'' << std::endl;
        exit(EXIT_FAILURE);
    }

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
    m::load_from_CSV(diag, T, argv[2], std::numeric_limits<std::size_t>::max(), true, false);

    if (diag.num_errors())
        exit(EXIT_FAILURE);

    /* Process the SQL file. */
    m::execute_file(diag, argv[3]);

    exit(EXIT_SUCCESS);
}

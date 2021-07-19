/*
Implementation of column store
*/

#include "ColumnStore.hpp"


ColumnStore::ColumnStore(const m::Table &table)
    : Store(table)
{
    /*Allocate columns for the attributes. */
    rows = 0;
    capacity = 8;
    this->table = &table;

    uint32_t sizeOfAttr = 0;
    for (auto it = table.begin(); it != table.end(); it++) {
        sizeOfAttr = (*it).type->size();

        // allocate 8 "rows" for the current column
        char* ptr = (char*) malloc(std::ceil(capacity * sizeOfAttr / 8.0f));  

        // add current column to vector of all columns
        columns.push_back(ptr);
        sizeOfAttrs.push_back(sizeOfAttr);
    }
    /*Allocate a column for the null bitmap. */
    char* nullBitMap = (char*) malloc(std::ceil(capacity * table.size() / 8.0f));
    columns.push_back(nullBitMap);
    sizeOfAttrs.push_back(table.size());

    /*Create linearization.*/
    createLinearization();
}

void ColumnStore::createLinearization() {
    auto lin = std::make_unique<m::Linearization>(m::Linearization::CreateInfinite(columns.size()));
    size_t i = 0;
    while (i < columns.size() - 1) {
        auto col = std::make_unique<m::Linearization>(m::Linearization::CreateFinite(1, 1));
        col->add_sequence(0, 0, table->at(i));
        lin->add_sequence(uint64_t(reinterpret_cast<uintptr_t>(columns.at(i))), sizeOfAttrs[i] / 8, std::move(col));
        i++;
    }
    // null bitmap
    auto null_bm = std::make_unique<m::Linearization>(m::Linearization::CreateFinite(1, 1));
    null_bm->add_null_bitmap(0, 0);
    lin->add_sequence(uint64_t(reinterpret_cast<uintptr_t>(columns.at(i))), sizeOfAttrs[i], std::move(null_bm));

    // set the linearization
    linearization(std::move(lin));
}


ColumnStore::~ColumnStore()
{
    // free the memory of every column
    for(uint32_t x = 0; x < columns.size(); x++) {
        free(columns.at(x));
    }

    // reset fields
    rows = 0;
    capacity = 8;

    return;
}

std::size_t ColumnStore::num_rows() const
{
    return rows;
}

void ColumnStore::append()
{
    // check whether allocated memory is full
    if(rows == capacity) {
        capacity *= 2;
        bool failed = false;
        // allocate more memory for the columns
        for(uint32_t x = 0; x < columns.size(); x++) {
            columns.at(x) = (char*) realloc(columns.at(x), std::ceil(capacity * sizeOfAttrs.at(x) / 8.0f));
            if (columns.at(x) == NULL) {
                std::cout << "could not reallocate memory in ColumnStore::append().." << std::endl;
                failed = true;
            }
        }
        if(failed) {
            this->~ColumnStore();
            exit(1);
        }
        //create new Linearization
        createLinearization();
    }
    // add row 
    rows++;

    return;
}

void ColumnStore::drop()
{
    // check whether there are any rows to drop
    if(rows > 0) rows--;

    // remove row
    return;
}

void ColumnStore::dump(std::ostream &out) const
{
    /*Print description of this store to `out`.*/
    int x = 0;
    for(auto &size:sizeOfAttrs){
        printf("Size of the %d. attribute: %d - address : %s \n", x+1, size, columns.at(x));
        x++;
    }

    out << "\n\n";
    out << rows << " rows in use and " << capacity << " rows are allocated." << std::endl;
    out.flush();
    return;
}
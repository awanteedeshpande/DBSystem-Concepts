#pragma once

#include <mutable/mutable.hpp>


struct ColumnStore : m::Store
{
    private:
    /*Declare necessary fields. */
    std::size_t rows, capacity;
    std::vector<char*> columns; // stores pointers of the columns
    std::vector<uint32_t> sizeOfAttrs; // stores sizes of the attributes as given from the table
    const m::Table *table;

    public:
    ColumnStore(const m::Table &table);
    ~ColumnStore();

    void createLinearization();

    std::size_t num_rows() const override;
    void append() override;
    void drop() override;

    void accept(m::StoreVisitor &v) override { v(*this); }
    void accept(m::ConstStoreVisitor &v) const override { v(*this); }

    void dump(std::ostream &out) const override;
    using Store::dump;
};

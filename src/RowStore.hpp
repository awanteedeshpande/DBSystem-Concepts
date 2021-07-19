#pragma once

#include <mutable/mutable.hpp>
#include <math.h> 

struct RowStore : m::Store
{
    private:
    /*Declare necessary fields. */
    std::size_t rowSize, rowsInUse; //size of 1 row, #rows in use
    std::size_t currentCapacity; //current memory capacity
    std::vector<uint32_t> offsets; //address offsets for linearisation
    std::vector<std::pair<std::string, std::size_t>> attributes; //attributes of table
    char *row_address; //pointer to beginning of row
    const m::Table &row_table; 

    void createLinearization(const m::Table &table, std::vector<uint32_t> &absoluteSizes, char *row_address);
    uint32_t find_aligned(uint32_t offset_in_bits, uint32_t align_in_bits);
    void getRowStoreSizes(const m::Table &table, std::vector<uint32_t> &absoluteSizes, std::size_t &rowSize);

    public:
    RowStore(const m::Table &table);
    ~RowStore();

    std::size_t num_rows() const override;
    void append() override;
    void drop() override;

    void accept(m::StoreVisitor &v) override { v(*this); }
    void accept(m::ConstStoreVisitor &v) const override { v(*this); }

    void dump(std::ostream &out) const override;
    using Store::dump;
};

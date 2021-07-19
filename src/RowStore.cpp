/* Implementation of Row Store Layout for Milestone 1
To debug the code, uncomment the cout statements.
Optionally, add a dump() statement for the respective objects */

#include "RowStore.hpp"

//function to find paddding and resulting offset for a data type
//Refer https://en.wikipedia.org/wiki/Data_structure_alignment
uint32_t RowStore::find_aligned(uint32_t offset_in_bits, uint32_t align_in_bits) {
    uint32_t offset = ceil(offset_in_bits/8.0);
    uint32_t align = ceil(align_in_bits/8.0);
    uint32_t padding = (align - (offset % align)) % align;
    uint32_t aligned = offset + padding;
    return aligned * 8; //return offset in bits
}

//function to get address offsets for table attributes
// use these to calculate size of a row
void RowStore::getRowStoreSizes(const m::Table &table, std::vector<uint32_t> &absoluteSizes, std::size_t &rowSize){
    uint32_t absoluteSize = 0, maxSize, size, sizeAligned;
    maxSize = 1; //size of largest attribute for alignment
    for(auto &attr: table) {
        size = attr.type->size(); //attribute size in bits
        sizeAligned = 0;
        //following condition ignores booleans
        if(size > 8) {
            //padding related calculations
            if(attr.type->is_character_sequence()) {
                sizeAligned = 8; //since a char is 1 byte, retain size of this type to 8 bits
            }
            else {
                sizeAligned = size;
            } 
            absoluteSize = find_aligned(absoluteSize, sizeAligned);
            if(maxSize < sizeAligned) {
                maxSize = sizeAligned;
            }    
        }
        //std::cout << "Offset for attribute " << attr.name <<"(" << size <<") is at " << absoluteSize << "\n";
        absoluteSizes.push_back(absoluteSize);
        absoluteSize += size;
    }
    //std::cout << "Offset for null bitmap at " << absoluteSize << "\n";
    absoluteSizes.push_back(absoluteSize);
    absoluteSize += table.size(); //#attributes = bits for null bitmap
    rowSize = find_aligned(absoluteSize, maxSize)/8;
    //std::cout << "Size with padding = " << rowSize << "\n";
}

//create linearisation
void RowStore::createLinearization(const m::Table &table, std::vector<uint32_t> &absoluteSizes, char *row_address) {
    //std::cout << "Creating Linearization..." << "\n";
    auto lin = std::make_unique<m::Linearization>(m::Linearization::CreateInfinite(1));
    auto row = std::make_unique<m::Linearization>(m::Linearization::CreateFinite((table.size() + 1) /* +1 for null bitmap */, 1));
    uint32_t i = 0;
    for(auto &attr: table)
    {
        row->add_sequence(absoluteSizes[i++], 0, attr);
    }
    //add null bitmap
    row->add_null_bitmap(absoluteSizes[i], 0);
    lin->add_sequence(reinterpret_cast<uintptr_t>(row_address), rowSize, std::move(row));
    linearization(std::move(lin));
}

//row store constructor
RowStore::RowStore(const m::Table &table)
        : Store(table), row_table(table)
{   
    /*Allocate memory. */
    rowSize = 0;
    currentCapacity = 10; //create memory for 10 row
    rowsInUse = 0;
    getRowStoreSizes(row_table, offsets, rowSize);
    row_address = (char*)malloc(rowSize * currentCapacity); //allocate memory
    //dump(std::cout);

    /*Create linearization. */
    createLinearization(row_table, offsets, row_address);
}

//destructor for row store
RowStore::~RowStore()
{
    /*Free allocated memory. */
    free(row_address);
    rowsInUse = 0;
    currentCapacity = 0;
    return;
}

std::size_t RowStore::num_rows() const
{
    /*return rows currently in use*/
    return rowsInUse;
}

//append new row
void RowStore::append()
{
    //check if enough memory to allocate additional row
    if(rowsInUse == currentCapacity) {
        currentCapacity *= 2;
        row_address = (char*)realloc(row_address, rowSize * currentCapacity);
        
        if(row_address == NULL) {
            //throw error
            std::cout << "Error in reallocating memory" << "\n";
            exit(1);
        }
        //reconstruct linearisation for new row address
        createLinearization(row_table, offsets, row_address);
    }
    rowsInUse++;
    return;
}

void RowStore::drop()
{
    /* drop a row */
    if(rowsInUse > 0) {
        rowsInUse--;
    }
    else if(rowsInUse == 0){
        //just print that no rows exist, so cannot drop
        std::cout << "Table empty. Cannot drop any row." << "\n";
    }
    return;
}

void RowStore::dump(std::ostream &out) const
{
    /* Print description of this store to `out`. */
    out << "Store metadata:-" << "\n";
    out << "Attributes and sizes(in bytes):" << "\n";
    for(auto &attr:attributes){
        out << attr.first <<", " << attr.second << "\n";
    }
    out << "Rows in use: " << rowsInUse << "\n";
    out.flush();
    return;
}
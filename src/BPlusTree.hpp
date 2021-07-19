/*
Header file for B+ Tree implementation
*/

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <utility>
#include <cmath>
#include <map>
#include <iostream>
#include <queue>
#include <cstring>

template<
    typename Key,
    typename Value,
    typename Compare = std::less<Key>>
struct BPlusTree
{   
    //using type alias is equivalent to typedef
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = std::size_t;
    using key_compare = Compare;

    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

    private:
    using entry_type = std::pair<Key, Value>;

    public:
    struct entry_comparator
    {
        // c is a comparison method for comparing keys
        // the return statement gives the lesser key between the elemtents first and second
        bool operator()(const value_type &first, const value_type &second) const {
            key_compare c;
            return c(first.first, second.first);
        }
    };

    /*--- Tree Node Data Types ---------------------------------------------------------------------------------------*/
    //Base node type inherited by inner node and leaf node
    struct node {
        char is_leaf;
    };

    struct inner_node;
    struct leaf_node;

    private:
    /* 
     * Declare fields of the B+-tree.
     */
    size_type num_entries;
    size_type height_of_tree;
    leaf_node *first_leaf, *last_leaf;
    entry_type *first_entry, *last_entry;
    node *root;

    /*--- Iterator ---------------------------------------------------------------------------------------------------*/
    private:
    template<bool C>
    struct the_iterator
    {
        friend struct BPlusTree;

        static constexpr bool Is_Const = C;
        //if Is_Const is true, pointer_type is const_pointer, else it's pointer
        using pointer_type = std::conditional_t<Is_Const, const_pointer, pointer>;
        using reference_type = std::conditional_t<Is_Const, const_reference, reference>;

        private:
        leaf_node *node_; ///< the current leaf node
        entry_type *elem_; ///< the current element in the current leaf

        public:
        the_iterator(leaf_node *node, entry_type *elem)
            : node_(node)
            , elem_(elem)
        { }

        /** Returns true iff this iterator points to the same entry as `other`. */
        bool operator==(the_iterator other) const {return this->elem_ == other.elem_; }
        /** Returns false iff this iterator points to the same entry as `other`. */
        bool operator!=(the_iterator other) const { return not operator==(other); }

        /** Advances the iterator to the next element.
         *
         * @return this iterator
         */
        the_iterator & operator++() {
            /*
             * Advance this iterator to the next entry.  The behaviour is undefined if no next entry exists.
             */

             /*
            If there is another element after elem_ in the current leaf node, simply
            advance elem_ to the next element. If elem_ already points to the last element of the leaf, advance
            node_ to the next leaf in the ISAM and elem_ to the first element of that leaf
            */ 
            elem_++;
            if (elem_ == node_->end()) {
                if(node_->next() != nullptr) {
                    node_ = node_->next();
                    elem_ = node_->begin();
                }
            }
            return *this;
        }

        /** Advances the iterator to the next element.
         *
         * @return this iterator
         */
        the_iterator operator++(int) {
            auto old = *this;
            operator++();
            return old;
        }

        /** Returns a pointer to the designated element. */
        pointer_type operator->() const { return reinterpret_cast<pointer_type>(elem_); }
        /** Returns a reference to the designated element */
        reference_type operator*() const { return *operator->(); }
    };
    public:
    using iterator = the_iterator<false>;
    using const_iterator = the_iterator<true>;

    private:
    template<bool C>
    struct the_leaf_iterator
    {
        friend struct BPlusTree;

        static constexpr bool Is_Const = C;
        using pointer_type = std::conditional_t<Is_Const, const leaf_node*, leaf_node*>;
        using reference_type = std::conditional_t<Is_Const, const leaf_node&, leaf_node&>;

        private:
        pointer_type node_; ///< the current leaf node

        public:
        the_leaf_iterator(pointer_type node) : node_(node) { }

        /** Returns true iff this iterator points to the same leaf as `other`. */
        bool operator==(the_leaf_iterator other) const { return this->node_ == other.node_; }
        /** Returns false iff this iterator points to the same leaf as `other`. */
        bool operator!=(the_leaf_iterator other) const { return not operator==(other); }

        /** Advances the iterator to the next leaf.
         *
         * @return this iterator
         */
        the_leaf_iterator & operator++() {
            /*
             * Advance this iterator to the next leaf.  The behaviour is undefined if no next leaf exists.
             */
            node_ = node_->next();
            return *this;
        }

        /** Advances the iterator to the next element.
         *
         * @return this iterator
         */
        the_leaf_iterator operator++(int) {
            auto old = *this;
            operator++();
            return old;
        }

        /** Returns a pointer to the designated leaf. */
        pointer_type operator->() const { return node_; }
        /** Returns a reference to the designated leaf. */
        reference_type operator*() const { return *node_; }
    };
    public:
    using leaf_iterator = the_leaf_iterator<false>;
    using const_leaf_iterator = the_leaf_iterator<true>;

    /*--- Range Type -------------------------------------------------------------------------------------------------*/
    private:
    template<bool C>
    struct the_range
    {
        friend struct BPlusTree;

        private:
        the_iterator<C> begin_;
        the_iterator<C> end_;

        public:
        the_range(the_iterator<C> begin, the_iterator<C> end) : begin_(begin), end_(end) {
        /*
        Commenting the first part out because
        1) It accesses private variables and makes the code implementation open
        2) How to address an empty BPlusTree is up to us, and the assert condition fails for this case
        for our implementation
        */
            #ifndef NDEBUG
                        const bool is_absolute_end = (end_.node_->next() == nullptr) and (end_.elem_ == end_.node_->end());
                        assert(is_absolute_end or not key_compare{}(end_->first, begin_->first)); // begin <= end
            #endif
        }

        the_iterator<C> begin() const { return begin_; }
        the_iterator<C> end() const { return end_; }

        bool empty() const { return begin_ == end_; }
    };
    public:
    using range = the_range<false>;
    using const_range = the_range<true>;


    /** Implements an inner node in a B+-Tree.  An inner node stores k-1 keys that distinguish the k child pointers. */
    struct inner_node : node
    {
        public:
        static constexpr size_type COMPUTE_CAPACITY() {
            /* 
             * Compute the capacity of a inner nodes. The capacity is the number of children an inner node can contain.
             * This means, the capacity equals the fan out. If the capacity is *n*, the inner node can contain *n*
             * children and *n - 1* keys.
             * When computing the capacity of a inner nodes, consider *all fields and padding* in your computation.  For
             * *fundamental types*, the size equals the alignment requirement of that type.
             */
            size_type keysize = sizeof(key_type);
            size_type ptrsize = sizeof(node*);
            return (int)(64.0 - (sizeof(size_type)+ sizeof(node::is_leaf)) + keysize)/(keysize + ptrsize);
        }

        private:
        /* 
         * Declare the fields of an inner node.
         */
        key_type keys[COMPUTE_CAPACITY()-1]; //keys
        node* children[COMPUTE_CAPACITY()]; //pointers to child nodes
        size_type filled_entries; //entries filled so far

        public:
        inner_node() {
            node::is_leaf = '0';
            filled_entries = 0;
            /* 
            The part below is not strictly necessary, but added it as an overkill to prevent
            memory corruption
            */
            std::memset(keys, 0, (COMPUTE_CAPACITY()-1) * sizeof(key_type));
            std::memset(children, 0, (COMPUTE_CAPACITY()) * sizeof(node*));
        }

        ~inner_node() {
            for(size_type i = 0; i < filled_entries; i++) {
                if(children[i]->is_leaf == '0') {
                    inner_node *in = static_cast<inner_node*>(children[i]);
                    delete in;
                }
                else {
                    leaf_node *in = static_cast<leaf_node*>(children[i]);
                    delete in;
                }
            }
        }

        key_type getKey(int index) {
            return keys[index];
        }

        node* getChild(int index) {
            return children[index];
        }

        key_type findLeftMostKey(node *n) {
            if(n->is_leaf == '1') {
                leaf_node *lf = static_cast<leaf_node*>(n);
                return (lf->begin())->first;
            }
            else {
                inner_node *in = static_cast<inner_node*>(n);
                return findLeftMostKey(in->children[0]);
            }
        }

        key_type findRightMostKey(node *n) {
            if(n->is_leaf == '1') {
                leaf_node *lf = static_cast<leaf_node*>(n);
                return (lf->entries[lf->filled_entries-1]).first;
            }
            else {
                inner_node *in = static_cast<inner_node*>(n);
                return findRightMostKey(in->children[in->filled_entries-1]);
            }
        }

        /*
         * In Bulkloading, the logic is to search for a key [k1, k2) w.r.t to key k2.
         * Hence, when we add a child node to a new node, the key is set as the leftmost/rightmost leaf entry
         * in that subtree, based on how the inner node is filled
         * If there is just one key in the inner node, the range is upto the largest key in the subtree
         * Otherwise, for the subsequent keys, the range is from the smallest entry in the subtree
         */
        void add(node* child) {
            if(filled_entries == 0) {
                key_type k = findRightMostKey(child);
                keys[filled_entries] = k;
            }
            else {
                key_type k = findLeftMostKey(child);
                keys[filled_entries-1] = k;
            }
            children[filled_entries] = child;
            filled_entries++;
            return;
        }

        /** Returns the number of children. */
        size_type size() const {
            return filled_entries;
        }

        /** Returns true iff the inner node is full, i.e. capacity is reached. */
        bool full() const {
            return filled_entries == COMPUTE_CAPACITY();
        }
    };

    /** Implements a leaf node in a B+-Tree.  A leaf node stores key-value-pairs.  */
    struct leaf_node : node
    {   
        friend struct inner_node;

        public:
        static constexpr size_type COMPUTE_CAPACITY() {
            /* 
             * Compute the capacity of leaf nodes.  The capacity is the number of key-value-pairs a leaf node can
             * contain.  If the capacity is *n*, the leaf node can contain *n* key-value-pairs.
             * When computing the capacity of inner nodes, consider *all fields and padding* in your computation.  Use
             * `sizeof` to get the size of a field.  For *fundamental types*, the size equals the alignment requirement
             * of that type.
             */
            
            size_type leaf_size = sizeof(size_type) + sizeof(leaf_node*) + sizeof(node::is_leaf);
            size_type entry_size = sizeof(entry_type);
            int capacity = (64.0 - leaf_size) / entry_size;
            return capacity;
        }

        private:
        /*
         * Declare the fields of a leaf node.
         */
        entry_type entries[COMPUTE_CAPACITY()];
        size_type filled_entries;
        leaf_node* next_leaf = nullptr;

        public:
        leaf_node() {
            node::is_leaf = '1';
            filled_entries = 0;
            next_leaf = nullptr;
            //precaution against memory corruption
            std::memset(entries, 0, (COMPUTE_CAPACITY()) * sizeof(entry_type));
        }

        entry_type* getEntry(int index) {
            return &entries[index];
        }

        void add(entry_type item) {
            entries[filled_entries] = item;
            filled_entries++;
        }

        /** Returns the number of entries. */
        size_type size() const {
            return filled_entries;
        }

        /** Returns true iff the leaf is empty, i.e. has zero entries. */
        bool empty() const {
            return filled_entries == 0;
        }

        /** Returns true iff the leaf is full, i.e. the capacity is reached. */
        bool full() const {/
            return filled_entries == COMPUTE_CAPACITY();
        }
        /** Returns a pointer to the next leaf node in the ISAM or `nullptr` if there is no next leaf node. */
        leaf_node * next() const {
            return next_leaf;
        }
        /** Sets the pointer to the next leaf node in the ISAM.  Returns the previously set value.
         *
         * @return the previously set next leaf
         */
        leaf_node * next(leaf_node *new_next) {
            auto old = next_leaf;
            next_leaf = new_next;
            return old;
        }

        /** Returns an iterator to the first entry in the leaf. */
        entry_type * begin() {
            return entries;
        }
        /** Returns an iterator to the entry following the last entry in the leaf. */
        entry_type * end() {
            return entries + filled_entries;
        }
        /** Returns an iterator to the first entry in the leaf. */
        const entry_type * begin() const {
            const auto p = entries;
            return p;
        }
        /** Returns an iterator to the entry following the last entry in the leaf. */
        const entry_type * end() const {
            const auto p = entries + COMPUTE_CAPACITY();
            return p;
        }
        /** Returns an iterator to the first entry in the leaf. */
        const entry_type * cbegin() const {
            const auto p = entries;
            return p;
        }
        /** Returns an iterator to the entry following the last entry in the leaf. */
        const entry_type * cend() const {
            const auto p = entries + COMPUTE_CAPACITY();
            return p;
        }
    };

    //for debugging
    static void printNode(node *n) {
        if(n->is_leaf == '0') {
            inner_node *in = static_cast<inner_node*>(n);
            size_type i = 0;
            if(in->size() == 1){
                std::cout << in->getKey(0) << ",";
            }
            else {
                for(i = 0; i < in->size()-1; i++) {
                    std::cout << in->getKey(i) << ",";
                }
            }
        }
        else {
            leaf_node *lf = static_cast<leaf_node*>(n);
            for(auto i = lf->begin(); i != lf->end(); i++) {
                std::cout << i->first << ",";
            }
        }
        std::cout << "|";
    }
    
    //for debugging
    static void printQueue(std::queue<node*> q) {
        std::cout << "Printing node keys..." << "\n";
        while(!q.empty()) {
            node *n = q.front();
            q.pop();
            printNode(n);
        }
        std::cout << "\n";
    }
    
    //simplified bulkloading
    template<typename It>
    static BPlusTree Bulkload(It begin, It end) {
        // push pairs into a vector
        size_type num_entries = 0;
        leaf_node *first_leaf, *last_leaf;
        size_type height = 0;

        //get all the data in a vector
        std::vector<entry_type> data;
        for (auto i = begin; i != end; ++i) {
            data.push_back(*i);
        }  
        num_entries = data.size();
        
        //0 entries => no BPlusTree
        if(num_entries == 0) {
            leaf_node *lf = new leaf_node();
            return BPlusTree(lf, num_entries, height, lf, lf);
        }


        size_type leaf_capacity = leaf_node::COMPUTE_CAPACITY();
        size_type num_leaves = std::ceil(num_entries * 1.0/ leaf_capacity);
        std::queue<node*> all_nodes1, all_nodes2, empty;

        //create leaf nodes
        leaf_node *current_leaf = new leaf_node();
        first_leaf = current_leaf;
        for(size_type i = 0; i < num_entries; i++) {
            if(current_leaf->full()) {
                leaf_node *new_leaf = new leaf_node();
                current_leaf->next(new_leaf);
                all_nodes1.push(current_leaf);
                current_leaf = new_leaf;
            }
            current_leaf->add(data.at(i));
        }
        all_nodes1.push(current_leaf);
        last_leaf = current_leaf;

         if(num_leaves == 1) {
            return BPlusTree(first_leaf, num_entries, height, first_leaf, first_leaf);
        }

        do {
            inner_node *current_in = new inner_node();
            height++;
            while(!all_nodes1.empty()) {
                if(current_in->full()) {
                    inner_node *new_inner = new inner_node();
                    all_nodes2.push(current_in);
                    current_in = new_inner;
                }
                node *node = all_nodes1.front();
                all_nodes1.pop();
                current_in->add(node);
            }
            all_nodes2.push(current_in);
            all_nodes1 = all_nodes2;
            all_nodes2 = empty;
        } while(all_nodes1.size() != 1);

        inner_node *root = static_cast<inner_node*>(all_nodes1.front());
        height++;

        return BPlusTree(root, num_entries, height, first_leaf, last_leaf);
    }

    template<typename Container>
    static BPlusTree Bulkload(const Container &C) {
        using std::begin, std::end;
        return Bulkload(begin(C), end(C));
    }


    /*--- Start of B+-Tree code --------------------------------------------------------------------------------------*/
    public:
    BPlusTree() {
        num_entries = height_of_tree = 0;
        first_leaf = nullptr;
        first_entry = nullptr;
        root = nullptr;
    }

    BPlusTree(node* tree_root, size_type ne, size_type height, \
    leaf_node *leaf_begin, leaf_node *leaf_end) {
        root = tree_root;
        num_entries = ne;
        height_of_tree = height;
        first_leaf = leaf_begin;
        last_leaf = leaf_end;
        first_entry = first_leaf->begin();
        last_entry = last_leaf->begin() + (last_leaf->size() - 1);
        //printBPlusTree(root);
    }

    BPlusTree(const BPlusTree&) = delete;
    BPlusTree(BPlusTree&&) = default;

    ~BPlusTree() {
        //recursively free children subtrees
        if(root->is_leaf == '0') {
            inner_node *in = static_cast<inner_node*>(root);
            delete in; 
        }
        else {
            leaf_node *in = static_cast<leaf_node*>(root);
            delete in;
        }
        num_entries = height_of_tree = 0;
    }

    //for debugging
    static void printBPlusTree(node *root) {
        node *currentNode = root;
        inner_node* currentIN = nullptr;
        std::queue<node*> q1, q2, empty;
        int level = 0;

        if(currentNode)
            q1.push(currentNode);

        size_type in_capacity = inner_node::COMPUTE_CAPACITY();
        std::cout << "\nLeaf node capacity = " << leaf_node::COMPUTE_CAPACITY() << "\n";
        std::cout << "Inner node capacity = " << in_capacity << "\n";
        
        std::cout << "\nPrinting BPlusTree..." << "\n";
        std::cout << "At level " << level << ": ";
        printQueue(q1);

        while(!q1.empty()) {
            currentNode = q1.front();
            q1.pop();
            if(currentNode->is_leaf == '0') {
                currentIN = static_cast<inner_node*>(currentNode);
                size_type i =0;
                for(i = 0; i < currentIN->size(); i++) {
                    q2.push(currentIN->getChild(i));
                }

                if(q1.empty()) {
                    level++;
                    std::cout << "\nAt level " << level << ": ";
                    printQueue(q2);
                    q1 = q2;
                    q2 = empty;
                }
            }
        }
        std::cout << "\n";
    }

    /** Returns the number of entries. */
    size_type size() const {
        return num_entries;
    }
    /** Returns the height of the tree, i.e. the number of edges on the longest path from leaf to root. */
    size_type height() const {
        return height_of_tree;
    }

    /** Returns an iterator to the first entry in the tree. */
    iterator begin() {
        return iterator(first_leaf, first_entry);
    }

    /** Returns an iterator to the entry following the last entry in the tree. */
    iterator end() {
        if (num_entries == 0) return iterator(first_leaf, first_entry);
        return ++iterator(last_leaf, last_entry);
    }

    /** Returns an iterator to the first entry in the tree. */
    const_iterator begin() const {
        return const_iterator(first_leaf, first_entry);
    }

    /** Returns an iterator to the entry following the last entry in the tree. */
    const_iterator end() const {
        if(num_entries == 0) return const_iterator(first_leaf, first_entry);
        return ++const_iterator(last_leaf, last_entry);
    }

    /** Returns an iterator to the first entry in the tree. */
    const_iterator cbegin() const {
        return const_iterator(first_leaf, first_entry);
    }

    /** Returns an iterator to the entry following the last entry in the tree. */
    const_iterator cend() const {
        if(num_entries == 0) return const_iterator(first_leaf, first_entry);
        return ++const_iterator(last_leaf, last_entry);
    }

    /** Returns an iterator to the first leaf of the tree. */
    leaf_iterator leaves_begin() {
        return leaf_iterator(first_leaf);
    }

    /** Returns an iterator to the next leaf after the last leaf of the tree. */
    leaf_iterator leaves_end() {
        return ++leaf_iterator(last_leaf);
    }

    /** Returns an iterator to the first leaf of the tree. */
    const_leaf_iterator leaves_begin() const {
        return const_leaf_iterator(first_leaf);
    }

    /** Returns an iterator to the next leaf after the last leaf of the tree. */
    const_leaf_iterator leaves_end() const {
        return ++const_leaf_iterator(last_leaf);
    }

    /** Returns an iterator to the first leaf of the tree. */
    const_leaf_iterator cleaves_begin() const {
        return const_leaf_iterator(first_leaf);
    }

    /** Returns an iterator to the next leaf after the last leaf of the tree. */
    const_leaf_iterator cleaves_end() const {
        return ++const_leaf_iterator(last_leaf);
    }

    /** Returns an iterator to the first entry with a key that equals `key`, or `end()` if no such entry exists. */
    const_iterator find(const key_type key) const {
        if (num_entries < 1) return cend();

        // go through nodes to search key
        node* currentNode = root; 
        while(currentNode->is_leaf == '0') {
            inner_node* innerNode = reinterpret_cast<inner_node*>(currentNode);
            size_type index = 0;
            bool found = false;
            for(index = 0; index < innerNode->size()-1; index++) {
                if (key < innerNode->getKey(index)) {
                    found = true;
                    break; 
                }
            }
            if(!innerNode->full() && !found) {
                index++;
            }
            if(index < innerNode->size()) {
                currentNode = innerNode->getChild(index);
            }
            else {
                return cend();
            }
        }

        // go through leaf and search the key
        leaf_node* leaf = static_cast<leaf_node*>(currentNode);
        for (auto entry = leaf->begin(); entry != leaf->end(); entry++) {
            if (entry->first == key) {
                // entry *i equals 'key'
                return iterator(leaf, entry);
            }
        }

        // found no entry that equals 'key'
        return cend();
    }

    /** Returns an iterator to the first entry with a key that equals `key`, or `end()` if no such entry exists. */
    iterator find(const key_type &key) {
        if (num_entries < 1) return end();

        // go through nodes to search key
        node* currentNode = root; 
        while(currentNode->is_leaf == '0') {
            inner_node* innerNode = reinterpret_cast<inner_node*>(currentNode);
            size_type index = 0;
            bool found = false;
            /*
            The range support is [k1, k2), hence we check for key < k2
            */
            for(index = 0; index < innerNode->size()-1; index++) {
                if (key < innerNode->getKey(index)) {
                    found = true;
                    break; 
                }
            }
            if(!innerNode->full() && !found) {
                index = innerNode->size()-1;
            }
            if(index < innerNode->size()) {
                currentNode = innerNode->getChild(index);
            }
            else {
                return end();
            }
        }

        // go through leaf and search the key
        leaf_node* leaf = static_cast<leaf_node*>(currentNode);
        for (auto entry = leaf->begin(); entry != leaf->end(); entry++) {
            if (entry->first == key) {
                // entry *i equals 'key'
                return iterator(leaf, entry);
            }
        }

        // found no entry that equals 'key'
        return end();
    }

    /** Returns the range of entries between `lower` (including) and `upper` (excluding). */
    const_range in_range(const key_type &lower, const key_type &upper) const {
        if (num_entries < 1) {
            return const_range(iterator(nullptr, nullptr), iterator(nullptr, nullptr));
        }

        the_iterator start = findLower(lower, root);
        if((start == end()) || (start->first >= upper)) {
            return const_range(iterator(nullptr, nullptr), iterator(nullptr, nullptr));
        }

        the_iterator fin = start;
        while(!(fin == end())) {
            if((fin->first) >= upper) {
                break;
            }
            fin++;
        }
        return const_range(start, fin);
    }

    /** Returns the range of entries between `lower` (including) and `upper` (excluding). */
    range in_range(const key_type &lower, const key_type &upper) {
        if (num_entries < 1) {
            return range(end(), end());
        }

        iterator start = findLower(lower, root);
        if((start == end()) || (start->first >= upper)) {
            return range(end(), end());
        }

        iterator fin = start;
        while(!(fin == end())) {
            if((fin->first) >= upper) {
                break;
            }
            fin++;
        }
        return range(start, fin);
    }

    //find element >= lower
    iterator findLower (const key_type &lower, node* root) {
        // go through nodes to search keys in range
        node* currentNode = root; 
        while(currentNode->is_leaf == '0') {
            inner_node* innerNode = reinterpret_cast<inner_node*>(currentNode);
            size_type index = 0;
            bool found = false;
    
            for(index = 0; index < innerNode->size()-1; index++) {
                if (lower < innerNode->getKey(index)) {
                    found = true;
                    break; 
                }
            }
            if(!innerNode->full() && !found) {
                index = innerNode->size()-1;
            }
            if(index < innerNode->size()) {
                currentNode = innerNode->getChild(index);
            }
        }
        // go through leaf and search the key
        leaf_node* leaf = static_cast<leaf_node*>(currentNode);
        for (entry_type *i = leaf->begin(); i != leaf->end(); i++) {
            entry_type entry = *i;
            if (entry.first >= lower) {
                return iterator(leaf, i);
            }
        }
        return end();
    }

};

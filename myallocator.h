#pragma once

#include <iostream>
#include <limits>
#include <forward_list>
#include <memory>
#include <utility>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <cassert>

extern uint32_t allocator_num;
extern uint32_t allocate_count;
extern uint32_t deallocate_count;

template<class T, std::size_t begin_default_pool_size = 4>
struct MyAllocator
{
    using value_type = T;
    using size_type = std::size_t;

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    static constexpr size_type max_pool_size = 1024;

    MyAllocator() noexcept : allocator_id(++allocator_num) {}

    MyAllocator(const MyAllocator<T>& oth) noexcept 
    :   default_pool_size(oth.default_pool_size),
        allocator_id(++allocator_num) {}
    MyAllocator<T>& operator=(const MyAllocator<T>& oth) noexcept 
    { 
        if (this != & oth)
            default_pool_size = oth.default_pool_size;
        return *this; 
    }

    MyAllocator(MyAllocator<T>&& oth) noexcept
    :   default_pool_size(oth.default_pool_size),
        node_count(oth.node_count),
        pool_list(std::move(oth.pool_list)),
        empty_list(oth.empty_list),
        allocator_id(++allocator_num)
    {
        oth.default_pool_size = 10;
        oth.node_count = 0;
        oth.empty_list = nullptr;
    }
    MyAllocator<T>& operator=(MyAllocator<T>&& oth) noexcept
    { 
        if (this != &oth) {
            default_pool_size = oth.default_pool_size;
            node_count = oth.node_count;
            pool_list = std::move(oth.pool_list);
            empty_list = oth.empty_list;
            oth.default_pool_size = 10;
            oth.node_count = 0;
            oth.empty_list = nullptr;
        }
        return *this; 
    }

    template<class U>
    MyAllocator(const MyAllocator<U>&) noexcept {}

    template <typename U>
    struct rebind {
        using other = MyAllocator<U>;
    };

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (n != 1)
            throw std::bad_array_new_length();
        
        Node* node = NodeAllocate();
        ++allocate_count;
        // Report(reinterpret_cast<T*>(& node->data), n);
        return reinterpret_cast<T*>(& node->data);
    }
    
    void deallocate(T* p, std::size_t n)
    {
        assert(n == 1);
        // Report(p, n, 0);
        NodeDeallocate(reinterpret_cast<Node*>(p));
        ++deallocate_count;
    }
private:
    void Report(T* p, std::size_t n, bool alloc = true) const
    {
        std::cout   << allocator_id << " "
                    << (alloc ? "Alloc: " : "Dealloc: ") 
                    << " count: " << n
                    << " size: " << sizeof(T)
                    << " bytes at " << std::hex << std::showbase
                    << reinterpret_cast<void*>(p) << std::dec << '\n';
    }

    union Node {
        Node* next;
        std::aligned_storage_t<sizeof(T), alignof(T)> data;
    };

    struct Pool {
        Pool() = delete;
        Pool(const Pool& oth) = delete;
        Pool& operator=(Pool&) = delete;

        ~Pool() 
        {
            if (arr != nullptr)
                std::free(arr);
        }
        explicit Pool(const size_type n) {
            if (n == 0)
                throw std::length_error("Pool size is zero");

            size = n;
            arr = reinterpret_cast<Node*>(std::calloc(n, sizeof(Node)));
        }
        Pool(Pool&& oth) : size(oth.size), arr(oth.arr) {
            oth.arr = nullptr;
            oth.size = 0;
        }
        Pool& operator=(Pool&& oth) {
            if (this != & oth) {
                arr = oth.arr;
                size = oth.size;
                oth.arr = nullptr;
                oth.size = 0;
            }
            return *this;
        }

        size_type size;
        Node* arr;
    };

    void EmplacePoolToEmptyList(const Pool& pool) noexcept {
        Node* first = pool.arr;
        Node* cur = first;
        const Node* const end = first + pool.size;
        for (; cur != end; ++cur)
            cur->next = cur + 1;

        --cur; 
        cur->next = empty_list;
        empty_list = first;
    }

    void AddPool() {
        pool_list.emplace_front(default_pool_size);
        EmplacePoolToEmptyList(pool_list.front());

        node_count += default_pool_size;
        default_pool_size += default_pool_size / 2;
        if (default_pool_size > max_pool_size)
            default_pool_size = max_pool_size;
    }

    Node* NodeAllocate() {
        if (empty_list == nullptr)
            AddPool();

        Node* node = empty_list;
        empty_list = node->next;
        node->next = nullptr;
        return node;
    }

    bool NodeCheck(const Node* const node) const noexcept {
        if (node == nullptr)
            return false;

        for (auto it = pool_list.cbegin(); it != pool_list.cend(); ++it) {
            if (node >= it->arr && node < (it->arr + it->size))
                return true;
        }
        return false;
    }

    void NodeDeallocate(Node* node) {
        if (!NodeCheck(node))
            throw std::invalid_argument("Bad node address!");

        node->next = empty_list;
        empty_list = node;
    }

    size_type default_pool_size = begin_default_pool_size;
    size_type node_count = 0;
    std::forward_list<Pool> pool_list;
    Node* empty_list = nullptr;
    uint32_t allocator_id;
};

template<class T, class U>
bool operator==(const MyAllocator <T>&, const MyAllocator <U>&) { return false; }

template<class T, class U>
bool operator!=(const MyAllocator <T>&, const MyAllocator <U>&) { return true; }

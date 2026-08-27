#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <iterator>
#include <utility>
#include <exception>
#include <cassert>

template<typename T>
struct Node {
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;

    Node* next;
    T data;

    Node() = delete;
    Node(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(const Node&)  = delete;
    Node& operator=(const Node&&)  = delete;

    Node(const T& val) : next(nullptr), data(val) {}
    Node(T&& val) : next(nullptr), data(std::forward(val)) {}
};

template<typename T, typename Alloc = std::allocator<T>>
struct MyForwardList {
    using node_type = Node<T>;
    using node_allocator_type = typename std::allocator_traits<Alloc>::rebind_alloc<node_type>;

private:
    using alloc_type = typename std::allocator_traits<Alloc>::rebind_alloc<T>;
    using alloc_traits = typename std::allocator_traits<alloc_type>;
    using node_allocator_traits = std::allocator_traits<node_allocator_type>;
    template<typename Iterator>
    using iterator_value_t = std::remove_cv_t<typename std::iterator_traits<Iterator>::value_type>;

public:
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename alloc_traits::pointer;
    using const_pointer = typename alloc_traits::const_pointer;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = alloc_type;

    ~MyForwardList() { clear(); }
    MyForwardList() = default;

    MyForwardList(const MyForwardList& oth)
    :   allocator(node_allocator_traits::select_on_container_copy_construction(oth.allocator)),
        head(nullptr)
    { initialize_from_it(oth.cbegin(), oth.cend()); }

    MyForwardList(MyForwardList&& oth) noexcept(std::is_nothrow_move_constructible_v<node_allocator_traits>)
    :   allocator(std::forward(oth.allocator)),
        head(std::exchange(oth.head, nullptr)) {}

    MyForwardList& operator=(const MyForwardList& oth) {
        if (this == &oth)
            return *this;

        clear();
            
        if constexpr (node_allocator_traits::propagate_on_container_copy_assignment::value) {
            allocator = oth.allocator;
        } 

        initialize_from_it(oth.cbegin(), oth.cend());
        return *this;
    }        

    MyForwardList& operator=(MyForwardList&& oth) {
        static_assert(node_allocator_traits::propagate_on_container_move_assignment::value);
        if (this != &oth) {
            clear();
            allocator = std::move(oth.allocator); 
            head = std::exchange(oth.head, nullptr);
        }
        return *this;
    }        

    node_allocator_type& get_allocator() const noexcept { return allocator; }
    size_type max_size() const noexcept { return node_allocator_traits::max_size(allocator); }
    bool empty() const noexcept { return head == nullptr; }

    void clear() {
        while(!empty())
            pop_front();
    }
    void pop_front() {
        node_type* cur = head;
        if (cur != nullptr) {
            head = cur->next;
            node_allocator_traits::destroy(allocator, cur);
            node_allocator_traits::deallocate(allocator, cur, 1);
        }
    }

    reference front() {
        if (empty())
            throw std::out_of_range("List is empty");

        return *head;
    }
    const_reference front() const {
        if (empty())
            throw std::out_of_range("List is empty");

        return *head;
    }

    void push_front(const_reference oth) {
        node_type* node = node_allocator_traits::allocate(allocator, 1);
        node_allocator_traits::construct(allocator, node, oth);
        node->next = head;
        head = node;
    }
    void push_front(value_type&& oth) {
        node_type* node = node_allocator_traits::allocate(allocator, 1);
        node_allocator_traits::construct(allocator, node, std::forward(oth));
        node->next = head;
        head = node;
    }

    template< class... Args >
    reference emplace_front(Args&&... args) {
        node_type* node = node_allocator_traits::allocate(allocator, 1);
        node_allocator_traits::construct(allocator, node, std::forward<Args>(args)...);
        node->next = head;
        head = node;
        return head->data;
    }

    struct iterator {
        using Self = iterator;
        using value_type = T;
        using reference = value_type&;
        using pointer = value_type*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag; 

        iterator() = delete;
        explicit iterator(node_type* p) noexcept : cur(p) {}
        explicit iterator(node_type& p) noexcept : cur(&p) {}

        reference operator*() const noexcept { return cur->data; }
        pointer operator->() const noexcept { return &cur->data; }

        Self& operator++() noexcept { cur = cur->next; return *this; }
        Self operator++(int) noexcept {
            Self tmp(*this);
            cur = cur->next;
            return tmp;
        }

        friend bool operator==(const Self& x, const Self& y) noexcept { return x.cur == y.cur; }
        friend bool operator!=(const Self& x, const Self& y) noexcept { return x.cur != y.cur; }

    private:
        friend struct MyForwardList<T, Alloc>;
        friend struct const_iterator;
        node_type* cur = nullptr;
    };

    struct const_iterator {
        using Self = const_iterator;
        using value_type = const T;
        using reference = value_type&;
        using pointer = value_type*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag; 

        const_iterator() = delete;
        explicit const_iterator(node_type* p) noexcept : cur(p) {}
        explicit const_iterator(node_type& p) noexcept : cur(&p) {}
        explicit const_iterator(const iterator* it) noexcept : cur(it->cur) {}

        const_reference operator*() const noexcept { return cur->data; }
        const_pointer operator->() const noexcept { return &cur->data; }

        Self& operator++() noexcept { cur = cur->next; return *this; }
        Self operator++(int) noexcept {
            Self tmp(*this);
            cur = cur->next;
            return tmp;
        }

        friend bool operator==(const Self& x, const Self& y) noexcept { return x.cur == y.cur; }
        friend bool operator!=(const Self& x, const Self& y) noexcept { return x.cur != y.cur; }

    private:
        friend struct MyForwardList<T, Alloc>;
        const node_type* cur = nullptr;
    };

    iterator begin() noexcept { return iterator(head); }
    const_iterator begin() const noexcept { return const_iterator(head); }
    const_iterator cbegin() const noexcept { return const_iterator(head); }
    iterator end() noexcept { return iterator(static_cast<node_type*>(nullptr)); }
    const_iterator end() const noexcept { return const_iterator(static_cast<node_type*>(nullptr)); }
    const_iterator cend() const noexcept { return const_iterator(static_cast<node_type*>(nullptr)); }

    iterator insert_after(const_iterator pos, const_reference value) {
        node_type* cur = contains(pos);
        if (cur == nullptr)
            return end();
        
        node_type* node = node_allocator_traits::allocate(allocator, 1);
        node_allocator_traits::construct(allocator, node, value);
        node->next = cur->next;
        cur->next = node;
        return iterator(node);
    }
    iterator insert_after(const_iterator pos, value_type&& value) {
        node_type* cur = contains(pos);
        if (cur == nullptr)
            return end();
        
        node_type* node = node_allocator_traits::allocate(allocator, 1);
        node_allocator_traits::construct(allocator, node, std::forward(value));
        node->next = cur->next;
        cur->next = node;
        return iterator(node);
    }

    template<typename InputIt>
    std::enable_if_t< std::is_same_v< iterator_value_t<InputIt>, value_type >, iterator> 
    insert_after(const_iterator pos, InputIt first, InputIt last ) {
        node_type* cur = contains(pos);
        if (cur == nullptr || first == last)
            return end();

        node_type* new_last_node = node_allocator_traits::allocate(allocator, 1);
        node_allocator_traits::construct(allocator, new_last_node, *first);
        node_type* new_head = new_last_node;
        for (++first; first != last; ++first) {
            node_type* new_cur = new_last_node;
            new_last_node = node_allocator_traits::allocate(allocator, 1);
            node_allocator_traits::construct(allocator, new_last_node, *first);
            new_cur->next = new_last_node;
        }
        new_last_node->next = cur->next;
        cur->next = new_head;
    }

private:
    template<typename InputIt>
    std::enable_if_t< std::is_same_v< iterator_value_t<InputIt>, value_type >, void>
    initialize_from_it(InputIt first, InputIt last) 
    {
        assert(head == nullptr);
        node_type* new_last_node = node_allocator_traits::allocate(allocator, 1);
        node_allocator_traits::construct(allocator, new_last_node, *first);
        head = new_last_node;
        for (++first; first != last; ++first) {
            node_type* new_cur = new_last_node;
            new_last_node = node_allocator_traits::allocate(allocator, 1);
            node_allocator_traits::construct(allocator, new_last_node, *first);
            new_cur->next = new_last_node;
        }
    }

    node_type* contains(const_iterator pos) const noexcept {
        if (pos.cur == nullptr)
            return nullptr;
        for (const node_type* cur = head; cur != nullptr; cur = cur->next) {
            if (cur == pos->cur)
                return cur;
        }
        return nullptr;
    }

    node_allocator_type allocator;
    node_type* head = nullptr;
};
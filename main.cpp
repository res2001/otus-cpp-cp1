#include "myallocator.h"
#include "myfwdlist.h"
#include <iostream>
#include <map>
#include <forward_list>
#include <list>
#include <type_traits>
#include <cstdint>
#include <cassert>

uint32_t allocator_num = 0;
uint32_t allocate_count = 0;
uint32_t deallocate_count = 0;

int factorial(int number) {
    int res = 1;
    for (int i = 2; i <= number; ++i) {
        res *= i;
    }
    return res;
}

using Map = std::map<int, int, std::less<int>, MyAllocator<std::map<int, int>::value_type>>;
using FWL = std::forward_list<int, MyAllocator<std::forward_list<int>::value_type>>;
using LST = std::list<int, MyAllocator<std::list<int>::value_type>>;

static_assert(std::is_copy_constructible_v<MyForwardList<int>>);
static_assert(std::is_move_constructible_v<MyForwardList<int>>);
static_assert(std::is_copy_assignable_v<MyForwardList<int>>);
static_assert(std::is_move_assignable_v<MyForwardList<int>>);
static_assert(std::allocator_traits<MyAllocator<int>>::is_always_equal::value || 
            std::is_nothrow_swappable_v<MyAllocator<int>>);

std::ostream& operator<<(std::ostream& stream, Map::const_iterator& it) {
    stream << it->first << ' ' << it->second;
    return stream;
}

void print_map(const Map& map) {
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        std::cout << it << std::endl;
    }
}

void test_std_map() {
    std::cout << "Test std::map<int, int, MyAllocator>\n";
    Map map1;
    for (int i = 0; i < 10; ++i) {          /* emplace */
        map1.emplace(i, factorial(i));
    }
    assert(map1.size() == 10);
    for (int i = 0; i < 10; ++i)
        assert(map1[i] == factorial(i));
    std::cout << "MAP1:\n"; print_map(map1);

    Map map2(map1.cbegin(), map1.cend());   /* конструктор с итераторами */
    assert(map2.size() == map1.size() && map1.size() == 10);
    for (int i = 0; i < 10; ++i)
        assert(map1[i] == map2[i]);
    // std::cout << "MAP2:\n"; print_map(map2);

    for (int i = 0; i < 10; ++i)            /* удаление элементов */
        map1.erase(i);
    assert(map1.size() == 0);

    map1 = map2;                            /* копировние */
    assert(map2.size() == map1.size() && map1.size() == 10);
    for (int i = 0; i < 10; ++i)
        assert(map1[i] == map2[i]);
    map1[9] = 0;
    assert(map1[9] == 0 && map2[9] != 0);
    // std::cout << "MAP1:\n"; print_map(map1);

    auto alloc1 = map1.get_allocator();
    auto alloc2 = map2.get_allocator();
    assert(alloc1 != alloc2);

    map1 = std::move(map2);                 /* перемещение */
    assert(map2.size() == 0);
    assert(map1.size() == 10);
    for (int i = 0; i < 10; ++i)
        assert(map1[i] == factorial(i));
    // std::cout << "MAP1:\n"; print_map(map1);
    // std::cout << "MAP2:\n"; print_map(map2);

    Map map3{{0,0}, {1,1}, {2,2}, {3,3}, {4,4}}; /* initializer_list */
    assert(map3.size() == 5);
    for (int i = 0; i < 5; ++i)
        assert(map3[i] == i);
    // std::cout << "MAP3:\n"; print_map(map3);
}

void test_std_forward_list() {
    std::cout << "Test std::forward_list<int, MyAllocator>\n";

    FWL fl1;                                /* forward list.emplace */
    for (int i = 0; i < 10; ++i)
        fl1.emplace_front(i);
    int tmp = 9;
    for (auto it = fl1.cbegin(); it != fl1.cend(); ++it)
        assert(*it == tmp--);

    FWL fl2(fl1);                           /* forward list.copy ctor */
    {
        auto it1 = fl1.cbegin();
        auto it2 = fl2.cbegin();
        for (int i = 0; i < 10; ++i)
            assert(*it1 == *it2);
    }

    fl1 = std::move(fl2);                   /* forward list.move assign */
    assert(fl2.empty());
    tmp = 9;
    for (auto it = fl1.cbegin(); it != fl1.cend(); ++it)
        assert(*it == tmp--);
}

void test_std_list() {
    std::cout << "Test std::list<int, MyAllocator>\n";
    LST ls1;                                /* list.emplace */
    for (int i = 0; i < 10; ++i)
        ls1.emplace_back(i);
    int tmp = 0;
    for (auto it = ls1.cbegin(); it != ls1.cend(); ++it)
        assert(*it == tmp++);

    LST ls2(ls1);                          /* list.copy ctor */
    assert(ls2.size() == ls1.size() && ls1.size() == 10);
    {
        auto it1 = ls1.cbegin();
        auto it2 = ls2.cbegin();
        for (int i = 0; i < 10; ++i)
            assert(*it1 == *it2);
    } 

    ls2 = std::move(ls1);                   /* list.move assign */
    assert(ls1.empty());
    tmp = 0;
    for (auto it = ls2.cbegin(); it != ls2.cend(); ++it)
        assert(*it == tmp++);
}

template<typename Alloc>
void test_myforwadrd_list() {
    using MYFWL = MyForwardList<int, Alloc>;
    MYFWL fl1;                                /* list.emplace */
    for (int i = 9; i >= 0; --i)
        fl1.emplace_front(i);
    int tmp = 0;
    for (auto it = fl1.cbegin(); it != fl1.cend(); ++it) {
        assert(*it == tmp++);
        std::cout << (tmp == 0 ? "" : " ") << *it;
    }
    std::cout << std::endl;

    MYFWL fl2(fl1);                          /* list.copy ctor */
    {
        auto it1 = fl1.cbegin();
        auto it2 = fl2.cbegin();
        for (int i = 0; i < 10; ++i)
            assert(*it1++ == *it2++);
    } 

    fl2 = std::move(fl1);                   /* list.move assign */
    assert(fl1.empty());
    tmp = 0;
    for (auto it = fl2.cbegin(); it != fl2.cend(); ++it)
        assert(*it == tmp++);
}

int main(int, char **) {
    test_std_map();
    test_std_forward_list();
    test_std_list();

    std::cout << "Test MyForwardList<int, std::allocator>\n";
    test_myforwadrd_list<std::allocator<int>>();
    std::cout << "Test MyForwardList<int, MyAllocator>\n";
    test_myforwadrd_list<MyAllocator<int>>();

    std::cout << "allocate_count=" << allocate_count << " deallocate_count=" << deallocate_count << std::endl;
    assert(allocate_count == deallocate_count);
}

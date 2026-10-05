#pragma once
#include <cstdint>
#include <array>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <optional>
#include <type_traits>
#include <tuple>
#include <cassert>
#include "hash_mix.h"

namespace matrix {

template <size_t Dim, size_t MatrixMaxDimension = 128, 
        typename = std::enable_if_t<(Dim > 0 && Dim <= MatrixMaxDimension)>>
struct MatrixIndex
{
    static constexpr size_t BAD_IDX = SIZE_MAX;
    using base_type = size_t;
    using key_type = std::array<base_type, Dim>;

    MatrixIndex() { index_arr.fill(BAD_IDX); }
    MatrixIndex(const key_type& arr) noexcept : index_arr(arr) {}
    constexpr MatrixIndex(std::initializer_list<base_type> list) {
        assert(list.size() == Dim && "Количество индексов не совпадает с размерностью матрицы!");
        std::copy(list.begin(), list.end(), index_arr.begin());
    }

    bool operator==(const MatrixIndex& oth) const noexcept { return index_arr == oth.index_arr; }

    std::string to_string() const
    {
        std::stringstream ss;
        ss << "[";
        std::for_each(index_arr.cbegin(), index_arr.cbegin() + (Dim - 1), [&ss](base_type idx) { ss << idx << ", "; });
        ss << index_arr[Dim - 1] << "]";
        return ss.str();
    }

    void Set(size_t dim_idx, base_type idx) noexcept {
        assert(dim_idx < Dim);
        index_arr[dim_idx] = idx;
    }
    void Set(const key_type& arr) noexcept {
        assert(MatrixIndex::isValid(arr));
        index_arr = arr;
    }

    static bool isValid(const key_type& arr) noexcept {
        return std::find_if(arr.cbegin(), arr.cend(), [](base_type idx) { return idx == BAD_IDX; }) == arr.cend();
    }
    bool isValid() const noexcept { return MatrixIndex::isValid(index_arr); }

    struct MatrixIndexHash
    {
        std::size_t operator()(const MatrixIndex& mi) const noexcept
        {
            assert(mi.isValid());
            size_t seed = 0;
            std::for_each(mi.index_arr.cbegin(), mi.index_arr.cend(), [&seed](base_type idx) { hash::hash_combine(seed, idx); });
            return seed;
        }
    };

    const key_type& GetRaw() const noexcept { return index_arr; }

private:
    friend class MatrixIndexHash;
    key_type index_arr;
};


template<typename T, size_t Dim>
struct MatrixType {
    using Key = MatrixIndex<Dim>;
    using Hash = typename Key::MatrixIndexHash;
    using Map = std::unordered_map<Key, T, Hash>;
};


template<typename T, size_t Dim, typename Default, bool isConst>
class MatrixSquareBracketsFirst;
template<typename T, size_t Dim, typename Default>
class Matrix;


template <typename T, size_t Dim, typename Default, bool isConst>
class ElementProxyImpl {
    using MapT = typename MatrixType<T, Dim>::Map;
    using MapType = std::conditional_t<isConst, const MapT, MapT>;
    using TType = typename std::conditional_t<isConst, const T, T>;
    using Key = typename MatrixType<T, Dim>::Key;

    template<typename, size_t, typename, bool, size_t, typename>
    friend class MatrixSquareBracketsNext;
    friend class MatrixSquareBracketsFirst<T, Dim, Default, isConst>;
    friend class Matrix<T, Dim, Default>;

    MapType& m;
    Key key;

    ElementProxyImpl(MapType& m_, const Key& key_) : m(m_), key(key_) {}

public:
    template<bool C = isConst> 
    std::enable_if_t<!C, ElementProxyImpl&>
    operator=(const T& val) {
        if (val == Default{}()) {
            m.erase(key);
        } else {
            m[key] = val;
        }
        return *this;
    }

    template<bool C = isConst>
    std::enable_if_t<!C, ElementProxyImpl&>
    operator=(T&& val) {
        if (val == Default{}()) {
            m.erase(key);
        } else {
            m[key] = std::move(val);
        }
        return *this;
    }

    operator TType() const {
        auto it = m.find(key);
        if (it != m.end())
            return it->second;
        return Default{}();
    }

    const Key& get_key() const noexcept { return key; }
};


template<typename T, size_t Dim, typename Default, bool isConst, size_t cur_dim, typename = std::enable_if_t<(cur_dim >= 1 && cur_dim < (Dim - 1)), void> >
class MatrixSquareBracketsNext {
    using MapT = typename MatrixType<T, Dim>::Map;
    using MapType = std::conditional_t<isConst, const MapT, MapT>;
    using Key = typename MatrixType<T, Dim>::Key;
    MapType& m;
    Key& key;
public:
    MatrixSquareBracketsNext(MapType& m_, Key& key_, typename Key::base_type cur_idx) noexcept : m(m_), key(key_) 
    { 
        assert(cur_idx != Key::BAD_IDX);
        key.Set(cur_dim, cur_idx);
    }

    auto operator[](typename Key::base_type cur_idx) {
        assert(cur_idx != Key::BAD_IDX);
        if constexpr ((cur_dim + 1) < (Dim - 1)) {
            return MatrixSquareBracketsNext<T, Dim, Default, isConst, cur_dim + 1>(m, key, cur_idx);
        } else {
            key.Set(cur_dim + 1, cur_idx);
            assert(key.isValid());
            return ElementProxyImpl<T, Dim, Default, isConst>(m, key);
        }
    }
};


template <typename T, size_t Dim, typename Default, bool isConst>
class MatrixSquareBracketsFirst {
    using MapT = typename MatrixType<T, Dim>::Map;
    using MapType = std::conditional_t<isConst, const MapT, MapT>;
    using Key = typename MatrixType<T, Dim>::Key;
    MapType& m;
    Key key;
public:
    MatrixSquareBracketsFirst(MapType& m_, typename Key::base_type cur_idx) noexcept 
    : m(m_), key() 
    { 
        static_assert(Dim > 1);
        assert(cur_idx != Key::BAD_IDX);
        key.Set(0, cur_idx); 
    }

    auto operator[](typename Key::base_type cur_idx) {
        assert(cur_idx != Key::BAD_IDX);
        if constexpr (Dim > 2) {
            return MatrixSquareBracketsNext<T, Dim, Default, isConst, 1>(m, key, cur_idx);
        } else {
            key.Set(1, cur_idx);
            assert(key.isValid());
            return ElementProxyImpl<T, Dim, Default, isConst>(m, key);
        }
    }
};

template<typename T, size_t Dim, bool isConst>
struct ElementImpl;

} /* matrix */

namespace std {
    // 1. Говорим, что ElementImpl раскладывается на Dim + 1 элементов
    template <typename T, size_t Dim, bool IsConst>
    struct tuple_size<matrix::ElementImpl<T, Dim, IsConst>> 
        : std::integral_constant<size_t, Dim + 1> {};

    // 2. Описываем типы каждого элемента
    template <size_t I, typename T, size_t Dim, bool IsConst>
    struct tuple_element<I, matrix::ElementImpl<T, Dim, IsConst>> {
        using type = std::conditional_t<
            (I < Dim),
            const size_t&, // Индексы возвращаем по значению
            std::conditional_t<IsConst, const T&, T&> // Значение — по ссылке
        >;
    };
}

namespace matrix {

template<typename T, size_t Dim, bool isConst>
class ElementImpl {
    template <size_t... Is>
    static auto make_tuple_type(std::index_sequence<Is...>) -> std::tuple<
        std::conditional_t<(Is >= 0), size_t, size_t>..., 
        std::conditional_t<isConst, const T&, T&>
    >;
public:
    using TType = std::conditional_t<isConst, const T, T>;
    using Key = typename MatrixType<T, Dim>::Key;
    using TupleType = decltype(make_tuple_type(std::make_index_sequence<Dim>{}));

    const Key& indices;
    TType& value;

    template <size_t I>
    decltype(auto) get() const {
        static_assert(I <= Dim, "Index out of range!");
        if constexpr (I < Dim) {
            return indices.GetRaw()[I];
        } else {
            return (value);
        }
    }    

    template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == Dim + 1>>
    operator std::tuple<Args...>() const {        
        typename Key::key_type mutable_indices = indices.GetRaw(); 
        return std::apply([this](auto... args) {
            return std::tuple<Args...>{ args..., value };
        }, mutable_indices);
    }
};

template<typename T, size_t Dim, bool isConst>
class MatrixIteratorImpl {
    using MapT = typename MatrixType<T, Dim>::Map;
    using MapIteratorType = std::conditional_t<isConst, typename MapT::const_iterator, typename MapT::iterator>;
    using ReturnType = ElementImpl<T, Dim, isConst>;
    using Self = MatrixIteratorImpl<T, Dim, isConst>;
    using value_type = ReturnType;
    using reference = value_type&;
    using pointer = value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag; 

public:
    MatrixIteratorImpl() = delete;
    explicit MatrixIteratorImpl(MapIteratorType it_) noexcept : it(it_), element_cache() {}

    reference operator*() const
    { reset_cache_element(); return *element_cache; }
    pointer operator->() const
    { reset_cache_element(); return &*element_cache; }

    Self& operator++() noexcept { ++it; return *this; }
    Self operator++(int) noexcept {
        Self tmp(*this);
        element_cache.reset(); ++it;
        return tmp;
    }

    friend bool operator==(const Self& x, const Self& y) noexcept { return x.it == y.it; }
    friend bool operator!=(const Self& x, const Self& y) noexcept { return x.it != y.it; }

private:
    void reset_cache_element() const {
        element_cache.emplace(ReturnType{it->first, it->second});
    }

    MapIteratorType it;
    mutable std::optional<ReturnType> element_cache;
};

template <typename T>
struct MatrixDefaultGenerator {
    constexpr T operator()() const { return T{}; }
};

template<typename T, size_t Dim = 2, typename Default = MatrixDefaultGenerator<T>>
class Matrix {
public:
    using Key = typename MatrixType<T, Dim>::Key;
    using Hash = typename MatrixType<T, Dim>::Hash;
    using Map = typename MatrixType<T, Dim>::Map;
    using MatrixIterator = MatrixIteratorImpl<T, Dim, false>;
    using MatrixIteratorConst = MatrixIteratorImpl<T, Dim, true>;

    auto operator[](typename Key::base_type cur_idx) {
        assert(cur_idx != Key::BAD_IDX);
        if constexpr (Dim == 1) {
            Key key;
            key.Set(0, cur_idx);
            return ElementProxyImpl<T, Dim, Default, false>(map_, key);
        } else {
            return MatrixSquareBracketsFirst<T, Dim, Default, false>(map_, cur_idx);
        }
    }

    auto operator[](typename Key::base_type cur_idx) const {
        assert(cur_idx != Key::BAD_IDX);
        if constexpr (Dim == 1) {
            Key key;
            key.Set(0, cur_idx);
            return ElementProxyImpl<T, Dim, Default, true>(map_, key);
        } else {
            return MatrixSquareBracketsFirst<T, Dim, Default, true>(map_, cur_idx);
        }
    }

    MatrixIterator begin() noexcept { return MatrixIterator(map_.begin()); }
    MatrixIterator end() noexcept { return MatrixIterator(map_.end()); }
    MatrixIteratorConst begin() const noexcept { return MatrixIteratorConst(map_.cbegin()); }
    MatrixIteratorConst end() const noexcept { return MatrixIteratorConst(map_.cend()); }
    MatrixIteratorConst cbegin() const noexcept { return MatrixIteratorConst(map_.cbegin()); }
    MatrixIteratorConst cend() const noexcept { return MatrixIteratorConst(map_.cend()); }

    size_t size() const noexcept { return map_.size(); }
    bool contains(const Key& key) const noexcept {
        assert(key.isValid());
        return map_.find(key) != map_.end();
    }
    auto find(const Key& key) noexcept {
        assert(key.isValid());
        return MatrixIterator(map_.find(key));
    }
    const auto find(const Key& key) const noexcept {
        assert(key.isValid());
        return MatrixIteratorConst(map_.find(key));
    }
    size_t erase(const Key& key) noexcept {
        assert(key.isValid());
        return map_.erase(key);
    }

    size_t get_dimensions() const noexcept { return Dim; }
    static Default get_default_value() { return Default{}; }

private:
    Map map_;
};

} /* matrix */
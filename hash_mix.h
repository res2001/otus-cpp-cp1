#include <cstddef>
#include <cstdint>
#include <climits>
#include <functional>

namespace hash {
/* boost::hash_combine */

namespace detail {

template<std::size_t Bits> struct hash_mix_impl;

template<> struct hash_mix_impl<64>
{
    inline static uint64_t fn(uint64_t x)
    {
        const uint64_t m = (uint64_t(0xe9846af) << 32) + 0x9b1a615d;

        x ^= x >> 32;
        x *= m;
        x ^= x >> 32;
        x *= m;
        x ^= x >> 28;
        return x;
    }
};

template<> struct hash_mix_impl<32>
{
    static uint32_t fn(uint32_t x)
    {
        uint32_t const m1 = 0x21f0aaad;
        uint32_t const m2 = 0x735a2d97;

        x ^= x >> 16;
        x *= m1;
        x ^= x >> 15;
        x *= m2;
        x ^= x >> 15;

        return x;
    }
};

inline size_t hash_mix(size_t v)
{
    return hash_mix_impl<sizeof(size_t) * CHAR_BIT>::fn( v );
}

} /* detail */

template <class T>
inline void hash_combine(size_t& seed, const T& v )
{
    seed = detail::hash_mix(seed + 0x9e3779b9 + std::hash<T>()(v));
}

} /* hash */
#pragma once
#include <boost/crc.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <iomanip>

namespace md5 = boost::uuids::detail;

struct IBlockHash {
    virtual ~IBlockHash() = default;
    virtual bool is_equal(const IBlockHash& other) const noexcept = 0;
    virtual std::string to_string() const = 0;
    bool operator==(const IBlockHash& rhs) const noexcept { return is_equal(rhs); }
    bool operator!=(const IBlockHash& rhs) const noexcept { return !is_equal(rhs); }
};

class Crc32Hash : public IBlockHash {
    uint32_t m_hash_value;

public:
    Crc32Hash() : m_hash_value(0) {}
    explicit Crc32Hash(uint32_t value) : m_hash_value(value) {}

    bool is_equal(const IBlockHash& other) const noexcept override {
        if (const auto* casted = dynamic_cast<const Crc32Hash*>(&other)) {
            return m_hash_value == casted->m_hash_value;
        }
        return false; 
    }

    std::string to_string() const override {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(8) << m_hash_value;
        return ss.str();
    }
};

class Md5Hash : public IBlockHash {
    md5::md5::digest_type m_digest;
public:
    Md5Hash() : m_digest() {}
    explicit Md5Hash(const md5::md5::digest_type& digest) {
        std::copy(std::begin(digest), std::end(digest), m_digest);
    }

    bool is_equal(const IBlockHash& other) const noexcept override {
        if (const auto* casted = dynamic_cast<const Md5Hash*>(&other)) {
            return std::equal(std::begin(m_digest), std::end(m_digest), std::begin(casted->m_digest));
        }
        return false;
    }

    std::string to_string() const override {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0');
        auto* bytes = reinterpret_cast<const unsigned char*>(m_digest);
        for (size_t i = 0; i < sizeof(m_digest); ++i) {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        return ss.str();
    }
};

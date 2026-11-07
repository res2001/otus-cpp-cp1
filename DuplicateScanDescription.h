#pragma once
#include "fwd.h"
#include "hash_mix.h"
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <fstream>
#include <functional>

struct SubDirectoryDescription {
    const std::string subdir;
    const ssize_t base_dir_index;

    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T&&, std::string>>>
    SubDirectoryDescription(ssize_t base_dir_index_, T&& subdir_)
    : subdir(std::forward<T>(subdir_)), base_dir_index(base_dir_index_)
    {
        if (base_dir_index < 0)
            throw std::invalid_argument("SubDirectoryDescription: base_dir index is wrong");
    }

    // Оператор равенства обязателен для std::unordered_set
    bool operator==(const SubDirectoryDescription& rhs) const {
        return base_dir_index == rhs.base_dir_index && subdir == rhs.subdir;
    }
};

// Хэш-функция для SubDirectoryData
struct SubDirectoryHash {
    size_t operator()(const SubDirectoryDescription& k) const
    {
        const size_t h1 = std::hash<size_t>{}(k.base_dir_index);
        const size_t h2 = std::hash<std::string>{}(k.subdir);
        size_t seed = 0;
        hash::hash_combine(seed, h1);
        hash::hash_combine(seed, h2);
        return seed;
    }
};

struct SameFileDescription {
    const SubDirectoryDescription& subdesc;
    std::string filename;

    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T&&, std::string>>>
    SameFileDescription(const SubDirectoryDescription& subdesc_, T&& filename_)
    : subdesc(subdesc_), filename(std::forward<T>(filename_))
    {
        if (filename.size() == 0)
            throw std::invalid_argument("SameFileDescription: filename is empty");
    }
};
using SameFileList = std::list<SameFileDescription>;
using HashBlockType = std::unique_ptr<IBlockHash>;
using HashBlockContainer = std::vector<HashBlockType>;

struct FileDescription {
    // SameFileDescription file;
    // uint64_t total_size = 0;
    HashBlockContainer block_hashes;
    SameFileList dublicate;
    // std::ifstream stream;
    bool is_fully_read = false;

    template <typename T, typename = std::enable_if_t<std::is_same_v<T, SameFileDescription>>>
    FileDescription(T&& sfd) : block_hashes(), dublicate{std::forward<T>(sfd)} {}

    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T&&, std::string>>>
    FileDescription(const SubDirectoryDescription& subdesc_, T&& filename_)
    : block_hashes(), dublicate(1, {subdesc_, filename_}) {}
};

using BaseDirContainer = std::vector<std::string>;
using SubdirContainer = std::unordered_set<SubDirectoryDescription, SubDirectoryHash>;
using FileContainer = std::unordered_multimap<uintmax_t, FileDescription>;                          /* Key - file size */
using DublicateContainer = std::unordered_set<FileDescription*>;
// std::vector<std::reference_wrapper<FileDescription>>;

struct ScanDescription {
    const ParametersType& param;
    SubdirContainer subdir;
    FileContainer files;
    DublicateContainer double_files;

    ScanDescription(const ParametersType& param_) : param(param_) {}
    std::string GetFullPath(const SameFileDescription& sfd) const
    {
        assert(sfd.subdesc.base_dir_index >= 0 && static_cast<size_t>(sfd.subdesc.base_dir_index) < param.scan_dir.size());
        auto& base = param.scan_dir[sfd.subdesc.base_dir_index];
        if (!sfd.subdesc.subdir.empty())
            return base + "/" + sfd.subdesc.subdir + "/" + sfd.filename;

        return base + "/" + sfd.filename;
    }

    template <typename T, typename = std::enable_if_t<std::is_convertible_v<T&&, std::string>>>
    const SubDirectoryDescription& AddSubdir(ssize_t base_dir_index_, T&& subdir_)
    {
        auto [it, ins] = subdir.emplace(base_dir_index_, std::forward<T>(subdir_));
        return *it;
    }

    template <
        typename T1, typename T2, 
        typename = std::enable_if_t<std::is_convertible_v<T1&&, std::string>>,
        typename = std::enable_if_t<std::is_convertible_v<T2&&, std::string>>
    >
    void AddFile(ssize_t base_dir_index_, T1&& subdir_, T2&& filename_, uintmax_t file_size_)
    {
        assert(base_dir_index_ < param.scan_dir.size() && file_size_ > 0);
        const SubDirectoryDescription& subdesc = AddSubdir(base_dir_index_, std::forward<T1>(subdir_));
    }
};

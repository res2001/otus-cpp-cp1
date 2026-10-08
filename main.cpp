#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <charconv>
#include <utility>
#include <chrono>
#include <fstream>
#include <memory>
#include <cstdlib>
#include <cstdint>
#include <cassert>

struct CommandContainer {
    using cmd_container = std::vector<std::string>;

    CommandContainer() : timestump_(0), store_() {}

    void reserve(size_t size) { store_.reserve(size); }
    void push_back(std::string str) {
        if (store_.size() == 0)
            set_timestump();
        store_.push_back(std::move(str));
    }
    void clear() { store_.clear(); timestump_ = 0; }
    size_t size() const noexcept { return store_.size(); }
    int64_t timestump() const noexcept { return timestump_; }

    auto begin() noexcept { return store_.begin(); }
    auto end() noexcept { return store_.end(); }
    auto begin() const noexcept { return store_.cbegin(); }
    auto end() const noexcept { return store_.cend(); }
    auto cbegin() const noexcept { return store_.cbegin(); }
    auto cend() const noexcept { return store_.cend(); }

    void print_bulk(std::ostream& stream) const {
        if (store_.size() == 0)
            return;

        bool is_not_first = false;
        for (auto e: store_) {
            stream << (is_not_first ? ", " : "") << e;
            is_not_first = true;
        }
        stream << std::endl;
    }

private:
    void set_timestump() {
        auto now = std::chrono::system_clock::now();
        auto now_sec = std::chrono::time_point_cast<std::chrono::seconds>(now);
        timestump_ = now_sec.time_since_epoch().count();
    }

    int64_t timestump_;
    cmd_container store_;
};

struct IHandle {
    virtual ~IHandle() = default;
    virtual void handle(const CommandContainer& store) = 0;
};

struct CmdConsoleOutput : public IHandle {
    CmdConsoleOutput() = default;
    virtual ~CmdConsoleOutput() = default;
    void handle(const CommandContainer& store) override {
        std::cout << "bulk: ";
        store.print_bulk(std::cout);
    }
};

struct CmdFileOutput : public IHandle {
    CmdFileOutput() = default;
    virtual ~CmdFileOutput() = default;
    void handle(const CommandContainer& store) override {
        if (store.size() == 0)
            return;

        std::string fileName = "bulk" + std::to_string(store.timestump()) + ".log";
        std::ofstream outFile(fileName);
        if (outFile.is_open()) {
            store.print_bulk(outFile);
            outFile.close();
        }
    }
};

struct ChainOfResponsibility {
    using type = IHandle;
    using pointer = std::unique_ptr<IHandle>;
    using store_type = std::vector<pointer>;

    ChainOfResponsibility(size_t reserve = 2) : handlers() { handlers.reserve(reserve); }

    void handle(CommandContainer& store) {
        if (store.size() == 0)
            return;

        for(auto& h: handlers)
            h->handle(store);
        store.clear();
    }

    void addHandler(pointer ptr) {
        handlers.push_back(std::move(ptr));
    }
  
private:
    store_type handlers;
};

int parse_args(int argc, const char* argv[]) {
    if (argc != 2)
        return -1;

    std::string_view str(argv[1]);
    char* end = nullptr;
    const int i = std::strtol(str.cbegin(), & end, 0);
    return i > 0 && end == str.cend() ? i : -1;
}

int main(int argc, const char* argv[]) {
    const int n = parse_args(argc, argv);
    if (n <= 0) {
        std::cerr << "Usage: " << argv[0] << " <number of commands for batch processing>" << std::endl;
        return EXIT_FAILURE;
    }

    ChainOfResponsibility cor(static_cast<size_t>(n));
    cor.addHandler(std::make_unique<CmdConsoleOutput>());
    cor.addHandler(std::make_unique<CmdFileOutput>());

    CommandContainer cmd_store;
    std::string cmd;
    cmd.reserve(1024);
    uint32_t nested_count = 0;
    while (std::getline(std::cin, cmd)) {
        if (cmd.size() == 0)
            continue;

        if (cmd.size() == 1) {
            if (cmd[0] == '{') {
                if (++nested_count == 1)
                    cor.handle(cmd_store);
                continue;
            } else if (cmd[0] == '}') {
                if (nested_count > 0) {
                    if (--nested_count == 0)
                        cor.handle(cmd_store);
                    continue;
                } else {
                    std::cerr << "Sequence error on command '}'" << std::endl;
                    return EXIT_FAILURE;
                }
            }
        }

        cmd_store.push_back(std::move(cmd));
        if (nested_count == 0 && cmd_store.size() == static_cast<size_t>(n)) {
            cor.handle(cmd_store);
        }
    }

    if (!std::cin.eof() && std::cin.fail()) {
        std::cerr << "std::cin read error" << std::endl;
        return EXIT_FAILURE;
    }

    if (nested_count == 0)
        cor.handle(cmd_store);

    return EXIT_SUCCESS;
}
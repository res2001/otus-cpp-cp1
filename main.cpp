#include "fwd.h"
#include <charconv>
#include <utility>
#include <chrono>
#include <fstream>
#include <queue>
#include <array>
#include <condition_variable>
#include <memory>
#include <thread>
#include <atomic>
#include <cstdlib>
#include <locale>

#ifdef _WIN32
    #include <windows.h>
#endif

CmdLineParameters params;
std::atomic_bool finish(false);

template<typename T>
struct Queue {
    using element_type = std::shared_ptr<T>;
    using queue_type = std::queue<element_type>;

private:
    std::mutex mx;
    std::condition_variable cv;
    queue_type q;

public:
    Queue() = default;
    ~Queue() = default;
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    void push(element_type e)
    { 
        std::unique_lock lk(mx);
        q.push(std::move(e));
        cv.notify_one();
    }

    element_type pop()
    {
        std::unique_lock lk(mx);
        while (q.empty()) {
            if (finish.load(std::memory_order_relaxed))
                return nullptr;

            // cv.wait_for(lk, std::chrono::milliseconds(100), [this] { return !q.empty() || finish.load(std::memory_order_relaxed); });
            cv.wait(lk);
        }

        element_type e = std::move(q.front());
        q.pop();
        return e;
    }

    bool empty() noexcept
    {
        std::unique_lock lk(mx);
        return q.empty();
    }
    void notify_all() noexcept { cv.notify_all(); }
};

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

using QueueCommandType = Queue<CommandContainer>;
static QueueCommandType qfile;
static QueueCommandType qlog;

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

class CmdFileOutput : public IHandle {
    uint32_t fnum = 0;
    uint32_t fsuffix = 1;
    int64_t prev_timestump = 0;
public:
    CmdFileOutput() = default;
    CmdFileOutput(uint32_t fnum_) : fnum(fnum_) {};
    virtual ~CmdFileOutput() = default;
    void handle(const CommandContainer& store) override {
        if (store.size() == 0)
            return;

        const int64_t timestump = store.timestump();
        if (timestump == prev_timestump)
            ++fsuffix;
        else {
            fsuffix = 1;
            prev_timestump = timestump;
        }

        std::string fileName = "bulk" + std::to_string(timestump) + "-" + std::to_string(fnum) + std::to_string(fsuffix) + ".log";
        std::ofstream outFile(fileName, std::ios::trunc);
        if (outFile.is_open()) {
            store.print_bulk(outFile);
            outFile.close();
        }
    }
};

class CommandData {
    CommandContainer cmd_store;
    const std::string& file;
    input_command_type async_reader = ASYNC_TYPE_INVALID;
    uint32_t nested_count = 0;
    bool is_good = true;

public:
    CommandData(const std::string& file_name) : cmd_store(), file(file_name)
    {
        async_reader = connect(file_name.c_str());
        if (async_reader == ASYNC_TYPE_INVALID)
            throw std::invalid_argument("Async connect is failed! File not found or other file error\n");
    }
    ~CommandData() { disconnect(async_reader); }
    bool is_read_good() const noexcept { return is_good; }
    bool ReadCommand(char* buf, size_t buf_size)
    {
        bool is_handle = false;
        do {
            const long res = receive(async_reader, buf, buf_size);
            if (res < 0) {
                if (res == -1)
                    std::cerr << "Read error from file: " << file << std::endl;
                is_good = false;
                break;
            }

            if (res == 0)
                break;

            if (res == 1) {
                if (buf[0] == '{') {
                    if (++nested_count == 1)
                        is_handle = true;
                    break;
                } else if (buf[0] == '}') {
                    if (nested_count > 0) {
                        if (--nested_count == 0)
                            is_handle = true;
                        break;
                    } else {
                        std::cerr << "Sequence error on command '}' in file: " << file << std::endl;
                        is_good = false;
                        break;
                    }
                }
            }

            cmd_store.push_back(buf);
            is_handle = nested_count == 0 && cmd_store.size() == params.number_of_command;
        
        } while(false);

        if (cmd_store.size() > 0 && (is_handle || !is_good)) {
            QueueCommandType::element_type sptr = std::make_shared<CommandContainer>(std::move(cmd_store));
            qfile.push(sptr);
            qlog.push(std::move(sptr));
        }
        return is_good;
    }
};

static void initialize_console()
{
#ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    
    // Включаем поддержку ANSI-последовательностей (виртуальный терминал), 
    // чтобы консоль Windows 10/11 адекватно принимала UTF-8 потоки
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    std::setlocale(LC_ALL, "");
#endif
}

static void thread_handle(IHandle& handle, QueueCommandType& q)
{
    while(!q.empty() || !finish.load(std::memory_order_relaxed)) {
        auto sptr = q.pop();
        if (sptr.get() != nullptr) {
            handle.handle(*sptr);
        }
    }
}

static void thread_reader(CommandData& cmd_data)
{
    std::vector<char> cmd(params.cmd_len_max + 1, 0);
    while (cmd_data.is_read_good())
        cmd_data.ReadCommand(cmd.data(), cmd.size());
}

int main(int argc, const char* argv[]) {
    initialize_console();

     try {
        if (!parse_args(argc, argv))
            return EXIT_FAILURE;
    } catch(const std::exception& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    std::vector<std::thread> thr;
    /* [0] - console output; next thread_num - file output; next files.size() - readers */
    thr.reserve(1 + params.thread_num + params.files.size());

    std::vector<CommandData> cmd_data_vec;
    cmd_data_vec.reserve(params.files.size());

    CmdConsoleOutput cmd_con_handle;
    std::vector<CmdFileOutput> cmd_file_handle;
    cmd_file_handle.reserve(params.thread_num);

    thr.emplace_back(thread_handle, std::ref(cmd_con_handle), std::ref(qlog));
    uint32_t i;
    for (i = 0; i < params.thread_num; ++i) {
        cmd_file_handle.emplace_back(i + 1);
        thr.emplace_back(thread_handle, std::ref(cmd_file_handle.back()), std::ref(qfile));
    }
    for (const auto& file: params.files) {
        cmd_data_vec.emplace_back(file);
        thr.emplace_back(thread_reader, std::ref(cmd_data_vec.back()));
    }

    /* wait file reader */
    for (i = 1 + params.thread_num; i < thr.size(); ++i)
        thr[i].join();

    /* signal to finish */
    finish.store(true, std::memory_order_relaxed);
    qfile.notify_all();
    qlog.notify_all();
    
    /* wait output thread */
    for (i = 0; i <= params.thread_num; ++i)
        thr[i].join();

    return EXIT_SUCCESS;
}
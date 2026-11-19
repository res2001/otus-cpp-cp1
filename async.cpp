#include "async.h"
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cassert>

class CommandFileReader {
    std::istream* fs = nullptr;
    bool is_ref = false;

public:
    CommandFileReader(std::istream& stream) : fs(& stream), is_ref(true) {}
    CommandFileReader(std::string file_name) : fs(new std::ifstream(file_name)) {}
    ~CommandFileReader() { if (!is_ref) delete fs; }
    int StreamIsGood() ASYNC_NOEXCEPT {
        if (fs == nullptr) return -1;
        if (fs->good()) return 0;
        if(fs->eof()) return -2;
        return -1;
    }
    long ReadCommand(char* buf, size_t buf_size) ASYNC_NOEXCEPT
    {
        assert(buf && buf_size > 0);
        long ret = StreamIsGood();
        if (ret == 0) {
            fs->getline(buf, buf_size);
            const long read_size = fs->gcount();
            if (read_size > 0)
                ret = read_size - 1;
            else
                ret = fs->eof() ? -2 : -1;
        }
        return ret;
    }
};

void disconnect(input_command_type in) ASYNC_NOEXCEPT
{
    if (in != nullptr) {
        CommandFileReader* cmd = reinterpret_cast<CommandFileReader*>(in);
        delete cmd;
    }
}

input_command_type connect(const char* input_file) ASYNC_NOEXCEPT
{
    if (input_file == nullptr || std::strlen(input_file) == 0)
        return nullptr;

    CommandFileReader* cmd = nullptr;
    try {
        if (std::strcmp(input_file, ASYNC_STDIN_NAME) == 0) {
            cmd = new CommandFileReader(std::cin);
        } else {
            cmd = new CommandFileReader(input_file);
        }
    } catch(std::exception& e) {
        std::cerr << e.what() << std::endl;
        return nullptr;
    }

    return cmd;
}

int file_is_good(input_command_type in) ASYNC_NOEXCEPT
{
    if (in == nullptr)
        return -1;

    CommandFileReader* cmd = reinterpret_cast<CommandFileReader*>(in);
    return cmd->StreamIsGood();
}

long receive(input_command_type in, char* buf, size_t buf_size) ASYNC_NOEXCEPT
{
    if (in == nullptr || buf == nullptr || buf_size == 0)
        return -1;

    CommandFileReader* cmd = reinterpret_cast<CommandFileReader*>(in);
    return cmd->ReadCommand(buf, buf_size);
}

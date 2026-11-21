#include "async.h"
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <limits>
#include <stdexcept>
#include <cassert>

struct CommandFileReader {
    using stream_deleter_type = void(*)(std::istream*);
    CommandFileReader(std::istream& stream, stream_deleter_type deleter = [](std::istream*){/* empty deleter */}) : fs(& stream, deleter) {}
    CommandFileReader(std::string file_name) : fs(new std::ifstream(file_name), [](std::istream* del_obj){ delete del_obj; }) {}
    int StreamIsGood() ASYNC_NOEXCEPT {
        if (fs == nullptr) return -1;
        if (fs->good()) return 0;
        if(fs->eof()) return -2;
        return -1;
    }
    long ReadCommand(char* buf, size_t buf_size) ASYNC_NOEXCEPT
    {
        if (buf == NULL || buf_size == 0)
            return -1;
        long ret = 0;
        fs->getline(buf, buf_size);
        const long read_size = fs->gcount();
        if (read_size > 0) {
            ret = read_size - 1;
            if (fs->fail() && !fs->bad()) {
                std::cerr << "\nBuffer overflow. Truncate commands!\n";
                fs->clear();
                fs->ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        } else if (fs->eof())
            ret = -2;
        else if (fs->bad())
            ret = -1;
        return ret;
    }

private:
    std::unique_ptr<std::istream, stream_deleter_type> fs;
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

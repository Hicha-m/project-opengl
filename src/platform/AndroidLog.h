#pragma once
#ifdef __ANDROID__
#include <SDL3/SDL.h>
#include <iostream>
#include <streambuf>
#include <string>

// Android does not expose C++ stdout/stderr in logcat. Keep complete lines together.
class AndroidLogBuffer : public std::streambuf {
public:
    explicit AndroidLogBuffer(std::ostream& stream) : stream(stream), previous(stream.rdbuf(this)) {}
    ~AndroidLogBuffer() override { stream.rdbuf(previous); }
protected:
    int_type overflow(int_type value) override {
        if (traits_type::eq_int_type(value, traits_type::eof())) return traits_type::not_eof(value);
        const char character = traits_type::to_char_type(value);
        if (character == '\n') {
            SDL_Log("%s", line.c_str());
            line.clear();
        } else line += character;
        return value;
    }
    int sync() override { return 0; }
private:
    std::ostream& stream;
    std::streambuf* previous;
    std::string line;
};
#endif

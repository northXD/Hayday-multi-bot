#pragma once
#include <string>

// Derleme Zamanı (Compile-Time) Metin Şifreleyici
template <unsigned size, char key, unsigned lineno>
struct xor_str_impl {
    char data[size];
    constexpr xor_str_impl(const char* str) : data{ 0 } {
        for (unsigned i = 0; i < size; ++i) {
            data[i] = str[i] ^ key;
        }
    }
};

// Kullanacağımız Makro
#define XorStr(s) ([]() -> std::string { \
    constexpr auto key = __LINE__ % 255 + 1; \
    constexpr xor_str_impl<sizeof(s), key, __LINE__> crypted(s); \
    std::string res(sizeof(s) - 1, '\0'); \
    for (size_t i = 0; i < sizeof(s) - 1; ++i) { \
        res[i] = crypted.data[i] ^ key; \
    } \
    return res; \
}())

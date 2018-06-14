//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#pragma once

#include <codecvt>
#include <cuchar>
#include <string>
#include <vector>

namespace Microsoft
{
namespace MSR
{
namespace CNTK
{

// Compares two ASCII strings ignoring the case.
// TODO: Should switch to boost, boost::iequal should be used instead.
// TODO: we already have EqualCI() in Basics.h which does the same thing.
template <class TElement>
inline bool AreEqualIgnoreCase(
    const std::basic_string<TElement, char_traits<TElement>, allocator<TElement>>& s1,
    const std::basic_string<TElement, char_traits<TElement>, allocator<TElement>>& s2)
{
    if (s1.size() != s2.size())
    {
        return false;
    }

    return std::equal(s1.begin(), s1.end(), s2.begin(), [](const TElement& a, const TElement& b) {
        return std::tolower(a) == std::tolower(b);
    });
}

template <class TString>
inline bool AreEqualIgnoreCase(
    const TString& s1,
    const typename TString::value_type* s2pointer)
{
    return AreEqualIgnoreCase(s1, TString(s2pointer));
}

template <class TString>
inline bool AreEqualIgnoreCase(
    const typename TString::value_type* s1pointer,
    const TString& s2)
{
    return AreEqualIgnoreCase(TString(s1pointer), s2);
}

// UTF8 is multibyte, so not returning std::basic_string-based type to avoid potential confusion
static std::vector<unsigned char> ToUTF8(char const* str, bool isFixedWidth = true);
static std::vector<unsigned char> ToUTF8(wchar_t const* str);
static std::vector<unsigned char> ToUTF8(unsigned char const* str);
static std::vector<unsigned char> ToUTF8(char16_t const* str);
static std::vector<unsigned char> ToUTF8(char32_t const* str);
static std::vector<unsigned char> ToUTF8(std::string const& value, bool isFixedWidth = true);
static std::vector<unsigned char> ToUTF8(std::wstring const& value);
static std::vector<unsigned char> ToUTF8(std::u16string const& value);
static std::vector<unsigned char> ToUTF8(std::u32string const& value);

static std::u16string ToUTF16(char const* str, bool isFixedWidth = true);
static std::u16string ToUTF16(wchar_t const* str);
static std::u16string ToUTF16(unsigned char const* str);
static std::u16string ToUTF16(char16_t const* str);
static std::u16string ToUTF16(char32_t const* str);
static std::u16string ToUTF16(std::string const& value, bool isFixedWidth = true);
static std::u16string ToUTF16(std::wstring const& value);
static std::u16string ToUTF16(std::vector<unsigned char> const& value); // UTF8
static std::u16string ToUTF16(std::u32string const& value);

static std::u32string ToUTF32(char const* str, bool isFixedWidth = true);
static std::u32string ToUTF32(wchar_t const* str);
static std::u32string ToUTF32(unsigned char const* str);
static std::u32string ToUTF32(char16_t const* str);
static std::u32string ToUTF32(char32_t const* str);
static std::u32string ToUTF32(std::string const& value, bool isFixedWidth = true);
static std::u32string ToUTF32(std::wstring const& value);
static std::u32string ToUTF32(std::vector<unsigned char> const& value);
static std::u32string ToUTF32(std::u16string const& value);

static std::string ToString(char const* str, bool isFixedWidth = true);
static std::string ToString(wchar_t const* str);
static std::string ToString(unsigned char const* str);
static std::string ToString(char16_t const* str);
static std::string ToString(char32_t const* str);
static std::string ToString(std::string const&, bool isFixedWidth = true);
static std::string ToString(std::wstring const& value);
static std::string ToString(std::vector<unsigned char> const& value);
static std::string ToString(std::u16string const& value);
static std::string ToString(std::u32string const& value);

static std::wstring ToWString(char const* str, bool isFixedWidth = true);
static std::wstring ToWString(wchar_t const* str);
static std::wstring ToWString(unsigned char const* str);
static std::wstring ToWString(char16_t const* str);
static std::wstring ToWString(char32_t const* str);
static std::wstring ToWString(std::string const& value, bool isFixedWidth = true);
static std::wstring ToWString(std::vector<unsigned char> const& value);
static std::wstring ToWString(std::u16string const& value);
static std::wstring ToWString(std::u32string const& value);

// Convert a multibyte string to a std::string without applying any conversion.
//
// Note that these methods should only be used when migrating existing code, as they
// create strings that are ambiguous to use when passed from function to function
// (are they fixed width or not?).
static std::string ToLegacyString(unsigned char const* str);
static std::string ToLegacyString(std::vector<unsigned char> const& value);

// ----------------------------------------------------------------------
// |
// |  Implementation
// |
// ----------------------------------------------------------------------
namespace
{

template <typename T>
T&& SingleConversionImpl_ApplySuffix(T&& value)
{
    return std::forward<T>(value);
}

static inline std::vector<unsigned char>&& SingleConversionImpl_ApplySuffix(std::vector<unsigned char>&& value)
{
    // Ensure that the memory is null terminated
    if (value.empty() == false && *value.crbegin() != 0)
        value.push_back(0);

    return std::move(value);
}

template <typename ReturnT, typename T>
ReturnT SimpleConversionImpl(T const* str)
{
    static_assert(sizeof(T) <= sizeof(typename ReturnT::value_type), "Must convert from a character set that is smaller or equal to the size of the destination character set");

    if (str == nullptr)
        return ReturnT();

    ReturnT result;

    while (*str)
    {
        result.push_back(*str);
        ++str;
    }

    return SingleConversionImpl_ApplySuffix(std::move(result));
}

template <typename T, typename ConvertFuncT>
std::vector<unsigned char> ToUTF8Impl(T const* str, ConvertFuncT const& func)
{
    if (str == nullptr)
        return std::vector<unsigned char>();

    std::vector<unsigned char> buffer;
    std::string oneChar;

    oneChar.resize(MB_CUR_MAX);

    std::mbstate_t state{}; // Initialize to zeros

    while (true)
    {
        size_t const result(func(const_cast<char*>(oneChar.data()), *str, &state));

        if (result == static_cast<size_t>(-1))
            throw std::invalid_argument("");

        size_t const prevBufferSize(buffer.size());

        buffer.resize(prevBufferSize + result);
        memcpy(buffer.data() + prevBufferSize, oneChar.data(), result);

        if (*str == 0)
            break;

        ++str;
    }

    return buffer;
}

template <typename ReturnT, typename ConvertFuncT>
ReturnT UTF8ToUTFXXImpl(char const* str, ConvertFuncT const& func)
{
    if (str == nullptr)
        return ReturnT();

    char const* const pEnd(
        [str]() mutable {
            while (*str)
                ++str;

            return str + 1; // Move past the null
        }());

    ReturnT buffer;
    std::mbstate_t state{}; // Initialize to zeros
    typename ReturnT::value_type c;

    while (std::size_t result = func(&c, str, pEnd - str, &state))
    {
        if (result == static_cast<size_t>(-1) || result == static_cast<size_t>(-2))
            throw std::invalid_argument("");

        buffer.push_back(c);

        if (result != static_cast<size_t>(-3))
            str += result;
    }

    return buffer;
}

template <typename T>
std::vector<unsigned char> ToUTF8_WcharSize(T const* str, std::integral_constant<size_t, 2>)
{
    static_assert(std::is_same<T, wchar_t>::value, "Template in support of SFINAE");

    // Convert from UCS-2 to UTF8 using a UTF16 algorithm. This is safe since UCS-2 is a subset of UTF16
    return ToUTF8Impl(reinterpret_cast<char16_t const*>(str), &std::c16rtomb);
}

template <typename T>
std::vector<unsigned char> ToUTF8_WcharSize(T const* str, std::integral_constant<size_t, 4>)
{
    static_assert(std::is_same<T, wchar_t>::value, "Template in support of SFINAE");

    // Convert from UCS-4 to UTF8 using a UTF32 algorithm. This is safe since UCS-4 == UTF32
    return ToUTF8Impl(reinterpret_cast<char32_t const*>(str), &std::c32rtomb);
}

template <typename T>
std::u16string ToUTF16_WcharSize(T const* str, std::integral_constant<size_t, 2>)
{
    static_assert(std::is_same<T, wchar_t>::value, "Template in the name of SFINAE");
    return SimpleConversionImpl<std::u16string>(str);
}

template <typename T>
std::u16string ToUTF16_WcharSize(T const* str, std::integral_constant<size_t, 4>)
{
    static_assert(std::is_same<T, wchar_t>::value, "Template in the name of SFINAE");
    return ToUTF16(ToUTF8(str));
}

} // anonymous namespace

static inline std::vector<unsigned char> ToUTF8(char const* str, bool isFixedWidth /* =true */)
{
    return isFixedWidth ? SimpleConversionImpl<std::vector<unsigned char>>(str) : ToUTF8(reinterpret_cast<unsigned char const*>(str));
}
static inline std::vector<unsigned char> ToUTF8(wchar_t const* str)
{
    return ToUTF8_WcharSize(str, std::integral_constant<size_t, sizeof(wchar_t)>());
}
static inline std::vector<unsigned char> ToUTF8(unsigned char const* str)
{
    return SimpleConversionImpl<std::vector<unsigned char>>(str);
}
static inline std::vector<unsigned char> ToUTF8(char16_t const* str)
{
    return ToUTF8Impl(str, &std::c16rtomb);
}
static inline std::vector<unsigned char> ToUTF8(char32_t const* str)
{
    return ToUTF8Impl(str, &std::c32rtomb);
}
static inline std::vector<unsigned char> ToUTF8(std::string const& value, bool isFixedWidth /* =true */)
{
    return ToUTF8(value.c_str(), isFixedWidth);
}
static inline std::vector<unsigned char> ToUTF8(std::wstring const& value)
{
    return ToUTF8(value.c_str());
}
static inline std::vector<unsigned char> ToUTF8(std::u16string const& value)
{
    return ToUTF8(value.c_str());
}
static inline std::vector<unsigned char> ToUTF8(std::u32string const& value)
{
    return ToUTF8(value.c_str());
}

static inline std::u16string ToUTF16(char const* str, bool isFixedWidth /* =true */)
{
    return isFixedWidth ? SimpleConversionImpl<std::u16string>(str) : ToUTF16(reinterpret_cast<unsigned char const*>(str));
}
static inline std::u16string ToUTF16(wchar_t const* str)
{
    return ToUTF16_WcharSize(str, std::integral_constant<size_t, sizeof(wchar_t)>());
}
static inline std::u16string ToUTF16(unsigned char const* str)
{
    return UTF8ToUTFXXImpl<std::u16string>(reinterpret_cast<char const*>(str), &std::mbrtoc16);
}
static inline std::u16string ToUTF16(char16_t const* str)
{
    return SimpleConversionImpl<std::u16string>(str);
}
static inline std::u16string ToUTF16(char32_t const* str)
{
    return ToUTF16(ToUTF8(str));
}
static inline std::u16string ToUTF16(std::string const& value, bool isFixedWidth /* =true */)
{
    return ToUTF16(value.c_str(), isFixedWidth);
}
static inline std::u16string ToUTF16(std::wstring const& value)
{
    return ToUTF16(value.c_str());
}
static inline std::u16string ToUTF16(std::vector<unsigned char> const& value)
{
    return ToUTF16(value.data());
}
static inline std::u16string ToUTF16(std::u32string const& value)
{
    return ToUTF16(value.c_str());
}

static inline std::u32string ToUTF32(char const* str, bool isFixedWidth /* =true */)
{
    return isFixedWidth ? SimpleConversionImpl<std::u32string>(str) : ToUTF32(reinterpret_cast<unsigned char const*>(str));
}
static inline std::u32string ToUTF32(wchar_t const* str)
{
    return SimpleConversionImpl<std::u32string>(str);
}
static inline std::u32string ToUTF32(unsigned char const* str)
{
    return UTF8ToUTFXXImpl<std::u32string>(reinterpret_cast<char const*>(str), &std::mbrtoc32);
}
static inline std::u32string ToUTF32(char16_t const* str)
{
    return ToUTF32(ToUTF8(str));
}
static inline std::u32string ToUTF32(char32_t const* str)
{
    return SimpleConversionImpl<std::u32string>(str);
}
static inline std::u32string ToUTF32(std::string const& value, bool isFixedWidth /* =true */)
{
    return ToUTF32(value.c_str(), isFixedWidth);
}
static inline std::u32string ToUTF32(std::wstring const& value)
{
    return ToUTF32(value.c_str());
}
static inline std::u32string ToUTF32(std::vector<unsigned char> const& value)
{
    return ToUTF32(value.data());
}
static inline std::u32string ToUTF32(std::u16string const& value)
{
    return ToUTF32(value.c_str());
}

namespace
{

template <typename T> 
std::string ToString_WcharSize(T const* str, std::integral_constant<size_t, 4>)
{
    static_assert(std::is_same<T, wchar_t>::value, "Template in support of SFINAE");
    return ToString(reinterpret_cast<char32_t const*>(str));
}

template <typename T>
std::string ToString_WcharSize(T const* str, std::integral_constant<size_t, 2>)
{
    static_assert(std::is_same<T, wchar_t>::value, "Template in support of SFINAE");
    return ToString(ToUTF32(str));
}

template <typename T>
std::wstring ToWString_WcharSize(T const* str, std::integral_constant<size_t, 4>)
{
    static_assert(std::is_same<T, char32_t>::value, "Template in support of SFINAE");
    return reinterpret_cast<wchar_t const*>(str);
}

template <typename T>
std::wstring ToWString_WcharSize(T const* str, std::integral_constant<size_t, 2>)
{
    static_assert(std::is_same<T, char32_t>::value, "Template in support of SFINAE");

    std::wstring result;

    while (*str)
    {
        // Known lossy conversion
        result.push_back(*reinterpret_cast<wchar_t const*>(str));
        ++str;
    }

    return result;
}

} // anonymous namespace

static inline std::string ToString(char const* str, bool isFixedWidth /* =true */)
{
    if (isFixedWidth == false)
        return ToString(reinterpret_cast<unsigned char const*>(str));

    return str ? std::string(str) : std::string();
}

static inline std::string ToString(wchar_t const* str)
{
    if (str == nullptr)
        return std::string();

    return ToString_WcharSize(str, std::integral_constant<size_t, sizeof(wchar_t)>());
}

static inline std::string ToString(unsigned char const* str)
{
    return str ? ToString(ToUTF32(str)) : std::string();
}

static inline std::string ToString(char16_t const* str)
{
    return str ? ToString(ToUTF32(str)) : std::string();
}

static inline std::string ToString(char32_t const* str)
{
    if (str == nullptr)
        return std::string();

    std::string result;

    while (*str)
    {
        // Known lossy conversion
        result.push_back(*reinterpret_cast<char const*>(str));
        ++str;
    }

    return result;
}

static inline std::string ToString(std::string const& value, bool isFixedWidth /* =true */)
{
    return ToString(value.c_str(), isFixedWidth);
}

static inline std::string ToString(std::wstring const& value)
{
    return ToString(value.c_str());
}

static inline std::string ToString(std::vector<unsigned char> const& value)
{
    return ToString(value.data());
}

static inline std::string ToString(std::u16string const& value)
{
    return ToString(value.c_str());
}

static inline std::string ToString(std::u32string const& value)
{
    return ToString(value.c_str());
}

static inline std::wstring ToWString(char const* str, bool isFixedWidth /* =true */)
{
    if (isFixedWidth == false)
        return ToWString(reinterpret_cast<unsigned char const*>(str));

    return SimpleConversionImpl<std::wstring>(str);
}

static inline std::wstring ToWString(wchar_t const* str)
{
    return str ? std::wstring(str) : std::wstring();
}

static inline std::wstring ToWString(unsigned char const* str)
{
    return str ? ToWString(ToUTF32(str)) : std::wstring();
}

static inline std::wstring ToWString(char16_t const* str)
{
    return str ? ToWString(ToUTF32(str)) : std::wstring();
}

static inline std::wstring ToWString(char32_t const* str)
{
    return str ? ToWString_WcharSize(str, std::integral_constant<size_t, sizeof(wchar_t)>()) : std::wstring();
}

static inline std::wstring ToWString(std::string const& value, bool isFixedWidth /* =true */)
{
    return ToWString(value.c_str(), isFixedWidth);
}

static inline std::wstring ToWString(std::vector<unsigned char> const& value)
{
    return ToWString(value.data());
}

static inline std::wstring ToWString(std::u16string const& value)
{
    return ToWString(value.c_str());
}

static inline std::wstring ToWString(std::u32string const& value)
{
    return ToWString(value.c_str());
}

static inline std::string ToLegacyString(unsigned char const* str)
{
    if (str == nullptr)
        return std::string();

    char const* const pEnd(
        [str]() mutable {
            while (*str != 0)
                ++str;

            return reinterpret_cast<char const*>(str);
        }());

    return std::string(reinterpret_cast<char const*>(str), pEnd);
}

static inline std::string ToLegacyString(std::vector<unsigned char> const& value)
{
    return ToLegacyString(value.data());
}

}
}
}

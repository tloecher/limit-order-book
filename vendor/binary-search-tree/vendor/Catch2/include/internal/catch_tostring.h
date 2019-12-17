/*
 *  Created by Phil on 8/5/2012.
 *  Copyright 2012 Two Blue Cubes Ltd. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TWOBLUECUBES_CATCH_TOSTRING_H_INCLUDED
#define TWOBLUECUBES_CATCH_TOSTRING_H_INCLUDED


#include <vector>
#include <cstddef>
#include <type_traits>
#include <string>
#include "catch_compiler_capabilities.h"
#include "catch_stream.h"
#include "catch_interfaces_enum_values_registry.h"

#ifdef CATCH_CONFIG_CPP17_STRING_VIEW
#include <string_view>
#endif

#ifdef __OBJC__
#include "catch_objc_arc.hpp"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4180) // We attempt to stream a function (address) by const&, which MSVC complains about but is harmless
#endif

namespace Catch {
    namespace Detail {

        extern const std::string unprintableString;

        std::string rawMemoryToString( const void *object, std::size_t size );

        template<typename T>
        std::string rawMemoryToString( const T& object ) {
          return rawMemoryToString( &object, sizeof(object) );
        }

        template<typename T>
        class IsStreamInsertable {
            template<typename SS, typename TT>
            static auto test(int)
                -> decltype(std::declval<SS&>() << std::declval<TT>(), std::true_type());

            template<typename, typename>
            static auto test(...)->std::false_type;

        public:
            static const bool value = decltype(test<std::ostream, const T&>(0))::value;
        };

        template<typename E>
        std::string convertUnknownEnumToString( E e );

        template<typename T>
        typename std::enable_if<
            !std::is_enum<T>::value && !std::is_base_of<std::exception, T>::value,
        std::string>::type convertUnstreamable( T const& ) {
            return Detail::unprintableString;
        }
        template<typename T>
        typename std::enable_if<
            !std::is_enum<T>::value && std::is_base_of<std::exception, T>::value,
         std::string>::type convertUnstreamable(T const& ex) {
            return ex.what();
        }


        template<typename T>
        typename std::enable_if<
            std::is_enum<T>::value
        , std::string>::type convertUnstreamable( T const& value ) {
            return convertUnknownEnumToString( value );
        }

#if defined(_MANAGED)
        //! Convert a CLR string to a utf8 std::string
        template<typename T>
        std::string clrReferenceToString( T^ ref ) {
            if (ref == nullptr)
                return std::string("null");
            auto bytes = System::Text::Encoding::UTF8->GetBytes(ref->ToString());
            cli::pin_ptr<System::Byte> p = &bytes[0];
            return std::string(reinterpret_cast<char const *>(p), bytes->Length);
        }
#endif

    } // namespace Detail


    // If we decide for C++14, change these to enable_if_ts
    template <typename T, typename = void>
    struct StringMaker {
        template <typename Fake = T>
        static
        typename std::enable_if<::Catch::Detail::IsStreamInsertable<Fake>::value, std::string>::type
            convert(const Fake& value) {
                ReusableStringStream rss;
                // NB: call using the function-like syntax to avoid ambiguity with
                // user-defined templated operator<< under clang.
                rss.operator<<(value);
                return rss.str();
        }

        template <typename Fake = T>
        static
        typename std::enable_if<!::Catch::Detail::IsStreamInsertable<Fake>::value, std::string>::type
            convert( const Fake& value ) {
#if !defined(CATCH_CONFIG_FALLBACK_STRINGIFIER)
            return Detail::convertUnstreamable(value);
#else
            return CATCH_CONFIG_FALLBACK_STRINGIFIER(value);
#endif
        }
    };

    namespace Detail {

        // This function dispatches all stringification requests inside of Catch.
        // Should be preferably called fully qualified, like ::Catch::Detail::stringify
        template <typename T>
        std::string stringify(const T& e) {
            return ::Catch::StringMaker<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::convert(e);
        }

        template<typename E>
        std::string convertUnknownEnumToString( E e ) {
            return ::Catch::Detail::stringify(static_cast<typename std::underlying_type<E>::type>(e));
        }

#if defined(_MANAGED)
        template <typename T>
        std::string stringify( T^ e ) {
            return ::Catch::StringMaker<T^>::convert(e);
        }
#endif

    } // namespace Detail

    // Some predefined specializations

    template<>
    struct StringMaker<std::string> {
        static std::string convert(const std::string& str);
    };

#ifdef CATCH_CONFIG_CPP17_STRING_VIEW
    template<>
    struct StringMaker<std::string_view> {
        static std::string convert(std::string_view str);
    };
#endif

    template<>
    struct StringMaker<char const *> {
        static std::string convert(char const * str);
    };
    template<>
    struct StringMaker<char *> {
        static std::string convert(char * str);
    };

#ifdef CATCH_CONFIG_WCHAR
    template<>
    struct StringMaker<std::wstring> {
        static std::string convert(const std::wstring& wstr);
    };

# ifdef CATCH_CONFIG_CPP17_STRING_VIEW
    template<>
    struct StringMaker<std::wstring_view> {
        static std::string convert(std::wstring_view str);
    };
# endif

    template<>
    struct StringMaker<wchar_t const *> {
        static std::string convert(wchar_t const * str);
    };
    template<>
    struct StringMaker<wchar_t *> {
        static std::string convert(wchar_t * str);
    };
#endif

    // TBD: Should we use `strnlen` to ensure that we don't go out of the buffer,
    //      while keeping string semantics?
    template<int SZ>
    struct StringMaker<char[SZ]> {
        static std::string convert(char const* str) {
            return ::Catch::Detail::stringify(std::string{ str });
        }
    };
    template<int SZ>
    struct StringMaker<signed char[SZ]> {
        static std::string convert(signed char const* str) {
            return ::Catch::Detail::stringify(std::string{ reinterpret_cast<char const *>(str) });
        }
    };
    template<int SZ>
    struct StringMaker<unsigned char[SZ]> {
        static std::string convert(unsigned char const* str) {
            return ::Catch::Detail::stringify(std::string{ reinterpret_cast<char const *>(str) });
        }
    };

    template<>
    struct StringMaker<int> {
        static std::string convert(int value);
    };
    template<>
    struct StringMaker<long> {
        static std::string convert(long value);
    };
    template<>
    struct StringMaker<long long> {
        static std::string convert(long long value);
    };
    template<>
    struct StringMaker<unsigned int> {
        static std::string convert(unsigned int value);
    };
    template<>
    struct StringMaker<unsigned long> {
        static std::string convert(unsigned long value);
    };
    template<>
    struct StringMaker<unsigned long long> {
        static std::string convert(unsigned long long value);
    };

    template<>
    struct StringMaker<bool> {
        static std::string convert(bool b);
    };

    template<>
    struct StringMaker<char> {
        static std::string convert(char c);
    };
    template<>
    struct StringMaker<signed char> {
        static std::string convert(signed char c);
    };
    template<>
    struct StringMaker<unsigned char> {
        static std::string convert(unsigned char c);
    };

    template<>
    struct StringMaker<std::nullptr_t> {
        static std::string convert(std::nullptr_t);
    };

    template<>
    struct StringMaker<float> {
        static std::string convert(float value);
        static int precision;
    };

    template<>
    struct StringMaker<double> {
        static std::string convert(double value);
        static int precision;
    };

    template <typename T>
    struct StringMaker<T*> {
        template <typename U>
        static std::string convert(U* p) {
            if (p) {
                return ::Catch::Detail::rawMemoryToString(p);
            } else {
                return "nullptr";
            }
        }
    };

    template <typename R, typename C>
    struct StringMaker<R C::*> {
        static std::string convert(R C::* p) {
            if (p) {
                return ::Catch::Detail::rawMemoryToString(p);
            } else {
                return "nullptr";
            }
        }
    };

#if defined(_MANAGED)
    template <typename T>
    struct StringMaker<T^> {
        static std::string convert( T^ ref ) {
            return ::Catch::Detail::clrReferenceToString(ref);
        }
    };
#endif

    namespace Detail {
        template<typename InputIterator>
        std::string rangeToString(InputIterator first, InputIterator last) {
            ReusableStringStream rss;
            rss << "{ ";
            if (first != last) {
                rss << ::Catch::Detail::stringify(*first);
                for (++first; first != last; ++first)
                    rss << ", " << ::Catch::Detail::stringify(*first);
            }
            rss << " }";
            return rss.str();
        }
    }

#ifdef __OBJC__
    template<>
    struct StringMaker<NSString*> {
        static std::string convert(NSString * nsstring) {
            if (!nsstring)
                return "nil";
            return std::string("@") + [nsstring UTF8String];
        }
    };
    template<>
    struct StringMaker<NSObject*> {
        static std::string convert(NSObject* nsObject) {
            return ::Catch::Detail::stringify([nsObject description]);
        }

    };
    namespace Detail {
        inline std::string stringify( NSString* nsstring ) {
            return StringMaker<NSString*>::convert( nsstring );
        }

    } // namespace Detail
#endif // __OBJC__

} // namespace Catch

//////////////////////////////////////////////////////
// Separate std-lib types stringification, so it can be selectively enabled
// This means that we do not bring in

#if defined(CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS)
#  define CATCH_CONFIG_ENABLE_PAIR_STRINGMAKER
#  define CATCH_CONFIG_ENABLE_TUPLE_STRINGMAKER
#  define CATCH_CONFIG_ENABLE_VARIANT_STRINGMAKER
#  define CATCH_CONFIG_ENABLE_CHRONO_STRINGMAKER
#  define CATCH_CONFIG_ENABLE_OPTIONAL_STRINGMAKER
#endif

// Separate std::pair specialization
#if defined(CATCH_CONFIG_ENABLE_PAIR_STRINGMAKER)
#include <utility>
namespace Catch {
    template<typename T1, typename T2>
    struct StringMaker<std::pair<T1, T2> > {
        static std::string convert(const std::pair<T1, T2>& pair) {
            ReusableStringStream rss;
            rss << "{ "
                << ::Catch::Detail::stringify(pair.first)
                << ", "
                << ::Catch::Detail::stringify(pair.second)
                << " }";
            return rss.str();
        }
    };
}
#endif // CATCH_CONFIG_ENABLE_PAIR_STRINGMAKER

#if defined(CATCH_CONFIG_ENABLE_OPTIONAL_STRINGMAKER) && defined(CATCH_CONFIG_CPP17_OPTIONAL)
#include <optional>
namespace Catch {
    template<typename T>
    struct StringMaker<std::optional<T> > {
        static std::string convert(const std::optional<T>& optional) {
            ReusableStringStream rss;
            if (optional.has_value()) {
                rss << ::Catch::Detail::stringify(*optional);
            } else {
                rss << "{ }";
            }
            return rss.str();
        }
    };
}
#endif // CATCH_CONFIG_ENABLE_OPTIONAL_STRINGMAKER

// Separate std::tuple specialization
#if defined(CATCH_CONFIG_ENABLE_TUPLE_STRINGMAKER)
#include <tuple>
namespace Catch {
    namespace Detail {
        template<
            typename Tuple,
            std::size_t N = 0,
            bool = (N < std::tuple_size<Tuple>::value)
            >
            struct TupleElementPrinter {
            static void print(const Tuple& tuple, std::ostream& os) {
                os << (N ? ", " : " ")
                    << ::Catch::Detail::stringify(std::get<N>(tuple));
                TupleElementPrinter<Tuple, N + 1>::print(tuple, os);
            }
        };

        template<
            typename Tuple,
            std::size_t N
        >
            struct TupleElementPrinter<Tuple, N, false> {
            static void print(const Tuple&, std::ostream&) {}
        };

    }


    template<typename ...Types>
    struct StringMaker<std::tuple<Types...>> {
        static std::string convert(const std::tuple<Types...>& tuple) {
            ReusableStringStream rss;
            rss << '{';
            Detail::TupleElementPrinter<std::tuple<Types...>>::print(tuple, rss.get());
            rss << " }";
            return rss.str();
        }
    };
}
#endif // CATCH_CONFIG_ENABLE_TUPLE_STRINGMAKER

#if defined(CATCH_CONFIG_ENABLE_VARIANT_STRINGMAKER) && defined(CATCH_CONFIG_CPP17_VARIANT)
#include <variant>
namespace Catch {
    template<>
    struct StringMaker<std::monostate> {
        static std::string convert(const std::monostate&) {
            return "{ }";
        }
    };

    template<typename... Elements>
    struct StringMaker<std::variant<Elements...>> {
        static std::string convert(const std::variant<Elements...>& variant) {
            if (variant.valueless_by_exception()) {
                return "{valueless variant}";
            } else {
                return std::visit(
                    [](const auto& value) {
                        return ::Catch::Detail::stringify(value);
                    },
                    variant
                );
            }
        }
    };
}
#endif // CATCH_CONFIG_ENABLE_VARIANT_STRINGMAKER

namespace Catch {
    struct not_this_one {}; // Tag type for detecting which begin/ end are being selected

    // Import begin/ end from std here so they are considered alongside the fallback (...) overloads in this namespace
    using std::begin;
    using std::end;

    not_this_one begin( ... );
    not_this_one end( ... );

    template <typename T>
    struct is_range {
        static const bool value =
            !std::is_same<decltype(begin(std::declval<T>())), not_this_one>::value &&
            !std::is_same<decltype(end(std::declval<T>())), not_this_one>::value;
    };

#if defined(_MANAGED) // Managed types are never ranges
    template <typename T>
    struct is_range<T^> {
        static const bool value = false;
    };
#endif

    template<typename Range>
    std::string rangeToString( Range const& range ) {
        return ::Catch::Detail::rangeToString( begin( range ), end( range ) );
    }

    // Handle vector<bool> specially
    template<typename Allocator>
    std::string rangeToString( std::vector<bool, Allocator> const& v ) {
        ReusableStringStream rss;
        rss << "{ ";
        bool first = true;
        for( bool b : v ) {
            if( first )
                first = false;
            else
                rss << ", ";
            rss << ::Catch::Detail::stringify( b );
        }
        rss << " }";
        return rss.str();
    }

    template<typename R>
    struct StringMaker<R, typename std::enable_if<is_range<R>::value && !::Catch::Detail::IsStreamInsertable<R>::value>::type> {
        static std::string convert( R const& range ) {
            return rangeToString( range );
        }
    };

    template <typename T, int SZ>
    struct StringMaker<T[SZ]> {
        static std::string convert(T const(&arr)[SZ]) {
            return rangeToString(arr);
        }
    };


} // namespace Catch

// Separate std::chrono::duration specialization
#if defined(CATCH_CONFIG_ENABLE_CHRONO_STRINGMAKER)
#include <ctime>
#include <ratio>
#include <chrono>


namespace Catch {

template <class Ratio>
struct ratio_string {
    static std::string symbol();
};

template <class Ratio>
std::string ratio_string<Ratio>::symbol() {
    Catch::ReusableStringStream rss;
    rss << '[' << Ratio::num << '/'
        << Ratio::den << ']';
    return rss.str();
}
template <>
struct ratio_string<std::atto> {
    static std::string symbol();
};
template <>
struct ratio_string<std::femto> {
    static std::string symbol();
};
template <>
struct ratio_string<std::pico> {
    static std::string symbol();
};
template <>
struct ratio_string<std::nano> {
    static std::string symbol();
};
template <>
struct ratio_string<std::micro> {
    static std::string symbol();
};
template <>
struct ratio_string<std::milli> {
    static std::string symbol();
};

    ////////////
    // std::chrono::duration specializations
    template<typename Value, typename Ratio>
    struct StringMaker<std::chrono::duration<Value, Ratio>> {
        static std::string convert(std::chrono::duration<Value, Ratio> const& duration) {
            ReusableStringStream rss;
            rss << duration.count() << ' ' << ratio_string<Ratio>::symbol() << 's';
            return rss.str();
        }
    };
    template<typename Value>
    struct StringMaker<std::chrono::duration<Value, std::ratio<1>>> {
        static std::string convert(std::chrono::duration<Value, std::ratio<1>> const& duration) {
            ReusableStringStream rss;
            rss << duration.count() << " s";
            return rss.str();
        }
    };
    template<typename Value>
    struct StringMaker<std::chrono::duration<Value, std::ratio<60>>> {
        static std::string convert(std::chrono::duration<Value, std::ratio<60>> const& duration) {
            ReusableStringStream rss;
            rss << duration.count() << " m";
            return rss.str();
        }
    };
    template<typename Value>
    struct StringMaker<std::chrono::duration<Value, std::ratio<3600>>> {
        static std::string convert(std::chrono::duration<Value, std::ratio<3600>> const& duration) {
            ReusableStringStream rss;
            rss << duration.count() << " h";
            return rss.str();
        }
    };

    ////////////
    // std::chrono::time_point specialization
    // Generic time_point cannot be specialized, only std::chrono::time_point<system_clock>
    template<typename Clock, typename Duration>
    struct StringMaker<std::chrono::time_point<Clock, Duration>> {
        static std::string convert(std::chrono::time_point<Clock, Duration> const& time_point) {
            return ::Catch::Detail::stringify(time_point.time_since_epoch()) + " since epoch";
        }
    };
    // std::chrono::time_point<system_clock> specialization
    template<typename Duration>
    struct StringMaker<std::chrono::time_point<std::chrono::system_clock, Duration>> {
        static std::string convert(std::chrono::time_point<std::chrono::system_clock, Duration> const& time_point) {
            auto converted = std::chrono::system_clock::to_time_t(time_point);

#ifdef _MSC_VER
            std::tm timeInfo = {};
            gmtime_s(&timeInfo, &converted);
#else
            std::tm* timeInfo = std::gmtime(&converted);
#endif

            auto const timeStampSize = sizeof("2017-01-16T17:06:45Z");
            char timeStamp[timeStampSize];
            const char * const fmt = "%Y-%m-%dT%H:%M:%SZ";

#ifdef _MSC_VER
            std::strftime(timeStamp, timeStampSize, fmt, &timeInfo);
#else
            std::strftime(timeStamp, timeStampSize, fmt, timeInfo);
#endif
            return std::string(timeStamp);
        }
    };
}
#endif // CATCH_CONFIG_ENABLE_CHRONO_STRINGMAKER

#define INTERNAL_CATCH_REGISTER_ENUM( enumName, ... ) \
namespace Catch { \
    template<> struct StringMaker<enumName> { \
        static std::string convert( enumName value ) { \
            static const auto& enumInfo = ::Catch::getMutableRegistryHub().getMutableEnumValuesRegistry().registerEnum( #enumName, #__VA_ARGS__, { __VA_ARGS__ } ); \
            return enumInfo.lookup( static_cast<int>( value ) ); \
        } \
    }; \
}

#define CATCH_REGISTER_ENUM( enumName, ... ) INTERNAL_CATCH_REGISTER_ENUM( enumName, __VA_ARGS__ )

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // TWOBLUECUBES_CATCH_TOSTRING_H_INCLUDED

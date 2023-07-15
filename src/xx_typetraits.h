#pragma once
#include "xx_includes.h"

namespace xx {

    /************************************************************************************/
    // std::is_pod 的自定义扩展, 用于标识一个类可以在容器中被 memcpy | memmove

    template<typename T, typename ENABLED = void>
    struct IsPod : std::false_type {};
    template<typename T>
    struct IsPod<T, std::enable_if_t<std::is_standard_layout_v<T>&& std::is_trivial_v<T>>> : std::true_type {};
    template<typename T> constexpr bool IsPod_v = IsPod<T>::value;


    /************************************************************************************/
    // 检测 T 是否为 代码内的明文 "xxxxx" 字串

    template<typename T, typename = void>
    struct IsLiteral : std::false_type {};
    template<size_t L>
    struct IsLiteral<char[L], void> : std::true_type {
        static const size_t len = L;
    };
    template<size_t L>
    struct IsLiteral<char const [L], void> : std::true_type {
        static const size_t len = L;
    };
    template<size_t L>
    struct IsLiteral<char const (&)[L], void> : std::true_type {
        static const size_t len = L;
    };
    template<typename T>
    constexpr bool IsLiteral_v = IsLiteral<T>::value;
    template<typename T>
    constexpr size_t LiteralLen = IsLiteral<T>::len;

    /************************************************************************************/
    // 检测 T 是否为 std::optional

    template<typename T>
    struct IsOptional : std::false_type {};
    template<typename T>
    struct IsOptional<std::optional<T>> : std::true_type {};
    template<typename T>
    struct IsOptional<std::optional<T>&> : std::true_type {};
    template<typename T>
    struct IsOptional<std::optional<T> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsOptional_v = IsOptional<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::vector

    template<typename T>
    struct IsVector : std::false_type {};
    template<typename T>
    struct IsVector<std::vector<T>> : std::true_type {};
    template<typename T>
    struct IsVector<std::vector<T>&> : std::true_type {};
    template<typename T>
    struct IsVector<std::vector<T> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsVector_v = IsVector<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::tuple

    template<typename>
    struct IsTuple : std::false_type {};
    template<typename ...T>
    struct IsTuple<std::tuple<T...>> : std::true_type {};
    template<typename ...T>
    struct IsTuple<std::tuple<T...>&> : std::true_type {};
    template<typename ...T>
    struct IsTuple<std::tuple<T...> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsTuple_v = IsTuple<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::variant

    template<typename>
    struct IsVariant : std::false_type {};
    template<typename ...T>
    struct IsVariant<std::variant<T...>> : std::true_type {};
    template<typename ...T>
    struct IsVariant<std::variant<T...>&> : std::true_type {};
    template<typename ...T>
    struct IsVariant<std::variant<T...> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsVariant_v = IsVariant<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::unique_ptr

    template<typename T>
    struct IsUnique : std::false_type {};
    template<typename T>
    struct IsUnique<std::unique_ptr<T>> : std::true_type {};
    template<typename T>
    struct IsUnique<std::unique_ptr<T>&> : std::true_type {};
    template<typename T>
    struct IsUnique<std::unique_ptr<T> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsUnique_v = IsUnique<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::unordered_set

    template<typename T>
    struct IsUnorderedSet : std::false_type {};
    template<typename T>
    struct IsUnorderedSet<std::unordered_set<T>> : std::true_type {};
    template<typename T>
    struct IsUnorderedSet<std::unordered_set<T>&> : std::true_type {};
    template<typename T>
    struct IsUnorderedSet<std::unordered_set<T> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsUnorderedSet_v = IsUnorderedSet<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::set

    template<typename T>
    struct IsSet : std::false_type {};
    template<typename T>
    struct IsSet<std::set<T>> : std::true_type {};
    template<typename T>
    struct IsSet<std::set<T>&> : std::true_type {};
    template<typename T>
    struct IsSet<std::set<T> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsSet_v = IsSet<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::set 或 std::unordered_set

    template<typename T>
    constexpr bool IsSetSeries_v = IsSet_v<T> || IsUnorderedSet_v<T>;

    /************************************************************************************/
    // 检测 T 是否为 std::queue

    template<typename T>
    struct IsQueue : std::false_type {};
    template<typename T>
    struct IsQueue<std::queue<T>> : std::true_type {};
    template<typename T>
    struct IsQueue<std::queue<T>&> : std::true_type {};
    template<typename T>
    struct IsQueue<std::queue<T> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsQueue_v = IsQueue<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::deque

    template<typename T>
    struct IsDeque : std::false_type {};
    template<typename T>
    struct IsDeque<std::deque<T>> : std::true_type {};
    template<typename T>
    struct IsDeque<std::deque<T>&> : std::true_type {};
    template<typename T>
    struct IsDeque<std::deque<T> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsDeque_v = IsDeque<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::queue 或 std::deque

    template<typename T>
    constexpr bool IsQueueSeries_v = IsQueue_v<T> || IsDeque_v<T>;

    /************************************************************************************/
    // 检测 T 是否为 std::unordered_map

    template<typename T>
    struct IsUnorderedMap : std::false_type {
        typedef void PT;
    };
    template<typename K, typename V>
    struct IsUnorderedMap<std::unordered_map<K, V>> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsUnorderedMap<std::unordered_map<K, V>&> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsUnorderedMap<std::unordered_map<K, V> const&> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename T>
    constexpr bool IsUnorderedMap_v = IsUnorderedMap<T>::value;
    template<typename T>
    using UnorderedMap_Pair_t = typename IsUnorderedMap<T>::PT;

    /************************************************************************************/
    // 检测 T 是否为 std::map

    template<typename T>
    struct IsMap : std::false_type {
        typedef void PT;
    };
    template<typename K, typename V>
    struct IsMap<std::map<K, V>> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsMap<std::map<K, V>&> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename K, typename V>
    struct IsMap<std::map<K, V> const&> : std::true_type {
        typedef std::pair<K, V> PT;
    };
    template<typename T>
    constexpr bool IsMap_v = IsMap<T>::value;
    template<typename T>
    using Map_Pair_t = typename IsMap<T>::PT;

    /************************************************************************************/
    // 检测 T 是否为 std::map 或 std::unordered_map

    template<typename T>
    constexpr bool IsMapSeries_v = IsUnorderedMap_v<T> || IsMap_v<T>;
    template<typename T>
    using MapSeries_Pair_t = std::conditional_t<IsUnorderedMap_v<T>, UnorderedMap_Pair_t<T>, Map_Pair_t<T>>;

    /************************************************************************************/
    // 检测 T 是否为 std::pair

    template<typename T>
    struct IsPair : std::false_type {};
    template<typename F, typename S>
    struct IsPair<std::pair<F, S>> : std::true_type {};
    template<typename F, typename S>
    struct IsPair<std::pair<F, S>&> : std::true_type {};
    template<typename F, typename S>
    struct IsPair<std::pair<F, S> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsPair_v = IsPair<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 std::array

    template<typename T>
    struct IsArray : std::false_type {};
    template<typename T, size_t S>
    struct IsArray<std::array<T, S>> : std::true_type {};
    template<typename T, size_t S>
    struct IsArray<std::array<T, S>&> : std::true_type {};
    template<typename T, size_t S>
    struct IsArray<std::array<T, S> const&> : std::true_type {};
    template<typename T>
    constexpr bool IsArray_v = IsArray<T>::value;

    /************************************************************************************/
    // 检测 T 是否为 带 data() size() 这两个函数的容器

    template<typename, typename = void>
    struct IsContainer : std::false_type {};
    template<typename T>
    struct IsContainer<T, std::void_t<decltype(std::declval<T>().data()), decltype(std::declval<T>().size())>>
        : std::true_type {};
    template<typename T>
    constexpr bool IsContainer_v = IsContainer<T>::value;


    /************************************************************************************/
    // 检测 T 是否为 带 operator() 的可执行仿函数类( 含 lambda )

    template<typename T, typename = void>
    struct IsCallable : std::is_function<T> {};
    template<typename T>
    struct IsCallable<T, typename std::enable_if<std::is_same<decltype(void(&T::operator())), void>::value>::type> : std::true_type {};
    template<typename T>
    constexpr bool IsCallable_v = IsCallable<T>::value;


    /************************************************************************************/
	// 检测 T 是否为 std::function

	template<typename T>
	struct IsFunction : std::false_type {
	};
	template<typename T>
	struct IsFunction<std::function<T>> : std::true_type {
		using FT = T;
	};
	template<typename T>
	struct IsFunction<std::function<T>&> : std::true_type {
		using FT = T;
	};
	template<typename T>
	struct IsFunction<std::function<T> const&> : std::true_type {
		using FT = T;
	};
	template<typename T>
	constexpr bool IsFunction_v = IsFunction<T>::value;


    /************************************************************************************/
    // 检测 T 是否为 逻辑上的 简单 / 基础 类型( 数值, enum, string[_view], Data, 以及包裹容器的这些类型 )

    template<typename T, typename ENABLED = void>
    struct IsBaseDataType : std::false_type {};
    template<typename T> constexpr bool IsBaseDataType_v = IsBaseDataType<T>::value;

    struct Span;
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<std::is_arithmetic_v<T>
        || std::is_enum_v<T>
        || IsLiteral_v<T>
        || std::is_same_v<std::string, std::decay_t<T>>
        || std::is_same_v<std::string_view, std::decay_t<T>>
        || std::is_base_of_v<Span, T>
        >> : std::true_type{
    };

    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsOptional_v<T>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsVector_v<T>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsSetSeries_v<T>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsPair_v<T>&& IsBaseDataType_v<typename T::first_type>&& IsBaseDataType_v<typename T::second_type> >> : std::true_type {};
    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsMapSeries_v<T>&& IsBaseDataType_v<typename T::key_type>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};

    // tuple 得遍历每个类型来判断
    template<typename Tuple, typename = std::enable_if_t<IsTuple_v<Tuple>>>
    struct TupleIsBaseDataType {
        template<std::size_t index>
        static constexpr bool Check__() {
            if constexpr (index == std::tuple_size_v<Tuple>) {
                return true;
            } else {
                return IsBaseDataType_v<std::tuple_element_t<index, Tuple>> ? Check__<index + 1>() : false;
            }
        }

        static constexpr bool Check() {
            return Check__<0>();
        }
    };

    template<typename T>
    struct IsBaseDataType<T, std::enable_if_t<IsTuple_v<T>&& TupleIsBaseDataType<T>::Check() >> : std::true_type {};


    /************************************************************************************/
    // 判断 tuple 里是否存在某种数据类型

    template<typename T, typename Tuple>
    struct HasType;
    template<typename T>
    struct HasType<T, std::tuple<>> : std::false_type {};
    template<typename T, typename U, typename... Ts>
    struct HasType<T, std::tuple<U, Ts...>> : HasType<T, std::tuple<Ts...>> {};
    template<typename T, typename... Ts>
    struct HasType<T, std::tuple<T, Ts...>> : std::true_type {};
    template<typename T, typename Tuple>
    using TupleContainsType = typename HasType<T, Tuple>::type;
    template<typename T, typename Tuple>
    constexpr bool TupleContainsType_v = TupleContainsType<T, Tuple>::value;


    /************************************************************************************/
    // 计算某类型在 tuple 里是第几个

    template<class T, class Tuple>
    struct TupleTypeIndex;
    template<class T, class...TS>
    struct TupleTypeIndex<T, std::tuple<T, TS...>> {
        static const size_t value = 0;
    };
    template<class T, class U, class... TS>
    struct TupleTypeIndex<T, std::tuple<U, TS...>> {
        static const size_t value = 1 + TupleTypeIndex<T, std::tuple<TS...>>::value;
    };
    template<typename T, typename Tuple>
    constexpr size_t TupleTypeIndex_v = TupleTypeIndex<T, Tuple>::value;


    /************************************************************************************/
    // 获取指定下标参数

    template <int I, typename...Args>
    decltype(auto) GetAt(Args&&...args) {
        return std::get<I>(std::forward_as_tuple(args...));
    }

    /************************************************************************************/
    // static comparer func type define

    template<typename A, typename B>
    using ABComparer = bool (*) (A const& a, B const& b);


    /************************************************************************************/
    // 拆解仿函数成员 operator() 得到它的 各种信息（返回值，所属类，参数，是否可变）

    template<typename T, class = void>
    struct FuncTraits;

    template<typename Rtv, typename...Args>
    struct FuncTraits<Rtv(*)(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = void;
        static const bool isMutable = true;
    };

    template<typename Rtv, typename...Args>
    struct FuncTraits<Rtv(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = void;
        static const bool isMutable = true;
    };

    template<typename Rtv, typename CT, typename... Args>
    struct FuncTraits<Rtv(CT::*)(Args ...)> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = CT;
        static const bool isMutable = true;
    };

    template<typename Rtv, typename CT, typename... Args>
    struct FuncTraits<Rtv(CT::*)(Args ...) const> {
        using R = Rtv;
        using A = std::tuple<Args...>;
        using A2 = std::tuple<std::decay_t<Args>...>;
        using C = CT;
        static const bool isMutable = false;
    };

    template<typename T>
    struct FuncTraits<T, std::void_t<decltype(&T::operator())> >
        : public FuncTraits<decltype(&T::operator())> {};

    template<typename T>
    using FuncR_t = typename FuncTraits<T>::R;
    template<typename T>
    using FuncA_t = typename FuncTraits<T>::A;
    template<typename T>
    using FuncA2_t = typename FuncTraits<T>::A2;
    template<typename T>
    using FuncC_t = typename FuncTraits<T>::C;
    template<typename T>
    constexpr bool isMutable_v = FuncTraits<T>::isMutable;


    /************************************************************************************/
    // 针对模板基类，检查某类型是否其派生类. 用法: XX_IsTemplateOf(BT, T)::value

    struct IsTemplateOf {
        template <template <class> class TM, class T> static std::true_type  check(TM<T>);
        template <template <class> class TM>          static std::false_type check(...);
        template <template <int>   class TM, int N>   static std::true_type  check(TM<N>);
        template <template <int>   class TM>          static std::false_type check(...);
    };
#define XX_IsTemplateOf(TM, ...) decltype(::xx::IsTemplateOf::check<TM>(std::declval<__VA_ARGS__>()))



    /************************************************************************************/
    // 辅助调用构造函数的容器

    template<typename U>
    struct TCtor {
        U* p;
        template<typename...Args>
        U& operator()(Args&&...args) {
            return *new (p) U(std::forward<Args>(args)...);
        }
    };


    /************************************************************************************/
    // literal string 的模板值容器

    template<size_t n>
    struct Str {
        char v[n];
        constexpr Str(const char(&s)[n]) {
            std::copy_n(s, n, v);
        }
        constexpr std::string_view ToSV() const {
            return { v, n - 1 };
        }
    };


    /************************************************************************************/
    // 判断目标类型是否为 []{} 这种 lambda( 依赖编译器具体实现 )

    namespace Detail {
        template<Str s, typename T>
        inline static constexpr bool LambdaChecker() {
            if constexpr (sizeof(s.v) > 8) {
                for (std::size_t i = 0; i < sizeof(s.v) - 8; i++) {
                    if ((s.v[i + 0] == '(' || s.v[i + 0] == '<')
                        && s.v[i + 1] == 'l'
                        && s.v[i + 2] == 'a'
                        && s.v[i + 3] == 'm'
                        && s.v[i + 4] == 'b'
                        && s.v[i + 5] == 'd'
                        && s.v[i + 6] == 'a'
                        && (s.v[i + 7] == ' ' || s.v[i + 7] == '(' || s.v[i + 7] == '_')
                        ) return true;
                }
            }
            return false;
        }
    }

    template<typename...T>
    constexpr bool IsLambda() {
        return ::xx::Detail::LambdaChecker<
#ifdef _MSC_VER
            __FUNCSIG__
#else
            __PRETTY_FUNCTION__
#endif
            , T...>();
    }
    template<typename ...T>
    constexpr bool IsLambda_v = IsLambda<T...>();



    /***********************************************************************************/
    // 找出 多个模板类型 的 sizeof 最大值

    template<typename T, typename... Args>
    struct MaxSizeof {
        static const size_t value = sizeof(T) > MaxSizeof<Args...>::value
            ? sizeof(T)
            : MaxSizeof<Args...>::value;
    };
    template<typename T>
    struct MaxSizeof<T> {
        static const size_t value = sizeof(T);
    };

    template<typename T, typename... Args>
    constexpr size_t MaxSizeof_v = MaxSizeof<T, Args...>::value;


    /************************************************************************************/
    // TypeName. 当前只支持运行时拿 std::string. android 下带混淆效果

#ifndef RAW_TYPE_NAME_ANDROID_XOR_KEY
#define RAW_TYPE_NAME_ANDROID_XOR_KEY 0b10101010
#endif

// for store __FUNCSIG__ or __PRETTY_FUNCTION__ raw content
    template<size_t n>
    struct RawTypeNameStr {
        char v[n];
        constexpr RawTypeNameStr(const char(&s)[n]) {
            std::copy_n(s, n, v);
#ifdef __ANDROID__
            for (auto& c : v) {
                c ^= RAW_TYPE_NAME_ANDROID_XOR_KEY;
            }
#endif
        }
    };

    namespace Detail {

        // store type name cut pos
        inline static std::pair<size_t, size_t> typeNameOffsetLen{};

        template<typename T>
        struct TypeName_ {

            // fill cut pos
            template<RawTypeNameStr s>
            inline static std::string Fil() {
                for (std::size_t i = 0;; i++) {
#ifdef __ANDROID__
                    if (s.v[i] == char('i' ^ RAW_TYPE_NAME_ANDROID_XOR_KEY)
                        && s.v[i + 1] == char('n' ^ RAW_TYPE_NAME_ANDROID_XOR_KEY)
                        && s.v[i + 2] == char('t' ^ RAW_TYPE_NAME_ANDROID_XOR_KEY)) {
#else
                    if (s.v[i] == 'i' && s.v[i + 1] == 'n' && s.v[i + 2] == 't') {
#endif
                        typeNameOffsetLen.first = i;
                        typeNameOffsetLen.second = sizeof(s.v) - i - 3;
                        break;
                    }
                    }
                return "";
                }

            // get type name string
            template<RawTypeNameStr s>
            inline static std::string Get() {
                auto len = sizeof(s.v) - typeNameOffsetLen.first - typeNameOffsetLen.second;
                std::string r;
                r.resize(len);
#ifdef __ANDROID__
                for (size_t j = 0; j < len; j++) {
                    r[j] = (s.v + typeNameOffsetLen.first)[j] ^ RAW_TYPE_NAME_ANDROID_XOR_KEY;
                }
#else
                memcpy(r.data(), s.v + typeNameOffsetLen.first, len);
#endif
                return r;
            }
            };
        }

    // for calc type name cut pos
    template<typename T>
    std::string FillTNOL() {
        return ::xx::Detail::TypeName_<T>::template Fil<
#ifdef _MSC_VER
            __FUNCSIG__
#else
            __PRETTY_FUNCTION__
#endif
        >();
    }

    // auto execute fill type name cut pos
    inline static bool typeNameOffsetLenFilled = [] {
        FillTNOL<int>();
        return true;
    }();

    // get type name
    template<typename T>
    std::string TypeName() {
        return ::xx::Detail::TypeName_<T>::template Get<
#ifdef _MSC_VER
            __FUNCSIG__
#else
            __PRETTY_FUNCTION__
#endif
        >();
    }

}

// 用于得到检测 T:: typename 是否存在的 constexpr. 已否决，应改用 concept

#define XX_HAS_TYPEDEF( TN ) \
template<typename, typename = void> struct HasTypedef_##TN : std::false_type {}; \
template<typename T> struct HasTypedef_##TN<T, std::void_t<typename T::TN>> : std::true_type {}; \
template<typename T> constexpr bool TN = HasTypedef_##TN<T>::value;

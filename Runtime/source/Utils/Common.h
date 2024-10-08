namespace RHINO {
    template<typename T, size_t size>
    constexpr size_t RHINO_ARR_SIZE(T (&)[size]) {
        return size;
    }

    template<typename OutT, typename InT>
    OutT INTERPRET_AS(InT obj) noexcept {
        assert(dynamic_cast<OutT>(obj) != nullptr);
        return static_cast<OutT>(obj);
    }

#ifdef __clang__
    template<typename T>
    void UnusedVarHelper(const T& var __attribute__((unused))){};
#endif// __clang__
}// namespace RHINO

#define RHINO_DEFINE_BITMASK_ENUM(enumType)                                            \
    enumType operator&(enumType a, enumType b) {                                       \
        return static_cast<enumType>(static_cast<size_t>(a) & static_cast<size_t>(b)); \
    }                                                                                  \
    enumType operator|(enumType a, enumType b) {                                       \
        return static_cast<enumType>(static_cast<size_t>(a) | static_cast<size_t>(b)); \
    }                                                                                  \
    bool operator||(bool a, enumType b) {                                              \
        return a || static_cast<size_t>(b) != 0;                                       \
    }                                                                                  \
    bool operator&&(bool a, enumType b) {                                              \
        return a && static_cast<size_t>(b) != 0;                                       \
    }                                                                                  \
    bool operator!(enumType a) {                                                       \
        return static_cast<size_t>(a) == 0;                                            \
    }

#define RHINO_CEIL_TO_MULTIPLE_OF(intValue, multiple) (((intValue + multiple - 1) / multiple) * multiple)
#define RHINO_CEIL_TO_POWER_OF_TWO(intValue, powerOfTwo) ((intValue + (powerOfTwo - 1)) & (~(powerOfTwo - 1)))

#ifdef __clang__
#define RHINO_UNUSED_VAR(var) (RHINO::UnusedVarHelper(var))
#else
#define RHINO_UNUSED_VAR(var) (var)
#endif

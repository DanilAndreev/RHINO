namespace RHINO {
    template<typename T, size_t size>
    constexpr size_t RHINO_ARR_SIZE(T (&)[size]) {
        return size;
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

#ifdef __clang__
#define RHINO_UNUSED_VAR(var) (RHINO::UnusedVarHelper(var))
#else
#define RHINO_UNUSED_VAR(var) (var)
#endif

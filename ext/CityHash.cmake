file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/builtin-expect.c
    "int main() { if (__builtin_expect(true)) return 0; }")

try_compile(BUILTIN_EXPECT ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/builtin-expect.c)

if (BUILTIN_EXPECT)
    set(BUILTIN_EXPECT "#define HAVE_BUILTIN_EXPECT")
endif()
find_file(DLFCN dlfcn.h)
if (DLFCN)
    set(DLFCN_H "#define HAVE_DLFCN_H")
endif()
find_file(INTTYPES inttypes.h)
if (INTTYPES)
    set(INTTYPES_H "#define HAVE_INTTYPES_H")
endif()
find_file(MEMORY memory.h)
if (MEMORY)
    set(MEMORY_H "#define HAVE_MEMORY_H")
endif()
find_file(STDINT stdint.h)
if (STDINT)
    set(STDINT_H "#define HAVE_STDINT_H")
endif()
find_file(STDLIB stdlib.h)
if (STDLIB)
    set(STDLIB_H "#define HAVE_STDLIB_H")
endif()
find_file(STRINGS_HDR strings.h)
if (STRINGS_HDR)
    set(STRINGS_H "#define HAVE_STRINGS_H")
endif()
find_file(STRING_HDR string.h)
if (STRING_HDR)
    set(STRING_H "#define HAVE_STRING_H")
endif()
find_file(SYSSTAT sys/stat.h)
if (SYSSTAT)
    set(SYSSTAT_H "#define HAVE_SYS_STAT_H")
endif()
find_file(SYSTYPES sys/types.h)
if (SYSTYPES)
    set(SYSTYPES_H "#define HAVE_SYS_TYPES_H")
endif()
find_file(UNISTD unistd.h)
if (UNISTD)
    set(UNISTD_H "#define HAVE_UNISTD_H")
endif()
if (c_std_90 IN_LIST CMAKE_C_COMPILE_FEATURES)
    set(STDC_H "#define STDC_HEADERS")
endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/config.h
    "${BUILTIN_EXPECT_H}
     ${DLFCN_H}
     ${INTTYPES_H}
     ${MEMORY_H}
     ${STDINT_H}
     ${STDLIB_H}
     ${STRINGS_H}
     ${STRING_H}
     ${SYSSTAT_H}
     ${SYSTYPES_H}
     ${UNISTD_H}
     ${STDC_H}")
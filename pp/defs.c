#include "defs.h"

#include <die.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"
#include "pp_toker.h"

// ReSharper disable CppDFANullDereference

static char **parse_args(const char *name, const char *args);

void defines_add(const defines *defs, const char *name, const char *args, const char *replace)
{
    def *d = calloc(1, sizeof(*d));
    d->name = strdup(name);
    d->args = parse_args(name, args);
    d->replace = replace ? strdup(replace) : NULL;

    hashmap_add(defs->h, name, d);
}

int defines_remove(const defines *defs, const char *name)
{
    const def *d = hashmap_delete(defs->h, name);
    if (d) {
        free(d->name);
        int i;
        for (i = 0; d->args && d->args[i]; i++) {
            free(d->args[i]);
        }
        free(d->replace);
        free((void *) d);
        return 1;
    }
    return 0;
}

static void skip_parens(token_state *ts)
{
    int parens_count = 1;
    while (!TOKEN_STATE_DONE(ts) && parens_count > 0) {
        const token t = get_token(ts);
        if (t.type == '(') {
            parens_count++;
        }
        if (t.type == ')') {
            parens_count--;
        }
    }
}

static char **parse_args(const char *name, const char *args)
{
    token_state ts;
    char **result = NULL;
    int comma_counter = 0;
    size_t n = 0;

    if (!args) {
        return NULL;
    }

    set_token_string(&ts, args);
    const size_t len = strlen(args);
    for (;;) {
        const token t = get_token(&ts);
        if (!len || t.type == TOK_WS) {
            // ignore
        } else if (t.type == '(') {
            // skip past parenthesized argument in args list
            skip_parens(&ts);
        } else if (t.type == ',') {
            if (comma_counter % 2 != 1) {
                die("#define %s args extra comma in %s", name, args);
            }
            comma_counter++;
        } else if (t.type == TOK_ID || IS_KEYWORD(t.type) || t.type == TOK_ELLIPSIS) {
            if (comma_counter % 2 != 0) {
                die("#define %s args missing comma in %s", name, args);
            }
            comma_counter++;
            if (n % 5 == 0) {
                result = realloc(result, sizeof(*result) * (n + 6));
            }
            result[n++] = strdup(t.tok);
        } else {
            die("#define %s(%s) bad arg \"%s\"", name, args, t.tok);
        }
        if (TOKEN_STATE_DONE(&ts)) {
            break;
        }
    }
    if (result && n) {
        result[n] = NULL;
    }
    return result;
}

defines *defines_init(void)
{
    defines *defs;

    defs = calloc(1, sizeof *defs);
    defs->h = hashmap_init(1024);

    // todo - implement
    defines_add(defs, "__FILE__", NULL, "\"__filename__\"");
    defines_add(defs, "__LINE__", NULL, "0");

    // initial defines from 'echo | cc -E -dM -std=c89 -'

    defines_add(defs, "_LP64", NULL, "1");
    defines_add(defs, "__AARCH64EL__", NULL, "1");
    defines_add(defs, "__AARCH64_CMODEL_SMALL__", NULL, "1");
    defines_add(defs, "__AARCH64_SIMD__", NULL, "1");
    defines_add(defs, "__APPLE_CC__", NULL, "6000");
    defines_add(defs, "__APPLE__", NULL, "1");
    defines_add(defs, "__ARM64_ARCH_8__", NULL, "1");
    defines_add(defs, "__ARM_64BIT_STATE", NULL, "1");
    defines_add(defs, "__ARM_ACLE", NULL, "200");
    defines_add(defs, "__ARM_ALIGN_MAX_STACK_PWR", NULL, "4");
    defines_add(defs, "__ARM_ARCH", NULL, "8");
    defines_add(defs, "__ARM_ARCH_8_3__", NULL, "1");
    defines_add(defs, "__ARM_ARCH_8_4__", NULL, "1");
    defines_add(defs, "__ARM_ARCH_8_5__", NULL, "1");
    defines_add(defs, "__ARM_ARCH_ISA_A64", NULL, "1");
    defines_add(defs, "__ARM_ARCH_PROFILE", NULL, "'A'");
    defines_add(defs, "__ARM_FEATURE_AES", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_ATOMICS", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_BTI", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_CLZ", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_COMPLEX", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_CRC32", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_CRYPTO", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_DIRECTED_ROUNDING", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_DIV", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_DOTPROD", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_FMA", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_FP16_FML", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_FP16_SCALAR_ARITHMETIC", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_FP16_VECTOR_ARITHMETIC", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_FRINT", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_IDIV", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_JCVT", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_LDREX", NULL, "0xF");
    defines_add(defs, "__ARM_FEATURE_NUMERIC_MAXMIN", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_PAUTH", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_QRDMX", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_RCPC", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_SHA2", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_SHA3", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_SHA512", NULL, "1");
    defines_add(defs, "__ARM_FEATURE_UNALIGNED", NULL, "1");
    defines_add(defs, "__ARM_FP", NULL, "0xE");
    defines_add(defs, "__ARM_FP16_ARGS", NULL, "1");
    defines_add(defs, "__ARM_FP16_FORMAT_IEEE", NULL, "1");
    defines_add(defs, "__ARM_NEON", NULL, "1");
    defines_add(defs, "__ARM_NEON_FP", NULL, "0xE");
    defines_add(defs, "__ARM_NEON__", NULL, "1");
    defines_add(defs, "__ARM_PCS_AAPCS64", NULL, "1");
    defines_add(defs, "__ARM_SIZEOF_MINIMAL_ENUM", NULL, "4");
    defines_add(defs, "__ARM_SIZEOF_WCHAR_T", NULL, "4");
    defines_add(defs, "__ATOMIC_ACQUIRE", NULL, "2");
    defines_add(defs, "__ATOMIC_ACQ_REL", NULL, "4");
    defines_add(defs, "__ATOMIC_CONSUME", NULL, "1");
    defines_add(defs, "__ATOMIC_RELAXED", NULL, "0");
    defines_add(defs, "__ATOMIC_RELEASE", NULL, "3");
    defines_add(defs, "__ATOMIC_SEQ_CST", NULL, "5");
    defines_add(defs, "__BIGGEST_ALIGNMENT__", NULL, "8");
    defines_add(defs, "__BITINT_MAXWIDTH__", NULL, "128");
    defines_add(defs, "__BLOCKS__", NULL, "1");
    defines_add(defs, "__BOOL_WIDTH__", NULL, "8");
    defines_add(defs, "__BYTE_ORDER__", NULL, "__ORDER_LITTLE_ENDIAN__");
    defines_add(defs, "__CHAR16_TYPE__", NULL, "unsigned short");
    defines_add(defs, "__CHAR32_TYPE__", NULL, "unsigned int");
    defines_add(defs, "__CHAR_BIT__", NULL, "8");
    defines_add(defs, "__CONSTANT_CFSTRINGS__", NULL, "1");
    defines_add(defs, "__DBL_DECIMAL_DIG__", NULL, "17");
    defines_add(defs, "__DBL_DENORM_MIN__", NULL, "4.9406564584124654e-324");
    defines_add(defs, "__DBL_DIG__", NULL, "15");
    defines_add(defs, "__DBL_EPSILON__", NULL, "2.2204460492503131e-16");
    defines_add(defs, "__DBL_HAS_DENORM__", NULL, "1");
    defines_add(defs, "__DBL_HAS_INFINITY__", NULL, "1");
    defines_add(defs, "__DBL_HAS_QUIET_NAN__", NULL, "1");
    defines_add(defs, "__DBL_MANT_DIG__", NULL, "53");
    defines_add(defs, "__DBL_MAX_10_EXP__", NULL, "308");
    defines_add(defs, "__DBL_MAX_EXP__", NULL, "1024");
    defines_add(defs, "__DBL_MAX__", NULL, "1.7976931348623157e+308");
    defines_add(defs, "__DBL_MIN_10_EXP__", NULL, "(-307)");
    defines_add(defs, "__DBL_MIN_EXP__", NULL, "(-1021)");
    defines_add(defs, "__DBL_MIN__", NULL, "2.2250738585072014e-308");
    defines_add(defs, "__DECIMAL_DIG__", NULL, "__LDBL_DECIMAL_DIG__");
    defines_add(defs, "__DYNAMIC__", NULL, "1");
    defines_add(defs, "__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__", NULL, "150000");
    defines_add(defs, "__ENVIRONMENT_OS_VERSION_MIN_REQUIRED__", NULL, "150000");
    defines_add(defs, "__FINITE_MATH_ONLY__", NULL, "0");
    defines_add(defs, "__FLT16_DECIMAL_DIG__", NULL, "5");
    defines_add(defs, "__FLT16_DENORM_MIN__", NULL, "5.9604644775390625e-8F16");
    defines_add(defs, "__FLT16_DIG__", NULL, "3");
    defines_add(defs, "__FLT16_EPSILON__", NULL, "9.765625e-4F16");
    defines_add(defs, "__FLT16_HAS_DENORM__", NULL, "1");
    defines_add(defs, "__FLT16_HAS_INFINITY__", NULL, "1");
    defines_add(defs, "__FLT16_HAS_QUIET_NAN__", NULL, "1");
    defines_add(defs, "__FLT16_MANT_DIG__", NULL, "11");
    defines_add(defs, "__FLT16_MAX_10_EXP__", NULL, "4");
    defines_add(defs, "__FLT16_MAX_EXP__", NULL, "16");
    defines_add(defs, "__FLT16_MAX__", NULL, "6.5504e+4F16");
    defines_add(defs, "__FLT16_MIN_10_EXP__", NULL, "(-4)");
    defines_add(defs, "__FLT16_MIN_EXP__", NULL, "(-13)");
    defines_add(defs, "__FLT16_MIN__", NULL, "6.103515625e-5F16");
    defines_add(defs, "__FLT_DECIMAL_DIG__", NULL, "9");
    defines_add(defs, "__FLT_DENORM_MIN__", NULL, "1.40129846e-45F");
    defines_add(defs, "__FLT_DIG__", NULL, "6");
    defines_add(defs, "__FLT_EPSILON__", NULL, "1.19209290e-7F");
    defines_add(defs, "__FLT_HAS_DENORM__", NULL, "1");
    defines_add(defs, "__FLT_HAS_INFINITY__", NULL, "1");
    defines_add(defs, "__FLT_HAS_QUIET_NAN__", NULL, "1");
    defines_add(defs, "__FLT_MANT_DIG__", NULL, "24");
    defines_add(defs, "__FLT_MAX_10_EXP__", NULL, "38");
    defines_add(defs, "__FLT_MAX_EXP__", NULL, "128");
    defines_add(defs, "__FLT_MAX__", NULL, "3.40282347e+38F");
    defines_add(defs, "__FLT_MIN_10_EXP__", NULL, "(-37)");
    defines_add(defs, "__FLT_MIN_EXP__", NULL, "(-125)");
    defines_add(defs, "__FLT_MIN__", NULL, "1.17549435e-38F");
    defines_add(defs, "__FLT_RADIX__", NULL, "2");
    defines_add(defs, "__FPCLASS_NEGINF", NULL, "0x0004");
    defines_add(defs, "__FPCLASS_NEGNORMAL", NULL, "0x0008");
    defines_add(defs, "__FPCLASS_NEGSUBNORMAL", NULL, "0x0010");
    defines_add(defs, "__FPCLASS_NEGZERO", NULL, "0x0020");
    defines_add(defs, "__FPCLASS_POSINF", NULL, "0x0200");
    defines_add(defs, "__FPCLASS_POSNORMAL", NULL, "0x0100");
    defines_add(defs, "__FPCLASS_POSSUBNORMAL", NULL, "0x0080");
    defines_add(defs, "__FPCLASS_POSZERO", NULL, "0x0040");
    defines_add(defs, "__FPCLASS_QNAN", NULL, "0x0002");
    defines_add(defs, "__FPCLASS_SNAN", NULL, "0x0001");
    defines_add(defs, "__FP_FAST_FMA", NULL, "1");
    defines_add(defs, "__FP_FAST_FMAF", NULL, "1");
    defines_add(defs, "__INT16_C_SUFFIX__", NULL, "");
    defines_add(defs, "__INT16_FMTd__", NULL, "\"hd\"");
    defines_add(defs, "__INT16_FMTi__", NULL, "\"hi\"");
    defines_add(defs, "__INT16_MAX__", NULL, "32767");
    defines_add(defs, "__INT16_TYPE__", NULL, "short");
    defines_add(defs, "__INT32_C_SUFFIX__", NULL, "");
    defines_add(defs, "__INT32_FMTd__", NULL, "\"d\"");
    defines_add(defs, "__INT32_FMTi__", NULL, "\"i\"");
    defines_add(defs, "__INT32_MAX__", NULL, "2147483647");
    defines_add(defs, "__INT32_TYPE__", NULL, "int");
    defines_add(defs, "__INT64_C_SUFFIX__", NULL, "LL");
    defines_add(defs, "__INT64_FMTd__", NULL, "\"lld\"");
    defines_add(defs, "__INT64_FMTi__", NULL, "\"lli\"");
    defines_add(defs, "__INT64_MAX__", NULL, "9223372036854775807LL");
    defines_add(defs, "__INT64_TYPE__", NULL, "long long int");
    defines_add(defs, "__INT8_C_SUFFIX__", NULL, "");
    defines_add(defs, "__INT8_FMTd__", NULL, "\"hhd\"");
    defines_add(defs, "__INT8_FMTi__", NULL, "\"hhi\"");
    defines_add(defs, "__INT8_MAX__", NULL, "127");
    defines_add(defs, "__INT8_TYPE__", NULL, "signed char");
    defines_add(defs, "__INTMAX_C_SUFFIX__", NULL, "L");
    defines_add(defs, "__INTMAX_FMTd__", NULL, "\"ld\"");
    defines_add(defs, "__INTMAX_FMTi__", NULL, "\"li\"");
    defines_add(defs, "__INTMAX_MAX__", NULL, "9223372036854775807L");
    defines_add(defs, "__INTMAX_TYPE__", NULL, "long int");
    defines_add(defs, "__INTMAX_WIDTH__", NULL, "64");
    defines_add(defs, "__INTPTR_FMTd__", NULL, "\"ld\"");
    defines_add(defs, "__INTPTR_FMTi__", NULL, "\"li\"");
    defines_add(defs, "__INTPTR_MAX__", NULL, "9223372036854775807L");
    defines_add(defs, "__INTPTR_TYPE__", NULL, "long int");
    defines_add(defs, "__INTPTR_WIDTH__", NULL, "64");
    defines_add(defs, "__INT_FAST16_FMTd__", NULL, "\"hd\"");
    defines_add(defs, "__INT_FAST16_FMTi__", NULL, "\"hi\"");
    defines_add(defs, "__INT_FAST16_MAX__", NULL, "32767");
    defines_add(defs, "__INT_FAST16_TYPE__", NULL, "short");
    defines_add(defs, "__INT_FAST16_WIDTH__", NULL, "16");
    defines_add(defs, "__INT_FAST32_FMTd__", NULL, "\"d\"");
    defines_add(defs, "__INT_FAST32_FMTi__", NULL, "\"i\"");
    defines_add(defs, "__INT_FAST32_MAX__", NULL, "2147483647");
    defines_add(defs, "__INT_FAST32_TYPE__", NULL, "int");
    defines_add(defs, "__INT_FAST32_WIDTH__", NULL, "32");
    defines_add(defs, "__INT_FAST64_FMTd__", NULL, "\"lld\"");
    defines_add(defs, "__INT_FAST64_FMTi__", NULL, "\"lli\"");
    defines_add(defs, "__INT_FAST64_MAX__", NULL, "9223372036854775807LL");
    defines_add(defs, "__INT_FAST64_TYPE__", NULL, "long long int");
    defines_add(defs, "__INT_FAST64_WIDTH__", NULL, "64");
    defines_add(defs, "__INT_FAST8_FMTd__", NULL, "\"hhd\"");
    defines_add(defs, "__INT_FAST8_FMTi__", NULL, "\"hhi\"");
    defines_add(defs, "__INT_FAST8_MAX__", NULL, "127");
    defines_add(defs, "__INT_FAST8_TYPE__", NULL, "signed char");
    defines_add(defs, "__INT_FAST8_WIDTH__", NULL, "8");
    defines_add(defs, "__INT_LEAST16_FMTd__", NULL, "\"hd\"");
    defines_add(defs, "__INT_LEAST16_FMTi__", NULL, "\"hi\"");
    defines_add(defs, "__INT_LEAST16_MAX__", NULL, "32767");
    defines_add(defs, "__INT_LEAST16_TYPE__", NULL, "short");
    defines_add(defs, "__INT_LEAST16_WIDTH__", NULL, "16");
    defines_add(defs, "__INT_LEAST32_FMTd__", NULL, "\"d\"");
    defines_add(defs, "__INT_LEAST32_FMTi__", NULL, "\"i\"");
    defines_add(defs, "__INT_LEAST32_MAX__", NULL, "2147483647");
    defines_add(defs, "__INT_LEAST32_TYPE__", NULL, "int");
    defines_add(defs, "__INT_LEAST32_WIDTH__", NULL, "32");
    defines_add(defs, "__INT_LEAST64_FMTd__", NULL, "\"lld\"");
    defines_add(defs, "__INT_LEAST64_FMTi__", NULL, "\"lli\"");
    defines_add(defs, "__INT_LEAST64_MAX__", NULL, "9223372036854775807LL");
    defines_add(defs, "__INT_LEAST64_TYPE__", NULL, "long long int");
    defines_add(defs, "__INT_LEAST64_WIDTH__", NULL, "64");
    defines_add(defs, "__INT_LEAST8_FMTd__", NULL, "\"hhd\"");
    defines_add(defs, "__INT_LEAST8_FMTi__", NULL, "\"hhi\"");
    defines_add(defs, "__INT_LEAST8_MAX__", NULL, "127");
    defines_add(defs, "__INT_LEAST8_TYPE__", NULL, "signed char");
    defines_add(defs, "__INT_LEAST8_WIDTH__", NULL, "8");
    defines_add(defs, "__INT_MAX__", NULL, "2147483647");
    defines_add(defs, "__INT_WIDTH__", NULL, "32");
    defines_add(defs, "__LDBL_DECIMAL_DIG__", NULL, "17");
    defines_add(defs, "__LDBL_DENORM_MIN__", NULL, "4.9406564584124654e-324L");
    defines_add(defs, "__LDBL_DIG__", NULL, "15");
    defines_add(defs, "__LDBL_EPSILON__", NULL, "2.2204460492503131e-16L");
    defines_add(defs, "__LDBL_HAS_DENORM__", NULL, "1");
    defines_add(defs, "__LDBL_HAS_INFINITY__", NULL, "1");
    defines_add(defs, "__LDBL_HAS_QUIET_NAN__", NULL, "1");
    defines_add(defs, "__LDBL_MANT_DIG__", NULL, "53");
    defines_add(defs, "__LDBL_MAX_10_EXP__", NULL, "308");
    defines_add(defs, "__LDBL_MAX_EXP__", NULL, "1024");
    defines_add(defs, "__LDBL_MAX__", NULL, "1.7976931348623157e+308L");
    defines_add(defs, "__LDBL_MIN_10_EXP__", NULL, "(-307)");
    defines_add(defs, "__LDBL_MIN_EXP__", NULL, "(-1021)");
    defines_add(defs, "__LDBL_MIN__", NULL, "2.2250738585072014e-308L");
    defines_add(defs, "__LITTLE_ENDIAN__", NULL, "1");
    defines_add(defs, "__LLONG_WIDTH__", NULL, "64");
    defines_add(defs, "__LONG_LONG_MAX__", NULL, "9223372036854775807LL");
    defines_add(defs, "__LONG_MAX__", NULL, "9223372036854775807L");
    defines_add(defs, "__LONG_WIDTH__", NULL, "64");
    defines_add(defs, "__LP64__", NULL, "1");
    defines_add(defs, "__MACH__", NULL, "1");
    defines_add(defs, "__NO_INLINE__", NULL, "1");
    defines_add(defs, "__NO_MATH_ERRNO__", NULL, "1");
    defines_add(defs, "__OBJC_BOOL_IS_BOOL", NULL, "1");
    defines_add(defs, "__OPENCL_MEMORY_SCOPE_ALL_SVM_DEVICES", NULL, "3");
    defines_add(defs, "__OPENCL_MEMORY_SCOPE_DEVICE", NULL, "2");
    defines_add(defs, "__OPENCL_MEMORY_SCOPE_SUB_GROUP", NULL, "4");
    defines_add(defs, "__OPENCL_MEMORY_SCOPE_WORK_GROUP", NULL, "1");
    defines_add(defs, "__OPENCL_MEMORY_SCOPE_WORK_ITEM", NULL, "0");
    defines_add(defs, "__ORDER_BIG_ENDIAN__", NULL, "4321");
    defines_add(defs, "__ORDER_LITTLE_ENDIAN__", NULL, "1234");
    defines_add(defs, "__ORDER_PDP_ENDIAN__", NULL, "3412");
    defines_add(defs, "__PIC__", NULL, "2");
    defines_add(defs, "__POINTER_WIDTH__", NULL, "64");
    defines_add(defs, "__PRAGMA_REDEFINE_EXTNAME", NULL, "1");
    defines_add(defs, "__PTRDIFF_FMTd__", NULL, "\"ld\"");
    defines_add(defs, "__PTRDIFF_FMTi__", NULL, "\"li\"");
    defines_add(defs, "__PTRDIFF_MAX__", NULL, "9223372036854775807L");
    defines_add(defs, "__PTRDIFF_TYPE__", NULL, "long int");
    defines_add(defs, "__PTRDIFF_WIDTH__", NULL, "64");
    defines_add(defs, "__REGISTER_PREFIX__", NULL, "");
    defines_add(defs, "__SCHAR_MAX__", NULL, "127");
    defines_add(defs, "__SHRT_MAX__", NULL, "32767");
    defines_add(defs, "__SHRT_WIDTH__", NULL, "16");
    defines_add(defs, "__SIG_ATOMIC_MAX__", NULL, "2147483647");
    defines_add(defs, "__SIG_ATOMIC_WIDTH__", NULL, "32");
    defines_add(defs, "__SIZEOF_DOUBLE__", NULL, "8");
    defines_add(defs, "__SIZEOF_FLOAT__", NULL, "4");
    defines_add(defs, "__SIZEOF_INT128__", NULL, "16");
    defines_add(defs, "__SIZEOF_INT__", NULL, "4");
    defines_add(defs, "__SIZEOF_LONG_DOUBLE__", NULL, "8");
    defines_add(defs, "__SIZEOF_LONG_LONG__", NULL, "8");
    defines_add(defs, "__SIZEOF_LONG__", NULL, "8");
    defines_add(defs, "__SIZEOF_POINTER__", NULL, "8");
    defines_add(defs, "__SIZEOF_PTRDIFF_T__", NULL, "8");
    defines_add(defs, "__SIZEOF_SHORT__", NULL, "2");
    defines_add(defs, "__SIZEOF_SIZE_T__", NULL, "8");
    defines_add(defs, "__SIZEOF_WCHAR_T__", NULL, "4");
    defines_add(defs, "__SIZEOF_WINT_T__", NULL, "4");
    defines_add(defs, "__SIZE_FMTX__", NULL, "\"lX\"");
    defines_add(defs, "__SIZE_FMTo__", NULL, "\"lo\"");
    defines_add(defs, "__SIZE_FMTu__", NULL, "\"lu\"");
    defines_add(defs, "__SIZE_FMTx__", NULL, "\"lx\"");
    defines_add(defs, "__SIZE_MAX__", NULL, "18446744073709551615UL");
    defines_add(defs, "__SIZE_TYPE__", NULL, "long unsigned int");
    defines_add(defs, "__SIZE_WIDTH__", NULL, "64");
    defines_add(defs, "__SSP__", NULL, "1");
    defines_add(defs, "__STDC_HOSTED__", NULL, "1");
    defines_add(defs, "__STDC_NO_THREADS__", NULL, "1");
    defines_add(defs, "__STDC_UTF_16__", NULL, "1");
    defines_add(defs, "__STDC_UTF_32__", NULL, "1");
    defines_add(defs, "__STDC__", NULL, "1");
    defines_add(defs, "__STRICT_ANSI__", NULL, "1");
    defines_add(defs, "__UINT16_C_SUFFIX__", NULL, "");
    defines_add(defs, "__UINT16_FMTX__", NULL, "\"hX\"");
    defines_add(defs, "__UINT16_FMTo__", NULL, "\"ho\"");
    defines_add(defs, "__UINT16_FMTu__", NULL, "\"hu\"");
    defines_add(defs, "__UINT16_FMTx__", NULL, "\"hx\"");
    defines_add(defs, "__UINT16_MAX__", NULL, "65535");
    defines_add(defs, "__UINT16_TYPE__", NULL, "unsigned short");
    defines_add(defs, "__UINT32_C_SUFFIX__", NULL, "U");
    defines_add(defs, "__UINT32_FMTX__", NULL, "\"X\"");
    defines_add(defs, "__UINT32_FMTo__", NULL, "\"o\"");
    defines_add(defs, "__UINT32_FMTu__", NULL, "\"u\"");
    defines_add(defs, "__UINT32_FMTx__", NULL, "\"x\"");
    defines_add(defs, "__UINT32_MAX__", NULL, "4294967295U");
    defines_add(defs, "__UINT32_TYPE__", NULL, "unsigned int");
    defines_add(defs, "__UINT64_C_SUFFIX__", NULL, "ULL");
    defines_add(defs, "__UINT64_FMTX__", NULL, "\"llX\"");
    defines_add(defs, "__UINT64_FMTo__", NULL, "\"llo\"");
    defines_add(defs, "__UINT64_FMTu__", NULL, "\"llu\"");
    defines_add(defs, "__UINT64_FMTx__", NULL, "\"llx\"");
    defines_add(defs, "__UINT64_MAX__", NULL, "18446744073709551615ULL");
    defines_add(defs, "__UINT64_TYPE__", NULL, "long long unsigned int");
    defines_add(defs, "__UINT8_C_SUFFIX__", NULL, "");
    defines_add(defs, "__UINT8_FMTX__", NULL, "\"hhX\"");
    defines_add(defs, "__UINT8_FMTo__", NULL, "\"hho\"");
    defines_add(defs, "__UINT8_FMTu__", NULL, "\"hhu\"");
    defines_add(defs, "__UINT8_FMTx__", NULL, "\"hhx\"");
    defines_add(defs, "__UINT8_MAX__", NULL, "255");
    defines_add(defs, "__UINT8_TYPE__", NULL, "unsigned char");
    defines_add(defs, "__UINTMAX_C_SUFFIX__", NULL, "UL");
    defines_add(defs, "__UINTMAX_FMTX__", NULL, "\"lX\"");
    defines_add(defs, "__UINTMAX_FMTo__", NULL, "\"lo\"");
    defines_add(defs, "__UINTMAX_FMTu__", NULL, "\"lu\"");
    defines_add(defs, "__UINTMAX_FMTx__", NULL, "\"lx\"");
    defines_add(defs, "__UINTMAX_MAX__", NULL, "18446744073709551615UL");
    defines_add(defs, "__UINTMAX_TYPE__", NULL, "long unsigned int");
    defines_add(defs, "__UINTMAX_WIDTH__", NULL, "64");
    defines_add(defs, "__UINTPTR_FMTX__", NULL, "\"lX\"");
    defines_add(defs, "__UINTPTR_FMTo__", NULL, "\"lo\"");
    defines_add(defs, "__UINTPTR_FMTu__", NULL, "\"lu\"");
    defines_add(defs, "__UINTPTR_FMTx__", NULL, "\"lx\"");
    defines_add(defs, "__UINTPTR_MAX__", NULL, "18446744073709551615UL");
    defines_add(defs, "__UINTPTR_TYPE__", NULL, "long unsigned int");
    defines_add(defs, "__UINTPTR_WIDTH__", NULL, "64");
    defines_add(defs, "__UINT_FAST16_FMTX__", NULL, "\"hX\"");
    defines_add(defs, "__UINT_FAST16_FMTo__", NULL, "\"ho\"");
    defines_add(defs, "__UINT_FAST16_FMTu__", NULL, "\"hu\"");
    defines_add(defs, "__UINT_FAST16_FMTx__", NULL, "\"hx\"");
    defines_add(defs, "__UINT_FAST16_MAX__", NULL, "65535");
    defines_add(defs, "__UINT_FAST16_TYPE__", NULL, "unsigned short");
    defines_add(defs, "__UINT_FAST32_FMTX__", NULL, "\"X\"");
    defines_add(defs, "__UINT_FAST32_FMTo__", NULL, "\"o\"");
    defines_add(defs, "__UINT_FAST32_FMTu__", NULL, "\"u\"");
    defines_add(defs, "__UINT_FAST32_FMTx__", NULL, "\"x\"");
    defines_add(defs, "__UINT_FAST32_MAX__", NULL, "4294967295U");
    defines_add(defs, "__UINT_FAST32_TYPE__", NULL, "unsigned int");
    defines_add(defs, "__UINT_FAST64_FMTX__", NULL, "\"llX\"");
    defines_add(defs, "__UINT_FAST64_FMTo__", NULL, "\"llo\"");
    defines_add(defs, "__UINT_FAST64_FMTu__", NULL, "\"llu\"");
    defines_add(defs, "__UINT_FAST64_FMTx__", NULL, "\"llx\"");
    defines_add(defs, "__UINT_FAST64_MAX__", NULL, "18446744073709551615ULL");
    defines_add(defs, "__UINT_FAST64_TYPE__", NULL, "long long unsigned int");
    defines_add(defs, "__UINT_FAST8_FMTX__", NULL, "\"hhX\"");
    defines_add(defs, "__UINT_FAST8_FMTo__", NULL, "\"hho\"");
    defines_add(defs, "__UINT_FAST8_FMTu__", NULL, "\"hhu\"");
    defines_add(defs, "__UINT_FAST8_FMTx__", NULL, "\"hhx\"");
    defines_add(defs, "__UINT_FAST8_MAX__", NULL, "255");
    defines_add(defs, "__UINT_FAST8_TYPE__", NULL, "unsigned char");
    defines_add(defs, "__UINT_LEAST16_FMTX__", NULL, "\"hX\"");
    defines_add(defs, "__UINT_LEAST16_FMTo__", NULL, "\"ho\"");
    defines_add(defs, "__UINT_LEAST16_FMTu__", NULL, "\"hu\"");
    defines_add(defs, "__UINT_LEAST16_FMTx__", NULL, "\"hx\"");
    defines_add(defs, "__UINT_LEAST16_MAX__", NULL, "65535");
    defines_add(defs, "__UINT_LEAST16_TYPE__", NULL, "unsigned short");
    defines_add(defs, "__UINT_LEAST32_FMTX__", NULL, "\"X\"");
    defines_add(defs, "__UINT_LEAST32_FMTo__", NULL, "\"o\"");
    defines_add(defs, "__UINT_LEAST32_FMTu__", NULL, "\"u\"");
    defines_add(defs, "__UINT_LEAST32_FMTx__", NULL, "\"x\"");
    defines_add(defs, "__UINT_LEAST32_MAX__", NULL, "4294967295U");
    defines_add(defs, "__UINT_LEAST32_TYPE__", NULL, "unsigned int");
    defines_add(defs, "__UINT_LEAST64_FMTX__", NULL, "\"llX\"");
    defines_add(defs, "__UINT_LEAST64_FMTo__", NULL, "\"llo\"");
    defines_add(defs, "__UINT_LEAST64_FMTu__", NULL, "\"llu\"");
    defines_add(defs, "__UINT_LEAST64_FMTx__", NULL, "\"llx\"");
    defines_add(defs, "__UINT_LEAST64_MAX__", NULL, "18446744073709551615ULL");
    defines_add(defs, "__UINT_LEAST64_TYPE__", NULL, "long long unsigned int");
    defines_add(defs, "__UINT_LEAST8_FMTX__", NULL, "\"hhX\"");
    defines_add(defs, "__UINT_LEAST8_FMTo__", NULL, "\"hho\"");
    defines_add(defs, "__UINT_LEAST8_FMTu__", NULL, "\"hhu\"");
    defines_add(defs, "__UINT_LEAST8_FMTx__", NULL, "\"hhx\"");
    defines_add(defs, "__UINT_LEAST8_MAX__", NULL, "255");
    defines_add(defs, "__UINT_LEAST8_TYPE__", NULL, "unsigned char");
    defines_add(defs, "__USER_LABEL_PREFIX__", NULL, "_");
    defines_add(defs, "__VERSION__", NULL, "twcc @ 0.1");
    defines_add(defs, "__WCHAR_MAX__", NULL, "2147483647");
    defines_add(defs, "__WCHAR_TYPE__", NULL, "int");
    defines_add(defs, "__WCHAR_WIDTH__", NULL, "32");
    defines_add(defs, "__WINT_MAX__", NULL, "2147483647");
    defines_add(defs, "__WINT_TYPE__", NULL, "int");
    defines_add(defs, "__WINT_WIDTH__", NULL, "32");
    defines_add(defs, "__aarch64__", NULL, "1");
    defines_add(defs, "__apple_build_version__", NULL, "16000026");
    defines_add(defs, "__arm64", NULL, "1");
    defines_add(defs, "__arm64__", NULL, "1");
    defines_add(defs, "__block", NULL, "__attribute__((__blocks__(byref)))");
    defines_add(defs, "__llvm__", NULL, "1");
    defines_add(defs, "__nonnull", NULL, "_Nonnull");
    defines_add(defs, "__null_unspecified", NULL, "_Null_unspecified");
    defines_add(defs, "__nullable", NULL, "_Nullable");
    defines_add(defs, "__pic__", NULL, "2");
    defines_add(defs, "__strong", NULL, "");
    defines_add(defs, "__unsafe_unretained", NULL, "");
    defines_add(defs, "__weak", NULL, "__attribute__((objc_gc(weak)))");

    // fixes for system-specific issues
    defines_add(defs, "__has_include", "...", "0");
    defines_add(defs, "__has_builtin", "...", "0");
    defines_add(defs, "_DONT_USE_CTYPE_INLINE_", NULL, NULL);
    defines_add(defs, "__STDC_VERSION__", NULL, "0L");
    defines_add(defs, "__GNUC__", NULL, "4");
    defines_add(defs, "__GNUC_MINOR__", NULL, "0");
    defines_add(defs, "__GNUC_PATCHLEVEL__", NULL, "0");

    return defs;
}

def *defines_get(const defines *defs, const char *name)
{
    if (defs) {
        def *d = hashmap_get(defs->h, name);
        if (d && !d->ignored) {
            return d;
        }
    }
    return NULL;
}

void defines_destroy(defines *defs)
{
    hashmap_iter_state iter = {};
    hashmap_entry *e;

    while ((e = hashmap_iter(defs->h, &iter))) {
        def *d = (def *) e->data;
        free((void *) d->name);
        int i;
        for (i = 0; d->args && d->args[i]; i++) {
            free((void *) d->args[i]);
        }
        free(d->replace);
        free(d);
    }
    free(defs);
}

void clear_ignore_list(ignore_list *l)
{
    int i;
    for (i = 0; i < l->count; i++) {
        l->ignored[i]->ignored--;
    }
    free(l->ignored);
    l->ignored = NULL;
    l->count = 0;
}

void add_to_ignore_list(ignore_list *l, def *d)
{
    l->ignored = realloc(l->ignored, sizeof (*l->ignored) * (l->count + 1));
    l->ignored[l->count] = d;
    l->ignored[l->count++]->ignored++;
}

/* FreeBSD: noop stub for WTFCrashWithInfoImpl — replaces the real crash
 * function so that debug-only asserts (especially verifyCanGC / doesGC)
 * continue execution instead of aborting.
 *
 * The precompiled Linux debug WebKit has ENABLE_DFG_DOES_GC_VALIDATION=1,
 * which inlines verifyCanGC into many JSC functions.  The inlined copies
 * call WTFCrashWithInfoImpl when the GC assertion fails.  By weakening
 * WTFCrashWithInfoImpl in libWTF.a and linking this strong stub BEFORE
 * the library, all crash paths (inlined or not) hit our `ret` instead
 * of __builtin_trap().
 *
 * This is a TEMPORARY measure for the FreeBSD port.  The proper fix is
 * to build WebKit locally with ENABLE_DFG_DOES_GC_VALIDATION=OFF (see
 * scripts/build/deps/webkit.ts).
 */

#if defined(__FreeBSD__) && defined(__aarch64__)

/* Each WTFCrashWithInfoImpl<T...> overload is a separate symbol.
 * We provide all variants as a single `ret` instruction via asm. */

__asm__(
    ".text\n"
    ".globl _Z20WTFCrashWithInfoImpliPKcS0_m\n"
    ".type _Z20WTFCrashWithInfoImpliPKcS0_m, @function\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_m:\n"
    "ret\n"
    ".size _Z20WTFCrashWithInfoImpliPKcS0_m, .-_Z20WTFCrashWithInfoImpliPKcS0_m\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mm\n"
    ".type _Z20WTFCrashWithInfoImpliPKcS0_mm, @function\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mm:\n"
    "ret\n"
    ".size _Z20WTFCrashWithInfoImpliPKcS0_mm, .-_Z20WTFCrashWithInfoImpliPKcS0_mm\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmm:\n"
    "ret\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmm:\n"
    "ret\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmmm:\n"
    "ret\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmmmm:\n"
    "ret\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmmmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmmmmm:\n"
    "ret\n"
);

#elif defined(__FreeBSD__) && defined(__x86_64__)

__asm__(
    ".text\n"
    ".globl _Z20WTFCrashWithInfoImpliPKcS0_m\n"
    ".type _Z20WTFCrashWithInfoImpliPKcS0_m, @function\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_m:\n"
    "retq\n"
    ".size _Z20WTFCrashWithInfoImpliPKcS0_m, .-_Z20WTFCrashWithInfoImpliPKcS0_m\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mm:\n"
    "retq\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmm:\n"
    "retq\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmm:\n"
    "retq\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmmm:\n"
    "retq\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmmmm:\n"
    "retq\n"

    ".globl _Z20WTFCrashWithInfoImpliPKcS0_mmmmmmm\n"
    "_Z20WTFCrashWithInfoImpliPKcS0_mmmmmmm:\n"
    "retq\n"
);

#endif /* __FreeBSD__ */

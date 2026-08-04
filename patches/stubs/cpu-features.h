/*
 * Minimal cpu-features.h stub for building Node.js for Android.
 *
 * NDK r26+ removed sources/android/cpufeatures, but zlib's cpu_features.c
 * still does `#include <cpu-features.h>` on ARMV8_OS_ANDROID.
 *
 * android_getCpuFeatures() returns 0, so zlib disables CRC32/PMULL SIMD
 * and falls back to portable code. Safe, just slightly slower.
 */
#ifndef ANDROID_CPU_FEATURES_H
#define ANDROID_CPU_FEATURES_H

#include <stdint.h>

typedef enum {
  ANDROID_CPU_FAMILY_UNKNOWN = 0,
  ANDROID_CPU_FAMILY_ARM = 1,
  ANDROID_CPU_FAMILY_X86 = 2,
  ANDROID_CPU_FAMILY_MIPS = 3,
  ANDROID_CPU_FAMILY_ARM64 = 4,
  ANDROID_CPU_FAMILY_X86_64 = 5,
} AndroidCpuFamily;

typedef uint64_t AndroidCpuFeatures;

#define ANDROID_CPU_ARM_FEATURE_ARMv7 (1 << 0)
#define ANDROID_CPU_ARM_FEATURE_VFPv3 (1 << 1)
#define ANDROID_CPU_ARM_FEATURE_NEON (1 << 2)
#define ANDROID_CPU_ARM_FEATURE_LDREX (1 << 3)
#define ANDROID_CPU_ARM_FEATURE_CRC32 (1 << 12)
#define ANDROID_CPU_ARM_FEATURE_PMULL (1 << 13)

#define ANDROID_CPU_ARM64_FEATURE_CRC32 (1 << 0)
#define ANDROID_CPU_ARM64_FEATURE_PMULL (1 << 1)

static inline AndroidCpuFamily android_getCpuFamily(void) {
  return ANDROID_CPU_FAMILY_UNKNOWN;
}

static inline AndroidCpuFeatures android_getCpuFeatures(void) { return 0; }

static inline int android_getCpuCount(void) { return 1; }

#endif /* ANDROID_CPU_FEATURES_H */

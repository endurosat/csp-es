#pragma once

/**
 * @file csp_version.h
 * @Description: CSP library version reporting API.
 *
 * Exposes the library version that was set at build time from the version file
 * maintained by internal/scripts/update_version.py — the same version string
 * used by CI packaging and Go-comms builds.
 *
 * Preprocessor constants (CSP_VERSION_MAJOR/MINOR/PATCH) allow compile-time
 * version checks in downstream code via #if directives.  The runtime function
 * csp_get_version() provides the same values plus the full version string
 * (e.g. "1.2.3-rc.1") for logging or protocol negotiation.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Major version number (compile-time constant).
 *
 * Defined by the build system via a PUBLIC compile definition so that both the
 * library and its consumers receive the value.  Usable in preprocessor #if
 * guards.  Falls back to 0 if the build system did not inject it (e.g. when
 * the header is included without going through cmake/version.cmake).
 */
#ifndef CSP_VERSION_MAJOR
#define CSP_VERSION_MAJOR 0
#endif

/** @brief Minor version number (compile-time constant). See CSP_VERSION_MAJOR. */
#ifndef CSP_VERSION_MINOR
#define CSP_VERSION_MINOR 0
#endif

/** @brief Patch version number (compile-time constant). See CSP_VERSION_MAJOR. */
#ifndef CSP_VERSION_PATCH
#define CSP_VERSION_PATCH 0
#endif

/**
 * CSP library version information returned at runtime.
 */
typedef struct csp_version_info_s {
    uint16_t major;              /**< Major version number */
    uint16_t minor;              /**< Minor version number */
    uint16_t patch;              /**< Patch version number */
    const char * version_string; /**< Full version string, e.g. "1.2.3-rc.1" */
} csp_version_info_t;

/**
 * Get CSP library version information.
 *
 * The returned pointer references a static structure that is valid for the
 * lifetime of the process.  The caller must not modify or free it.
 *
 * @return Pointer to read-only version information.
 */
const csp_version_info_t * csp_get_version(void);

#ifdef __cplusplus
}
#endif

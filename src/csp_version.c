#include <csp/csp_version.h>

/*
 * SW_VERSION_MAJOR, SW_VERSION_MINOR, SW_VERSION_PATCH, and SW_VERSION_STRING
 * are injected as PRIVATE compile definitions by add_version_definitions_to_target()
 * in cmake/version.cmake.  The fallback values below cover standalone submodule
 * builds that do not go through the parent project's versioning machinery.
 */

#ifndef SW_VERSION_MAJOR
#define SW_VERSION_MAJOR 0
#endif

#ifndef SW_VERSION_MINOR
#define SW_VERSION_MINOR 0
#endif

#ifndef SW_VERSION_PATCH
#define SW_VERSION_PATCH 0
#endif

#ifndef SW_VERSION_STRING
#define SW_VERSION_STRING "0.0.0-unknown"
#endif

static const csp_version_info_t version_info = {
    .major          = SW_VERSION_MAJOR,
    .minor          = SW_VERSION_MINOR,
    .patch          = SW_VERSION_PATCH,
    .version_string = SW_VERSION_STRING,
};

const csp_version_info_t * csp_get_version(void) {
    return &version_info;
}

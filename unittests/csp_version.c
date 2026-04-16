#include <check.h>
#include <stdio.h>
#include <string.h>
#include "../include/csp/csp_version.h"

START_TEST(test_version_ptr_nonnull)
{
    ck_assert_ptr_nonnull(csp_get_version());
}
END_TEST

START_TEST(test_version_ptr_stable)
{
    ck_assert_ptr_eq(csp_get_version(), csp_get_version());
}
END_TEST

START_TEST(test_version_fields_match_macros)
{
    const csp_version_info_t *v = csp_get_version();
    ck_assert_int_eq(v->major, CSP_VERSION_MAJOR);
    ck_assert_int_eq(v->minor, CSP_VERSION_MINOR);
    ck_assert_int_eq(v->patch, CSP_VERSION_PATCH);
}
END_TEST

START_TEST(test_version_string_nonnull)
{
    ck_assert_ptr_nonnull(csp_get_version()->version_string);
}
END_TEST

START_TEST(test_version_string_nonempty)
{
    ck_assert(strlen(csp_get_version()->version_string) > 0);
}
END_TEST

START_TEST(test_version_string_contains_major_minor_patch)
{
    const csp_version_info_t *v = csp_get_version();
    char expected_prefix[32];
    snprintf(expected_prefix, sizeof(expected_prefix),
             "%u.%u.%u", v->major, v->minor, v->patch);
    ck_assert_ptr_nonnull(strstr(v->version_string, expected_prefix));
}
END_TEST

Suite * csp_version_suite(void)
{
    Suite *s = suite_create("CSP Version");
    TCase *tc = tcase_create("version info");

    tcase_add_test(tc, test_version_ptr_nonnull);
    tcase_add_test(tc, test_version_ptr_stable);
    tcase_add_test(tc, test_version_fields_match_macros);
    tcase_add_test(tc, test_version_string_nonnull);
    tcase_add_test(tc, test_version_string_nonempty);
    tcase_add_test(tc, test_version_string_contains_major_minor_patch);

    suite_add_tcase(s, tc);
    return s;
}

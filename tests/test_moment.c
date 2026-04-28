#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../src/moment.h"

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond, msg)                                    \
    do                                                      \
    {                                                       \
        if (cond)                                           \
        {                                                   \
            g_passed++;                                     \
        }                                                   \
        else                                                \
        {                                                   \
            g_failed++;                                     \
            printf("[FAIL] %s\n", msg);                    \
        }                                                   \
    } while (0)

static void test_create_get_clear(void)
{
    pMoment now = Moment_Now();
    CHECK(now != NULL, "Moment_Now should not return NULL");
    if (now != NULL)
    {
        CHECK(Moment_Get_Millisecond(now) >= Moment_Get_Sec(now) * 1000, "Milliseconds should be >= sec*1000");
        Moment_Clear(now);
    }

    pMoment sec = Moment_Second(1628578601);
    CHECK(sec != NULL, "Moment_Second should not return NULL");
    if (sec != NULL)
    {
        CHECK(Moment_Get_Sec(sec) == 1628578601, "Moment_Second sec mismatch");
        Moment_Clear(sec);
    }

    pMoment ms = Moment_Millisecond(1628578601123);
    CHECK(ms != NULL, "Moment_Millisecond should not return NULL");
    if (ms != NULL)
    {
        CHECK(Moment_Get_Millisecond(ms) == 1628578601123, "Moment_Millisecond value mismatch");
        Moment_Clear(ms);
    }

    Moment_Clear(NULL);
    CHECK(Moment_Get_Sec(NULL) == 0, "Moment_Get_Sec(NULL) should be 0");
    CHECK(Moment_Get_Millisecond(NULL) == 0, "Moment_Get_Millisecond(NULL) should be 0");
}

static void test_clone_and_set_clone(void)
{
    pMoment src = Moment_Millisecond(1700000000123);
    CHECK(src != NULL, "Source moment allocation failed");
    if (src == NULL)
    {
        return;
    }

    char *src_fmt = Moment_Format(src, "YYYY-MM-DD HH:mm:ss.SSS");
    CHECK(src_fmt != NULL && strcmp(src_fmt, "Invalid date") != 0, "Moment_Format on source should work");

    pMoment copy = Moment_Clone(src);
    CHECK(copy != NULL, "Moment_Clone should not return NULL");
    if (copy != NULL)
    {
        CHECK(Moment_Get_Millisecond(copy) == Moment_Get_Millisecond(src), "Clone should preserve timestamp");
        Moment_Clear(copy);
    }

    pMoment dst = Moment_Now();
    CHECK(dst != NULL, "Destination moment allocation failed");
    if (dst != NULL)
    {
        pMoment set_ret = Moment_Set_Clone(dst, src);
        CHECK(set_ret == dst, "Moment_Set_Clone should return destination pointer");
        CHECK(Moment_Get_Millisecond(dst) == Moment_Get_Millisecond(src), "Moment_Set_Clone should copy timestamp");
        Moment_Clear(dst);
    }

    CHECK(Moment_Clone(NULL) == NULL, "Moment_Clone(NULL) should be NULL");
    CHECK(Moment_Set_Clone(NULL, src) == NULL, "Moment_Set_Clone(NULL, src) should be NULL");
    CHECK(Moment_Set_Clone(src, NULL) == NULL, "Moment_Set_Clone(src, NULL) should be NULL");

    Moment_Clear(src);
}

static void test_parse_and_format(void)
{
    pMoment p1 = Moment_Parse("2013-02-08 09:30:26.123+08:00");
    CHECK(p1 != NULL, "Moment_Parse valid datetime should succeed");
    if (p1 != NULL)
    {
        char *f1 = Moment_Format(p1, "YYYY-MM-DDTHH:mm:ss.SSSZ");
        CHECK(f1 != NULL, "Moment_Format should not return NULL");
        CHECK(strstr(f1, "2013-02-08T09:30:26.123") != NULL, "Formatted time text mismatch");
        CHECK(strstr(f1, "+08:00") != NULL, "Formatted timezone mismatch");

        char *f2 = Moment_Format(p1, "H:m:s");
        CHECK(f2 != NULL && strstr(f2, "9:30:26") != NULL, "Single token minute format 'm' should work");

        char *sf = Moment_strftime(p1, "%Y-%m-%d %H:%M:%S");
        CHECK(sf != NULL && strlen(sf) > 0, "Moment_strftime should produce output");

        char *def = Moment_Format(p1, NULL);
        CHECK(def != NULL && strlen(def) > 0, "Moment_Format default format should produce output");

        Moment_Clear(p1);
    }

    CHECK(Moment_Parse("not-a-date") == NULL, "Invalid parse should return NULL");
    CHECK(Moment_Parse(NULL) == NULL, "Moment_Parse(NULL) should return NULL");

    pMoment p2 = Moment_Second(0);
    CHECK(p2 != NULL, "Moment_Second(0) allocation failed");
    if (p2 != NULL)
    {
        CHECK(strcmp(Moment_Format(NULL, "YYYY"), "Invalid date") == 0, "Moment_Format(NULL, ..) should be Invalid date");
        CHECK(strcmp(Moment_strftime(NULL, "%Y"), "Invalid date") == 0, "Moment_strftime(NULL, ..) should be Invalid date");
        Moment_Clear(p2);
    }
}

static void test_set_get_offset_min_max(void)
{
    pMoment a = Moment_Second(100);
    pMoment b = Moment_Second(200);
    pMoment c = Moment_Millisecond(200500);
    CHECK(a != NULL && b != NULL && c != NULL, "Allocation for min/max tests failed");

    if (a != NULL)
    {
        CHECK(Moment_Set_utcOffset(a, 28800) == a, "Moment_Set_utcOffset should return self");
        char *zone = Moment_Format(a, "Z");
        CHECK(zone != NULL && strstr(zone, "+08:00") != NULL, "UTC offset format mismatch");
    }

    if (a != NULL && b != NULL && c != NULL)
    {
        pMoment mx = Moment_Max(a, b, c, NULL);
        pMoment mn = Moment_Min(a, b, c, NULL);
        CHECK(mx == c, "Moment_Max should return latest moment");
        CHECK(mn == a, "Moment_Min should return earliest moment");
    }

    Moment_Clear(a);
    Moment_Clear(b);
    Moment_Clear(c);
}

static void test_add_subtract_start_end_of(void)
{
    pMoment p = Moment_Parse("2021-08-09 11:45:30.250+00:00");
    CHECK(p != NULL, "Moment_Parse for add/subtract test failed");
    if (p == NULL)
    {
        return;
    }

    Moment_Add(p, 1, "days");
    CHECK(strstr(Moment_Format(p, "YYYY-MM-DD"), "2021-08-10") != NULL, "Add days failed");

    Moment_Subtract(p, 2, "hours");
    CHECK(strstr(Moment_Format(p, "HH"), "09") != NULL, "Subtract hours failed");

    Moment_Add(p, 1500, "ms");
    CHECK(strstr(Moment_Format(p, "SSS"), "750") != NULL, "Add milliseconds failed");

    Moment_StartOf(p, "day");
    CHECK(strcmp(Moment_Format(p, "HH:mm:ss.SSS"), "00:00:00.000") == 0, "StartOf day failed");

    Moment_EndOf(p, "day");
    CHECK(strcmp(Moment_Format(p, "HH:mm:ss.SSS"), "23:59:59.999") == 0, "EndOf day failed");

    CHECK(Moment_Add(NULL, 1, "days") == NULL, "Moment_Add(NULL, ..) should return NULL");
    CHECK(Moment_Add(p, 1, NULL) == NULL, "Moment_Add(.., NULL) should return NULL");
    CHECK(Moment_Subtract(NULL, 1, "days") == NULL, "Moment_Subtract(NULL, ..) should return NULL");
    CHECK(Moment_StartOf(NULL, "day") == NULL, "Moment_StartOf(NULL, ..) should return NULL");
    CHECK(Moment_EndOf(NULL, "day") == NULL, "Moment_EndOf(NULL, ..) should return NULL");

    Moment_Clear(p);
}

static void test_iso_week(void)
{
    /* 2013-01-01 (Tuesday) -> ISO week 1 */
    pMoment p = Moment_Parse("2013-01-01T00:00:00Z");
    CHECK(p != NULL, "ISO week: parse 2013-01-01 failed");
    if (p != NULL)
    {
        CHECK(strcmp(Moment_Format(p, "W"), "1") == 0, "ISO W: 2013-01-01 should be 1");
        CHECK(strcmp(Moment_Format(p, "WW"), "01") == 0, "ISO WW: 2013-01-01 should be 01");
        Moment_Clear(p);
    }

    /* 2013-12-29 (Sunday) -> ISO week 52 */
    p = Moment_Parse("2013-12-29T00:00:00Z");
    CHECK(p != NULL, "ISO week: parse 2013-12-29 failed");
    if (p != NULL)
    {
        CHECK(strcmp(Moment_Format(p, "W"), "52") == 0, "ISO W: 2013-12-29 should be 52");
        Moment_Clear(p);
    }

    /* 2013-12-30 (Monday) -> belongs to ISO week 1 of 2014 */
    p = Moment_Parse("2013-12-30T00:00:00Z");
    CHECK(p != NULL, "ISO week: parse 2013-12-30 failed");
    if (p != NULL)
    {
        CHECK(strcmp(Moment_Format(p, "W"), "1") == 0, "ISO W: 2013-12-30 should be 1 (week 1 of 2014)");
        Moment_Clear(p);
    }

    /* 2015-12-31 (Thursday) -> ISO week 53 (2015 has a week 53) */
    p = Moment_Parse("2015-12-31T00:00:00Z");
    CHECK(p != NULL, "ISO week: parse 2015-12-31 failed");
    if (p != NULL)
    {
        CHECK(strcmp(Moment_Format(p, "W"), "53") == 0, "ISO W: 2015-12-31 should be 53");
        Moment_Clear(p);
    }

    /* 2016-01-04 (Monday) -> ISO week 1 */
    p = Moment_Parse("2016-01-04T00:00:00Z");
    CHECK(p != NULL, "ISO week: parse 2016-01-04 failed");
    if (p != NULL)
    {
        CHECK(strcmp(Moment_Format(p, "W"), "1") == 0, "ISO W: 2016-01-04 should be 1");
        Moment_Clear(p);
    }
}

static void test_diff(void)
{
    pMoment a = Moment_Parse("2021-08-10T00:00:00Z");
    pMoment b = Moment_Parse("2021-08-09T00:00:00Z");
    CHECK(a != NULL && b != NULL, "Moment_Diff: parse failed");
    if (a != NULL && b != NULL)
    {
        CHECK(Moment_Diff(a, b, "days")    ==  1,     "Diff days a-b should be 1");
        CHECK(Moment_Diff(b, a, "days")    == -1,     "Diff days b-a should be -1");
        CHECK(Moment_Diff(a, b, "hours")   == 24,     "Diff hours should be 24");
        CHECK(Moment_Diff(a, b, "minutes") == 1440,   "Diff minutes should be 1440");
        CHECK(Moment_Diff(a, b, "seconds") == 86400,  "Diff seconds should be 86400");
        CHECK(Moment_Diff(a, b, "ms")      == 86400000L, "Diff ms should be 86400000");
        CHECK(Moment_Diff(a, b, "weeks")   == 0,      "Diff weeks <7 days should be 0");
    }
    Moment_Clear(a);
    Moment_Clear(b);

    /* exact year boundary */
    pMoment y2 = Moment_Parse("2022-08-09T00:00:00Z");
    pMoment y1 = Moment_Parse("2021-08-09T00:00:00Z");
    CHECK(y2 != NULL && y1 != NULL, "Moment_Diff year: parse failed");
    if (y2 != NULL && y1 != NULL)
    {
        CHECK(Moment_Diff(y2, y1, "years")  ==  1, "Diff: exactly 1 year");
        CHECK(Moment_Diff(y1, y2, "years")  == -1, "Diff: exactly -1 year");
        CHECK(Moment_Diff(y2, y1, "months") == 12, "Diff: 12 months");
    }
    Moment_Clear(y2);
    Moment_Clear(y1);

    /* partial year: 2021-08-09 -> 2022-02-09 = 6 months, 0 full years */
    pMoment p1 = Moment_Parse("2022-02-09T00:00:00Z");
    pMoment p0 = Moment_Parse("2021-08-09T00:00:00Z");
    CHECK(p1 != NULL && p0 != NULL, "Moment_Diff partial: parse failed");
    if (p1 != NULL && p0 != NULL)
    {
        CHECK(Moment_Diff(p1, p0, "years")  == 0, "Diff partial year should be 0");
        CHECK(Moment_Diff(p1, p0, "months") == 6, "Diff partial months should be 6");
    }
    Moment_Clear(p1);
    Moment_Clear(p0);

    /* NULL safety */
    CHECK(Moment_Diff(NULL, NULL, "days") == 0, "Moment_Diff(NULL,NULL) should be 0");
}

int main(void)
{
    test_create_get_clear();
    test_clone_and_set_clone();
    test_parse_and_format();
    test_set_get_offset_min_max();
    test_add_subtract_start_end_of();
    test_iso_week();
    test_diff();

    printf("[RESULT] passed=%d failed=%d\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}

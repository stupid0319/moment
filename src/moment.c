#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>

#include "moment.h"

static int time_offset(time_t t, int *isdst)
{
    time_t gmt, rawtime = t;
    struct tm *ptm;

#if !defined(WIN32)
    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);
#else
    ptm = gmtime(&rawtime);
#endif
    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    if (isdst != NULL)
    {
        *isdst = ptm->tm_isdst;
    }

    return (int)difftime(rawtime, gmt);
}

static int isDigit(char *s, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (!isdigit((unsigned char)*(s + i)))
        {
            return 0;
        }
    }
    return 1;
}

static int atoi_len(char *s, int len)
{
    int value = 0;
    for (int i = 0; *(s + i) != '\0' && i < len; i++)
    {
        value = value * 10 + *(s + i) - '0';
    }
    return value;
}

static char *stndrdth(int number)
{
    static char *st = "st";
    static char *nd = "nd";
    static char *rd = "rd";
    static char *th = "th";
    int last = 0;
    if (number > 3 && number <= 20)
    {
        return th;
    }
    last = number % 10;
    if (last == 1)
    {
        return st;
    }
    else if (last == 2)
    {
        return nd;
    }
    else if (last == 3)
    {
        return rd;
    }
    return th;
}

static int iso_week_number(const struct tm *t)
{
    /* ISO 8601: week containing the first Thursday is week 1;
       Monday = first day of week.  Returns 1..53. */
    int yday = t->tm_yday + 1;                        /* 1-based */
    int wday = (t->tm_wday == 0) ? 7 : t->tm_wday;   /* Mon=1 Sun=7 */
    int w    = (yday - wday + 10) / 7;

    if (w < 1)
    {
        /* belongs to last week of previous year; Dec 28 always lies there */
        struct tm dec28 = {0};
        struct tm r;
        dec28.tm_year   = t->tm_year - 1;
        dec28.tm_mon    = 11;
        dec28.tm_mday   = 28;
        time_t t_dec28  = timegm(&dec28);
        gmtime_r(&t_dec28, &r);
        int py = r.tm_yday + 1;
        int pw = (r.tm_wday == 0) ? 7 : r.tm_wday;
        w      = (py - pw + 10) / 7;
    }
    else if (w >= 53)
    {
        /* week 53 valid only if Jan 1 of next year is Fri/Sat/Sun */
        struct tm jan1 = {0};
        struct tm r;
        jan1.tm_year   = t->tm_year + 1;
        jan1.tm_mon    = 0;
        jan1.tm_mday   = 1;
        time_t t_jan1  = timegm(&jan1);
        gmtime_r(&t_jan1, &r);
        int nwday = (r.tm_wday == 0) ? 7 : r.tm_wday;
        if (nwday <= 4)   /* Mon–Thu: that Jan 1 starts week 1 of next year */
            w = 1;
    }
    return w;
}

static int mallocStringBuffer(pMoment pmo, int size)
{
    char *newBuffer = NULL;

    if (pmo == NULL)
    {
        return 0;
    }

    if (size <= 0)
    {
        size = 1;
    }

    if (pmo->outputStr != NULL && pmo->outputSize >= size)
    {
        return 1;
    }

    newBuffer = (char *)malloc((size_t)size);
    if (newBuffer == NULL)
    {
        return 0;
    }

    if (pmo->outputStr != NULL)
    {
        free(pmo->outputStr);
    }

    pmo->outputStr = newBuffer;
    pmo->outputSize = size;
    pmo->outputStr[0] = '\0';
    return 1;
}

// Create
pMoment Moment_Now()
{
    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memset(pmo, 0, sizeof(Moment));
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        free(pmo);
        return NULL;
    }
    pmo->sec = tv.tv_sec;
    pmo->usec = tv.tv_usec;
    pmo->utcOffset = time_offset(tv.tv_sec, &pmo->isdst);
    return pmo;
}

pMoment Moment_Clone(pMoment pmo_)
{
    if (pmo_ == NULL)
    {
        return NULL;
    }

    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memcpy(pmo, pmo_, sizeof(Moment));
    pmo->outputStr = NULL;
    pmo->outputSize = 0;

    if (pmo_->outputStr != NULL && pmo_->outputSize > 0)
    {
        if (!mallocStringBuffer(pmo, pmo_->outputSize))
        {
            free(pmo);
            return NULL;
        }
        memcpy(pmo->outputStr, pmo_->outputStr, (size_t)pmo_->outputSize);
    }

    return pmo;
}

pMoment Moment_Second(time_t unixtime)
{
    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memset(pmo, 0, sizeof(Moment));
    pmo->sec = unixtime;
    pmo->utcOffset = time_offset(pmo->sec, &pmo->isdst);
    return pmo;
}

pMoment Moment_Millisecond(long int millisecond)
{
    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memset(pmo, 0, sizeof(Moment));
    pmo->sec = millisecond / 1000;
    pmo->usec = (millisecond % 1000) * 1000;
    pmo->utcOffset = time_offset(pmo->sec, &pmo->isdst);
    return pmo;
}

pMoment Moment_Set_Clone(pMoment pmo, pMoment pmo_)
{
    if (pmo == NULL || pmo_ == NULL)
    {
        return NULL;
    }

    if (pmo->outputStr != NULL)
    {
        free(pmo->outputStr);
    }

    memcpy(pmo, pmo_, sizeof(Moment));
    pmo->outputStr = NULL;
    pmo->outputSize = 0;

    if (pmo_->outputStr != NULL && pmo_->outputSize > 0)
    {
        if (!mallocStringBuffer(pmo, pmo_->outputSize))
        {
            return NULL;
        }
        memcpy(pmo->outputStr, pmo_->outputStr, (size_t)pmo_->outputSize);
    }

    return pmo;
}

pMoment Moment_Set_Now(pMoment pmo)
{
    if (pmo == NULL)
    {
        return NULL;
    }

    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        return NULL;
    }
    pmo->sec = tv.tv_sec;
    pmo->usec = tv.tv_usec;
    pmo->utcOffset = time_offset(tv.tv_sec, &pmo->isdst);
    return pmo;
}

pMoment Moment_Set_Second(pMoment pmo, time_t unixtime)
{
    if (pmo == NULL)
    {
        return NULL;
    }

    pmo->sec = unixtime;
    pmo->usec = 0;
    return pmo;
}

// Supported ISO 8601 strings , not valid return NULL;
pMoment Moment_Parse(char *string)
{
    if (string == NULL || *string == '\0')
    {
        return NULL;
    }

    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memset(pmo, 0, sizeof(Moment));

    int error = 0;
    int step = 0;
    int i = 0;
    int sign;
    int tzHour = 0, tzMin = 0;
    int week = 0, day = 0;

    //handle
    for (; *(string + i) != '\0'; i++)
    {
        // Set time zone and end parse
        if (*(string + i) == '+' || *(string + i) == '-')
        {
            sign = *(string + i) == '+' ? 1 : -1;
            i += 1;
            if (isDigit(string + i, 2))
            {
                tzHour = atoi_len(string + i, 2);
                i += *(string + i + 2) == ':' ? 4 : 3;
            }
            else
            {
                error = 1;
                break;
            }
            if (isDigit(string + i, 2))
            {
                tzMin = atoi_len(string + i, 2);
            }
            pmo->utcOffset = sign * (tzHour * 3600 + tzMin * 60);
        }
        else if (*(string + i) == 'Z' || *(string + i) == 'z')
        {
            break;
        }

        // year
        if (step == 0)
        {
            if (isDigit(string, 4))
            {

                pmo->timetm.tm_year = atoi_len(string, 4) - 1900;
                i += *(string + 4) == '-' ? 4 : 3;
                step = 1;
            }
            else
            {
                error = 1;
                break;
            }
        }

        // month or week
        else if (step == 1)
        {
            // A week date part
            if (*(string + i) == 'W')
            {
                i += 1;
                if (isDigit(string + i, 2))
                {
                    week = atoi_len(string + i, 2);
                    i += *(string + i + 2) == '-' ? 4 : 2;
                }
                else
                {
                    error = 1;
                    break;
                }
                if (isDigit(string + i, 1))
                {
                    day = atoi_len(string + i, 1);
                    i += 1;
                }
                pmo->sec = timegm(&pmo->timetm);
                pmo->sec += (week * 604800 + day * 86400);
                gmtime_r(&pmo->sec, &pmo->timetm);

                if (*(string + i) == ' ' || *(string + i) == 'T')
                {
                    step = 3;
                    // to hour
                }
                else
                {
                    break;
                }
            }
            // An ordinal date part
            else if (!isDigit(string + i, 4) && isDigit(string + i, 3))
            {
                pmo->sec = timegm(&pmo->timetm);
                pmo->sec += ((atoi_len(string + i, 3)) * 86400);
                gmtime_r(&pmo->sec, &pmo->timetm);
                if (*(string + i + 3) == ' ' || *(string + i + 3) == 'T')
                {
                    i += 3;
                    step = 3;
                    // to hour
                }
                else
                {
                    break;
                }
            }
            // A Month
            else
            {
                if (isDigit(string + i, 2))
                {
                    pmo->timetm.tm_mon = atoi_len(string + i, 2) - 1;
                    i += *(string + i + 2) == '-' ? 2 : 1;
                    step = 2;
                }
                else
                {
                    error = 1;
                    break;
                }
            }
        }

        // day
        else if (step == 2)
        {
            if (isDigit(string + i, 2))
            {
                pmo->timetm.tm_mday = atoi_len(string + i, 2);
                if (*(string + i + 2) == ' ' || *(string + i + 2) == 'T')
                {
                    i += 2;
                    step = 3;
                    // to hour
                }
                else
                {
                    break;
                }
            }
            else
            {
                error = 1;
                break;
            }
        }

        // Hour
        else if (step == 3)
        {
            if (isDigit(string + i, 2))
            {
                pmo->timetm.tm_hour = atoi_len(string + i, 2);
                i += *(string + i + 2) == ':' ? 2 : 1;
                step = 4;
            }
            else
            {
                error = 1;
                break;
            }
        }

        // Minute
        else if (step == 4)
        {
            if (isDigit(string + i, 2))
            {
                pmo->timetm.tm_min = atoi_len(string + i, 2);
                i += *(string + i + 2) == ':' ? 2 : 1;
                step = 5;
            }
            else
            {
                error = 1;
                break;
            }
        }

        // Second
        else if (step == 5)
        {
            if (isDigit(string + i, 2))
            {
                pmo->timetm.tm_sec = atoi_len(string + i, 2);
                if (*(string + i + 2) == '.')
                {
                    step = 6;
                }
                else
                {
                    step = 7;
                }
                i += *(string + i + 2) == '.' || *(string + i + 2) == ',' ? 2 : 1;
            }
            else
            {
                error = 1;
                break;
            }
        }

        // ms
        else if (step == 6)
        {
            if (isDigit(string + i, 3))
            {
                pmo->usec = atoi_len(string + i, 3) * 1000;
                i += 2;
                step = 7;
            }
            else
            {
                error = 1;
                break;
            }
        }

        else
        {
            break;
        }
    }

    if (error)
    {
        free(pmo);
        return NULL;
    }

    pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
    return pmo;
}

// Get
time_t Moment_Get_Sec(pMoment pmo)
{
    if (pmo == NULL)
    {
        return 0;
    }

    return pmo->sec;
}

time_t Moment_Get_Millisecond(pMoment pmo)
{
    if (pmo == NULL)
    {
        return 0;
    }

    return pmo->sec * 1000 + pmo->usec / 1000;
}

// return the max one
pMoment Moment_Max(pMoment pmo, ...)
{
    va_list ap;
    pMoment i = NULL;
    pMoment MaxOne = NULL;
    va_start(ap, pmo);
    for (i = pmo; i != NULL; i = va_arg(ap, pMoment))
    {
        if (MaxOne == NULL)
        {
            MaxOne = i;
        }
        else if (MaxOne->sec < i->sec)
        {
            MaxOne = i;
        }
        else if (MaxOne->sec == i->sec && MaxOne->usec < i->usec)
        {
            MaxOne = i;
        }
    }
    va_end(ap);
    return MaxOne;
}

// return the min one
pMoment Moment_Min(pMoment pmo, ...)
{
    va_list ap;
    pMoment i = NULL;
    pMoment MinOne = NULL;
    va_start(ap, pmo);
    for (i = pmo; i != NULL; i = va_arg(ap, pMoment))
    {
        if (MinOne == NULL)
        {
            MinOne = i;
        }
        else if (MinOne->sec > i->sec)
        {
            MinOne = i;
        }
        else if (MinOne->sec == i->sec && MinOne->usec > i->usec)
        {
            MinOne = i;
        }
    }
    va_end(ap);
    return MinOne;
}

// return maked str length
size_t head_patten_to_str(
    pMoment pmo,
    char *out,
    int outSize,
    const char *format,
    int *handledLen)
{
    size_t outlen = 0;

    //year
    if (strncmp(format, "YYYY", 4) == 0)
    {
        outlen = snprintf(out, outSize, "%04d", pmo->timetm.tm_year + 1900);
        *handledLen += 4;
        return outlen;
    }
    else if (strncmp(format, "YY", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", pmo->timetm.tm_year % 100);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "Y", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_year + 1900);
        *handledLen += 1;
        return outlen;
    }

    // Month
    else if (strncmp(format, "MMMM", 4) == 0)
    {
        outlen = strftime(out, outSize, "%B", &pmo->timetm);
        *handledLen += 4;
        return outlen;
    }
    else if (strncmp(format, "MMM", 3) == 0)
    {
        outlen = strftime(out, outSize, "%b", &pmo->timetm);
        *handledLen += 3;
        return outlen;
    }
    else if (strncmp(format, "MM", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", pmo->timetm.tm_mon + 1);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "Mo", 2) == 0)
    {
        if (pmo->timetm.tm_mon == 0)
        {
            outlen = snprintf(out, outSize, "1st");
        }
        else if (pmo->timetm.tm_mon == 1)
        {

            outlen = snprintf(out, outSize, "2nd");
        }
        else if (pmo->timetm.tm_mon == 2)
        {
            outlen = snprintf(out, outSize, "3rd");
        }
        else
        {
            outlen = snprintf(out, outSize, "%dth", pmo->timetm.tm_mon + 1);
        }
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "M", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_mon + 1);
        *handledLen += 1;
        return outlen;
    }

    // Day
    else if (strncmp(format, "DDDD", 4) == 0)
    {
        outlen = snprintf(out, outSize, "%03d", pmo->timetm.tm_yday);
        *handledLen += 4;
        return outlen;
    }
    else if (strncmp(format, "DDDo", 4) == 0)
    {
        outlen = snprintf(out, outSize, "%d%s", pmo->timetm.tm_yday + 1, stndrdth(pmo->timetm.tm_yday + 1));
        *handledLen += 4;
        return outlen;
    }
    else if (strncmp(format, "DDD", 3) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_yday);
        *handledLen += 3;
        return outlen;
    }
    else if (strncmp(format, "DD", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", pmo->timetm.tm_mday);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "Do", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%d%s", pmo->timetm.tm_mday, stndrdth(pmo->timetm.tm_mday));
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "D", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_mday);
        *handledLen += 1;
        return outlen;
    }

    // Day of Week
    else if (strncmp(format, "dddd", 4) == 0)
    {
        outlen = strftime(out, outSize, "%A", &pmo->timetm);
        *handledLen += 4;
        return outlen;
    }
    else if (strncmp(format, "ddd", 3) == 0)
    {
        outlen = strftime(out, outSize, "%a", &pmo->timetm);
        *handledLen += 3;
        return outlen;
    }
    else if (strncmp(format, "dd", 2) == 0)
    {
        outlen = strftime(out, outSize, "%a", &pmo->timetm);
        out[outlen - 1] = '\0';
        *handledLen += 2;
        return outlen - 1;
    }
    else if (strncmp(format, "do", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%d%s", pmo->timetm.tm_wday, stndrdth(pmo->timetm.tm_wday));
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "d", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_wday);
        *handledLen += 1;
        return outlen;
    }

    // Day of Week (Locale)
    else if (strncmp(format, "e", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_wday);
        *handledLen += 1;
        return outlen;
    }

    // Day of Week (ISO)
    else if (strncmp(format, "E", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_wday + 1);
        *handledLen += 1;
        return outlen;
    }

    // Week of Year (locale, Sunday-first, 1-based)
    else if (strncmp(format, "ww", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", 1 + pmo->timetm.tm_yday / 7);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "wo", 2) == 0)
    {
        int wy = 1 + pmo->timetm.tm_yday / 7;
        outlen = snprintf(out, outSize, "%d%s", wy, stndrdth(wy));
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "w", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", 1 + pmo->timetm.tm_yday / 7);
        *handledLen += 1;
        return outlen;
    }

    // Week of Year (ISO 8601, Monday-first)
    else if (strncmp(format, "WW", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", iso_week_number(&pmo->timetm));
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "Wo", 2) == 0)
    {
        int iw = iso_week_number(&pmo->timetm);
        outlen = snprintf(out, outSize, "%d%s", iw, stndrdth(iw));
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "W", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", iso_week_number(&pmo->timetm));
        *handledLen += 1;
        return outlen;
    }

    // AM/PM
    else if (strncmp(format, "A", 1) == 0)
    {
        outlen = snprintf(out, outSize, pmo->timetm.tm_hour < 12 ? "AM" : "PM");
        *handledLen += 1;
        return outlen;
    }
    else if (strncmp(format, "a", 1) == 0)
    {
        outlen = snprintf(out, outSize, pmo->timetm.tm_hour < 12 ? "am" : "pm");
        *handledLen += 1;
        return outlen;
    }

    // Hour
    else if (strncmp(format, "HH", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", pmo->timetm.tm_hour);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "H", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_hour);
        *handledLen += 1;
        return outlen;
    }
    else if (strncmp(format, "hh", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", ((pmo->timetm.tm_hour - 1) % 12) + 1);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "h", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", ((pmo->timetm.tm_hour - 1) % 12) + 1);
        *handledLen += 1;
        return outlen;
    }
    else if (strncmp(format, "kk", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", pmo->timetm.tm_hour + 1);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "k", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_hour + 1);
        *handledLen += 1;
        return outlen;
    }

    // Mintue
    else if (strncmp(format, "mm", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", pmo->timetm.tm_min);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "m", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_min);
        *handledLen += 1;
        return outlen;
    }

    // Second
    else if (strncmp(format, "ss", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", pmo->timetm.tm_sec);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "s", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%d", pmo->timetm.tm_sec);
        *handledLen += 1;
        return outlen;
    }

    // Fractional Second
    else if (strncmp(format, "SSS", 3) == 0)
    {
        outlen = snprintf(out, outSize, "%03ld", pmo->usec / 1000);
        *handledLen += 3;
        return outlen;
    }
    else if (strncmp(format, "SS", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02ld", pmo->usec / 10000);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "S", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%01ld", pmo->usec / 100000);
        *handledLen += 1;
        return outlen;
    }

    // Time Zone
    else if (strncmp(format, "ZZ", 2) == 0)
    {
        int sign = pmo->utcOffset < 0 ? -1 : 1;
        int hour = sign * pmo->utcOffset / 3600;
        int min = sign * pmo->utcOffset % 3600 / 60;
        outlen = snprintf(out, outSize, "%+03d%02d", sign * hour, min);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "Z", 1) == 0)
    {
        int sign = pmo->utcOffset < 0 ? -1 : 1;
        int hour = sign * pmo->utcOffset / 3600;
        int min = sign * pmo->utcOffset % 3600 / 60;
        outlen = snprintf(out, outSize, "%+03d:%02d", sign * hour, min);
        *handledLen += 1;
        return outlen;
    }

    // Unix Timestamp
    else if (strncmp(format, "X", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%ld", pmo->sec);
        *handledLen += 1;
        return outlen;
    }

    //Unix Millisecond Timestamp
    else if (strncmp(format, "x", 1) == 0)
    {
        outlen = snprintf(out, outSize, "%ld", pmo->sec * 1000 + pmo->usec / 1000);
        *handledLen += 1;
        return outlen;
    }

    // Pure text
    else if (strncmp(format, "[", 1) == 0)
    {
        int i = 1;
        int j = outSize;
        while (*(format + i) != '\0' && *(format + i) != ']')
        {
            if (j > 0)
            {
                *(out + i - 1) = *(format + i);
                outlen++;
                j--;
                i++;
            }
            else
            {
                i++;
                break;
            }
        }
        *handledLen += (i + 1);
        return outlen;
    }

    *out = *format;
    *handledLen += 1;
    return 1;
}

// strftime
char *Moment_strftime(pMoment pmo, char *format)
{
    static char *nan = "Invalid date";

    if (pmo == NULL)
    {
        return nan;
    }

    if (format == NULL)
    {
        format = "%c";
    }

    if (!mallocStringBuffer(pmo, (int)strlen(format) * 20 + 1))
    {
        return nan;
    }

    //renew tm
    time_t tztime = pmo->sec + pmo->utcOffset;
    gmtime_r(&tztime, &pmo->timetm);
    strftime(pmo->outputStr, pmo->outputSize, format, &pmo->timetm);
    return pmo->outputStr;
}

// Format
char *Moment_Format(pMoment pmo, char *format)
{
    static char *nan = "Invalid date";
    // "2014-09-08T08:02:17-05:00" (ISO 8601, no fractional seconds)
    static const char *defaultFormat = "YYYY-MM-DDTHH:mm:ssZ";
    const char *workFormat = NULL;
    time_t tztime = 0;
    int formatLen = 0;
    int handledLen = 0;
    int outPutLen = 0;
    //

    if (pmo == NULL)
    {
        return nan;
    }

    if (format == NULL)
    {
        workFormat = defaultFormat;
        formatLen = strlen(workFormat);
    }
    else
    {
        workFormat = format;
        formatLen = strlen(workFormat);
        if (formatLen == 0)
        {
            workFormat = defaultFormat;
            formatLen = strlen(workFormat);
        }
    }
    if (!mallocStringBuffer(pmo, formatLen * 16 + 32))
    {
        return nan;
    }

    //renew tm
    tztime = pmo->sec + pmo->utcOffset;
    gmtime_r(&tztime, &pmo->timetm);

    while (handledLen < formatLen)
    {
        outPutLen += head_patten_to_str(
            pmo,
            pmo->outputStr + outPutLen,
            pmo->outputSize - outPutLen,
            workFormat + handledLen,
            &handledLen);
    }

    return pmo->outputStr;
}

// TimeZone
pMoment Moment_Set_utcOffset(pMoment pmo, int utcOffset)
{
    if (pmo == NULL)
    {
        return NULL;
    }

    pmo->utcOffset = utcOffset;
    return pmo;
}

int Moment_Get_utcOffset(pMoment pmo)
{
    if (pmo == NULL)
    {
        return 0;
    }

    return pmo->utcOffset;
}

// Add and Subtract
pMoment Moment_Add(pMoment pmo, long int number, char *string)
{
    time_t tztime;

    if (pmo == NULL || string == NULL)
    {
        return NULL;
    }

    if (strcmp(string, "years") == 0 || strcmp(string, "y") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_year += number;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
    }
    else if (strcmp(string, "months") == 0 || strcmp(string, "M") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_mon += number;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
    }
    else if (strcmp(string, "weeks") == 0 || strcmp(string, "w") == 0)
    {
        pmo->sec += number * 604800;
    }
    else if (strcmp(string, "days") == 0 || strcmp(string, "d") == 0)
    {
        pmo->sec += number * 86400;
    }
    else if (strcmp(string, "hours") == 0 || strcmp(string, "h") == 0)
    {
        pmo->sec += number * 3600;
    }
    else if (strcmp(string, "minutes") == 0 || strcmp(string, "m") == 0)
    {
        pmo->sec += number * 60;
    }
    else if (strcmp(string, "seconds") == 0 || strcmp(string, "s") == 0)
    {
        pmo->sec += number;
    }
    else if (strcmp(string, "milliseconds") == 0 || strcmp(string, "ms") == 0)
    {
        pmo->usec += number * 1000;
        pmo->sec += (pmo->usec / 1000000);
        pmo->usec = pmo->usec % 1000000;
        if (pmo->usec < 0)
        {
            pmo->usec += 1000000;
            pmo->sec -= 1;
        }
    }
    return pmo;
}

pMoment Moment_Subtract(pMoment pmo, long int number, char *string)
{
    return Moment_Add(pmo, -number, string);
}

// Start of Time
pMoment Moment_StartOf(pMoment pmo, char *string)
{
    time_t tztime;

    if (pmo == NULL || string == NULL)
    {
        return NULL;
    }

    if (strcmp(string, "year") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_mon = 0;
        pmo->timetm.tm_mday = 1;
        pmo->timetm.tm_hour = 0;
        pmo->timetm.tm_min = 0;
        pmo->timetm.tm_sec = 0;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
        pmo->usec = 0;
    }
    else if (strcmp(string, "month") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_mday = 1;
        pmo->timetm.tm_hour = 0;
        pmo->timetm.tm_min = 0;
        pmo->timetm.tm_sec = 0;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
        pmo->usec = 0;
    }
    else if (strcmp(string, "week") == 0 || strcmp(string, "isoWeek") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_mday -= pmo->timetm.tm_wday;
        pmo->timetm.tm_hour = 0;
        pmo->timetm.tm_min = 0;
        pmo->timetm.tm_sec = 0;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
        pmo->usec = 0;
    }
    else if (strcmp(string, "day") == 0 || strcmp(string, "date") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_hour = 0;
        pmo->timetm.tm_min = 0;
        pmo->timetm.tm_sec = 0;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
        pmo->usec = 0;
    }
    else if (strcmp(string, "hour") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_min = 0;
        pmo->timetm.tm_sec = 0;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
        pmo->usec = 0;
    }
    else if (strcmp(string, "minute") == 0)
    {
        tztime = pmo->sec + pmo->utcOffset;
        gmtime_r(&tztime, &pmo->timetm);
        pmo->timetm.tm_sec = 0;
        pmo->sec = timegm(&pmo->timetm) - pmo->utcOffset;
        pmo->usec = 0;
    }
    else if (strcmp(string, "second") == 0)
    {
        pmo->usec = 0;
    }
    return pmo;
}

// End of Time
pMoment Moment_EndOf(pMoment pmo, char *string)
{
    if (pmo == NULL || string == NULL)
    {
        return NULL;
    }

    if (strcmp(string, "year") == 0)
    {
        Moment_StartOf(pmo, "year");
        Moment_Add(pmo, 1, "years");
        pmo->sec -= 1;
        pmo->usec = 999999;
    }
    else if (strcmp(string, "month") == 0)
    {
        Moment_StartOf(pmo, "month");
        Moment_Add(pmo, 1, "months");
        pmo->sec -= 1;
        pmo->usec = 999999;
    }
    else if (strcmp(string, "week") == 0 || strcmp(string, "isoWeek") == 0)
    {
        Moment_StartOf(pmo, "week");
        Moment_Add(pmo, 1, "weeks");
        pmo->sec -= 1;
        pmo->usec = 999999;
    }
    else if (strcmp(string, "day") == 0 || strcmp(string, "date") == 0)
    {
        Moment_StartOf(pmo, "day");
        Moment_Add(pmo, 1, "days");
        pmo->sec -= 1;
        pmo->usec = 999999;
    }
    else if (strcmp(string, "hour") == 0)
    {
        Moment_StartOf(pmo, "hour");
        Moment_Add(pmo, 1, "hours");
        pmo->sec -= 1;
        pmo->usec = 999999;
    }
    else if (strcmp(string, "minute") == 0)
    {
        Moment_StartOf(pmo, "minute");
        Moment_Add(pmo, 1, "minutes");
        pmo->sec -= 1;
        pmo->usec = 999999;
    }
    else if (strcmp(string, "second") == 0)
    {
        pmo->usec = 999999;
    }
    return pmo;
}

// clear
void Moment_Clear(pMoment pmo)
{
    if (pmo == NULL)
    {
        return;
    }

    if (pmo->outputStr != NULL)
    {
        free(pmo->outputStr);
    }
    free(pmo);
}

int Moment_snprintf(char *s, size_t n, char *format, time_t sec)
{
    int len = 0;
    pMoment pmo = NULL;

    if (s == NULL || n == 0)
    {
        return 0;
    }

    pmo = Moment_Second(sec);
    if (pmo == NULL)
    {
        s[0] = '\0';
        return 0;
    }

    len = snprintf(s, n, "%s", Moment_Format(pmo, format));
    Moment_Clear(pmo);
    return len;
}

// Diff
long int Moment_Diff(pMoment a, pMoment b, char *unit)
{
    time_t    tztime_a, tztime_b;
    struct tm tm_a, tm_b;

    if (a == NULL || b == NULL || unit == NULL)
    {
        return 0;
    }

    if (strcmp(unit, "years") == 0 || strcmp(unit, "y") == 0)
    {
        tztime_a        = a->sec + a->utcOffset;
        tztime_b        = b->sec + b->utcOffset;
        gmtime_r(&tztime_a, &tm_a);
        gmtime_r(&tztime_b, &tm_b);
        long int years  = (long int)(tm_a.tm_year - tm_b.tm_year);
        struct tm check = tm_b;
        check.tm_year   = tm_a.tm_year;
        time_t check_t  = timegm(&check);
        if (years > 0 && tztime_a < check_t)
            years--;
        else if (years < 0 && tztime_a > check_t)
            years++;
        return years;
    }
    else if (strcmp(unit, "months") == 0 || strcmp(unit, "M") == 0)
    {
        tztime_a          = a->sec + a->utcOffset;
        tztime_b          = b->sec + b->utcOffset;
        gmtime_r(&tztime_a, &tm_a);
        gmtime_r(&tztime_b, &tm_b);
        long int months   = (long int)(tm_a.tm_year - tm_b.tm_year) * 12
                          + (long int)(tm_a.tm_mon  - tm_b.tm_mon);
        struct tm check   = tm_b;
        check.tm_year     = tm_a.tm_year;
        check.tm_mon      = tm_a.tm_mon;
        time_t check_t    = timegm(&check);
        if (months > 0 && tztime_a < check_t)
            months--;
        else if (months < 0 && tztime_a > check_t)
            months++;
        return months;
    }
    else if (strcmp(unit, "weeks") == 0 || strcmp(unit, "w") == 0)
    {
        return (a->sec - b->sec) / 604800;
    }
    else if (strcmp(unit, "days") == 0 || strcmp(unit, "d") == 0)
    {
        return (a->sec - b->sec) / 86400;
    }
    else if (strcmp(unit, "hours") == 0 || strcmp(unit, "h") == 0)
    {
        return (a->sec - b->sec) / 3600;
    }
    else if (strcmp(unit, "minutes") == 0 || strcmp(unit, "m") == 0)
    {
        return (a->sec - b->sec) / 60;
    }
    else if (strcmp(unit, "seconds") == 0 || strcmp(unit, "s") == 0)
    {
        return a->sec - b->sec;
    }
    else if (strcmp(unit, "milliseconds") == 0 || strcmp(unit, "ms") == 0)
    {
        return (a->sec - b->sec) * 1000L
             + (a->usec - b->usec) / 1000L;
    }
    return 0;
}

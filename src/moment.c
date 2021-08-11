#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>

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
        if (!isdigit(*(s + i)))
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

static char * stndrdth(int number)
{
    static char * st = "st";
    static char * nd = "nd";
    static char * rd = "rd";
    static char * th = "th";
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

// Create
pMoment Moment_Now()
{
    int ret = 0;
    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memset(pmo, 0, sizeof(Moment));
    struct timeval tv;
    ret = gettimeofday(&tv, NULL);
    pmo->sec = tv.tv_sec;
    pmo->usec = tv.tv_usec;
    pmo->utcOffset = time_offset(tv.tv_sec, &pmo->isdst);
    return pmo;
}

pMoment Moment_Clone(pMoment pmo_)
{
    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memcpy(pmo, pmo_, sizeof(Moment));
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

pMoment Moment_Millisecond(
    long int millisecond)
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

pMoment Moment_Set_Clone(
    pMoment pmo,
    pMoment pmo_)
{
    memcpy(pmo, pmo_, sizeof(Moment));
    return pmo;
}

pMoment Moment_Set_Now(
    pMoment pmo)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pmo->sec = tv.tv_sec;
    pmo->usec = tv.tv_usec;
    pmo->utcOffset = time_offset(tv.tv_sec, &pmo->isdst);
    return pmo;
}

// Supported ISO 8601 strings , not valid return NULL;
pMoment Moment_Parse(
    char *string)
{
    pMoment pmo = (pMoment)malloc(sizeof(Moment));
    if (pmo == NULL)
    {
        return NULL;
    }
    memset(pmo, 0, sizeof(Moment));

    int error = 0;
    int handled = 0;
    int step = 0;
    int value = 0;
    int i = 0, j = 0;
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
                i += *(string + 2) == ':' ? 4 : 3;
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
                    i += *(string + 2) == '-' ? 4 : 2;
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

                i += *(string + i + 2) == '.' || *(string + i + 2) == ',' ? 2 : 1;
                step = 6;
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

//
time_t Moment_Get_Sec(pMoment pmo)
{
    return pmo->sec;
}

time_t Moment_Get_Millisecond(pMoment pmo)
{
    return pmo->sec * 1000 + pmo->usec / 1000;
}

// return maked str length
size_t head_patten_to_str(
    pMoment pmo,
    char *out,
    int outSize,
    char *format,
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
    else if (strncmp(format, "Y", 2) == 0)
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

    // Week of Year	, Week of Year (ISO)
    else if (strncmp(format, "ww", 2) == 0 || strncmp(format, "WW", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%02d", 1 + pmo->timetm.tm_yday / 7);
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "wo", 2) == 0 || strncmp(format, "Wo", 2) == 0)
    {
        int wy = 1 + pmo->timetm.tm_yday / 7;
        outlen = snprintf(out, outSize, "%d%s", wy, stndrdth(wy));
        *handledLen += 2;
        return outlen;
    }
    else if (strncmp(format, "w", 2) == 0 || strncmp(format, "W", 2) == 0)
    {
        outlen = snprintf(out, outSize, "%d", 1 + pmo->timetm.tm_yday / 7);
        *handledLen += 2;
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
    else if (strncmp(format, "mm", 1) == 0)
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

// Format
char *Moment_Format(
    pMoment pmo,
    char *format)
{
    static char *nan = "Invalid date";
    // "2014-09-08T08:02:17-05:00" (ISO 8601, no fractional seconds)
    static char *defaultFormat = "YYYY-MM-DDTHH:mm:ssZ";
    char *workFormat = NULL;
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

    if (pmo->outputSize < formatLen * 3)
    {
        free(pmo->outputStr);
        pmo->outputSize = 0;
    }

    if (pmo->outputSize == 0)
    {
        pmo->outputSize = formatLen * 3;
        pmo->outputStr = (char *)malloc(pmo->outputSize);
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
pMoment Moment_Set_utcOffset(
    pMoment pmo,
    int utcOffset)
{
    pmo->utcOffset = utcOffset;
    return pmo;
}

int Moment_Get_utcOffset(
    pMoment pmo)
{
    return pmo->utcOffset;
}

// Add and Subtract
pMoment Moment_Add(
    pMoment pmo,
    long int number,
    char *string)
{
    time_t tztime;

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
        pmo->usec += number;
        pmo->sec += (pmo->usec / 1000000);
        pmo->usec = pmo->usec % 1000000;
    }
    return pmo;
}

pMoment Moment_Subtract(
    pMoment pmo,
    long int number,
    char *string)
{
    return Moment_Add(pmo, -number, string);
}

// Start of Time
pMoment Moment_StartOf(
    pMoment pmo,
    char *string)
{
    time_t tztime;
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
pMoment Moment_EndOf(
    pMoment pmo,
    char *string)
{
    time_t tztime;
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
void Moment_Clear(
    pMoment pmo)
{
    if (pmo->outputStr != NULL)
    {
        free(pmo->outputStr);
    }
    free(pmo);
}
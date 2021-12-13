#ifndef _MOMENT_H
#define _MOMENT_H

#include <time.h>

#define VERSION "0.0.0.1"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct Moment_
    {
        long int sec;  /* seconds part */
        long int usec; /* microseconds part */
        struct tm timetm;
        int utcOffset;
        int isdst;
        char *outputStr;
        int outputSize;
    } Moment, *pMoment;

    // Create
    pMoment Moment_Now();
    pMoment Moment_Clone(pMoment pmo_);
    pMoment Moment_Second(time_t unixtime);
    pMoment Moment_Millisecond(long int millisecond);
    pMoment Moment_Parse(char *string);

    // Set
    pMoment Moment_Set_Clone(pMoment pmo, pMoment pmo_);
    pMoment Moment_Set_utcOffset(pMoment pmo, int utcOffset);

    // Get
    time_t Moment_Get_Sec(pMoment moment);
    time_t Moment_Get_Millisecond(pMoment pmo);

    // Max
    pMoment Moment_Max(pMoment pmo, ...);

    // Min
    pMoment Moment_Min(pMoment pmo, ...);

    // Display
    char *Moment_strftime(pMoment pmo, char *format);
    char *Moment_Format(pMoment pmo, char *format);

    // Calculate
    pMoment Moment_Add(pMoment pmo, long int number, char *string);
    pMoment Moment_Subtract(pMoment pmo, long int number, char *string);

    pMoment Moment_StartOf(pMoment pmo, char *string);
    pMoment Moment_EndOf(pMoment pmo, char *string);

    // clear
    void Moment_Clear(pMoment moment);

#ifdef __cplusplus
};
#endif
#endif

# moment
Parse, validate, manipulate, and display dates in c.
***
# Create moment object
## pMoment Moment_Now();
pMoment pmo = Moment_Now();  //Now Time
## pMoment Moment_Second(time_t unixtime);
pMoment pmo = Moment_Second(1628578601);
## pMoment Moment_Millisecond(long int millisecond);
pMoment pmo = Moment_Millisecond(1628578601000);
## pMoment Moment_Clone(pMoment pmo_);
pMoment pmo = Moment_Clone(pmo_);
## pMoment Moment_Parse(char *string);
Supported ISO 8601 strings

An ISO 8601 string requires a date part.
| Example | Description |
| ---  | --- |
| 2013-02-08 | A calendar date part |
| 2013-02 | A month date part |
| 2013-W06-5 | A week date part |
| 2013-039 | An ordinal date part |
| 20130208 | Basic (short) full date |
| 201303 | Basic (short) year+month |
| 2013 | Basic (short) year only |
| 2013W065 | Basic (short) week, weekday |
| 2013W06 | Basic (short) week only |
| 2013050 | Basic (short) ordinal date (year + day-of-year) |
| 2013-02-08T09 | An hour time part separated by a T |
| 2013-02-08 09 | An hour time part separated by a space |
| 2013-02-08 09:30 | An hour and minute time part |
| 2013-02-08 09:30:26 | An hour, minute, and second time part |
| 2013-02-08 09:30:26.123 | An hour, minute, second, and millisecond time part |
| 2013-02-08 24:00:00.000 | hour 24, minute, second, millisecond equal 0 means next day at midnight |
| 20130208T080910,123 | Short date and time up to ms, separated by comma |
| 20130208T080910.123 | Short date and time up to ms |
| 20130208T080910 | Short date and time up to seconds |
| 20130208T0809 | Short date and time up to minutes |
| 20130208T08 | Short date and time, hours only |
***
# Clear moment object
## void Moment_Clear(pMoment pmo);
Moment_Clear(pmo);  //clear moment object
***
# Set moment object
## pMoment Moment_Set_Clone(pMoment pmo, pMoment pmo_);
Moment_Set_Clone(pmo, pmo_); //return origin pmo object point;
## pMoment Moment_Set_utcOffset(pMoment pmo, int utcOffset);
Moment_Set_utcOffset(pmo, 28800);  //set +8 timezone and return origin pmo;
***
# Get moment object value
## time_t Moment_Get_Sec(pMoment moment);
## time_t Moment_Get_Millisecond(pMoment pmo);
***
# Format moment object and return formated string
## char *Moment_Format(pMoment pmo, char *format);
Moment_Format(pmo, NULL); // "2014-09-08T08:02:17-05:00" (ISO 8601)

Moment_Format(pmo, "dddd, MMMM Do YYYY, h:mm:ss a");  // "Sunday, February 14th 2010, 3:25:50 pm"

Moment_Format(pmo, "ddd, hA"); // "Sun, 3PM"

|  | Token | Output |
| ---  | --- | --- |
| Month | M | 1 2 ... 11 12 |
| | Mo | 1st 2nd ... 11th 12th |
| | MM | 01 02 ... 11 12 |
| | MMM | Jan Feb ... Nov Dec |
| | MMMM | January February ... November December |
| Day of Month | D | 1 2 ... 30 31 |
| | Do | 1st 2nd ... 30th 31st |
| | DD | 01 02 ... 30 31 |
| Day of Year | DDD | 1 2 ... 364 365 |
| | DDDo | 1st 2nd ... 364th 365th |
| | DDDD | 001 002 ... 364 365 |
| Day of Week | d | 0 1 ... 5 6 |
| | do | 0th 1st ... 5th 6th |
| | dd | Su Mo ... Fr Sa |
| | ddd | Sun Mon ... Fri Sat |
| | dddd | Sunday Monday ... Friday Saturday |
| Day of Week (Locale) | e | 0 1 ... 5 6 |
| Day of Week (ISO) | E | 1 2 ... 6 7 |
| Week of Year | w | 1 2 ... 52 53 |
| | wo | 1st 2nd ... 52nd 53rd |
| | ww | 01 02 ... 52 53 |
| Week of Year (ISO) | W | 1 2 ... 52 53 |
| | Wo | 1st 2nd ... 52nd 53rd |
| | WW | 01 02 ... 52 53 |
| Year | YY | 70 71 ... 29 30 |
| | YYYY | 1970 1971 ... 2029 2030 |
| AM/PM | A | AM PM |
| | a | am pm |
| Hour | H | 0 1 ... 22 23 |
| | HH | 00 01 ... 22 23 |
| | h | 1 2 ... 11 12 |
| | hh | 01 02 ... 11 12 |
| | k |	1 2 ... 23 24 |
| | kk | 01 02 ... 23 24 | 
| Minute | m | 0 1 ... 58 59 |
| | mm | 00 01 ... 58 59 |
| Second | s | 0 1 ... 58 59 |
| | ss | 00 01 ... 58 59 |
| Fractional Second | S | 0 1 ... 8 9 |
| | SS | 00 01 ... 98 99 |
| | SSS | 000 001 ... 998 999 |
| Time Zone | Z | -07:00 -06:00 ... +06:00 +07:00 |
| | ZZ | -0700 -0600 ... +0600 +0700 |
| Unix Timestamp | X |1360013296 |
| Unix Millisecond Timestamp | x | 1360013296123 |
***
# Add / Subtract moment object
## pMoment Moment_Add(pMoment pmo, long int number, char *string);
Moment_Add(pmo, 7, "days");  //return origin pmo object point;
## pMoment Moment_Subtract(pMoment pmo, long int number, char *string);
Moment_Subtract(pmo, 7, "days");  //return origin pmo object point;

| Key | Shorthand |
| --- | --- |
| years | y |
| months | M |
| weeks | w |
| days | d |
| hours | h |
| minutes | m |
| seconds | s |
| milliseconds | ms |
***
# Start of Time / End of Time
## pMoment Moment_StartOf(pMoment pmo, char *string);
Moment_StartOf(pmo, "year");  //return origin pmo object point;

| Key | Memo |
| --- | --- |
| year | set to January 1st, 12:00 am this year |
| month | set to the first of this month, 12:00 am |
| week | set to the first day of this week, 12:00 am |
| day | set to 12:00 am today |
| date | set to 12:00 am today |
| hour | set to now, but with 0 mins, 0 secs, and 0 ms |
| minute | set to now, but with 0 seconds and 0 milliseconds |
| second | set to 0 milliseconds |

## pMoment Moment_EndOf(pMoment pmo, char *string);
Moment_EndOf(pmo, "year");  //return origin pmo object point;
***
# moment

一個以 C 語言實作的時間處理函式庫，提供：

- 解析日期字串（ISO 8601 子集合）
- 時間加減、取區間起訖
- 格式化輸出（moment token + strftime）
- 比較最大/最小時間

---

## 1. 專案結構

```text
src/moment.h    公開 API
src/moment.c    實作
tests/test_moment.c  測試程式
```

---

## 2. 快速開始

### 2.1 編譯（Linux / macOS / MinGW）

```bash
gcc -Wall -Wextra -std=c11 src/moment.c your_app.c -o your_app
```

### 2.2 最小範例

```c
#include <stdio.h>
#include "src/moment.h"

int main(void)
{
	pMoment m = Moment_Now();
	if (m == NULL)
	{
		return 1;
	}

	printf("now = %s\n", Moment_Format(m, "YYYY-MM-DD HH:mm:ss.SSS Z"));
	Moment_Clear(m);
	return 0;
}
```

---

## 3. 記憶體與回傳值規則

### 3.1 物件生命週期

- `Moment_Now` / `Moment_Second` / `Moment_Millisecond` / `Moment_Parse` / `Moment_Clone` 會配置新物件。
- 使用完必須呼叫 `Moment_Clear` 釋放。
- `Moment_Clear(NULL)` 可安全呼叫。

### 3.2 字串輸出生命週期

- `Moment_Format`、`Moment_strftime` 回傳的是物件內部緩衝區指標。
- 下一次對同一個 `pMoment` 再次格式化，會覆蓋舊內容。
- 物件被 `Moment_Clear` 後，該字串指標失效。

### 3.3 錯誤處理

- 物件建立失敗時會回傳 `NULL`。
- `Moment_Format(NULL, ...)`、`Moment_strftime(NULL, ...)` 會回傳字串 `"Invalid date"`。
- 多數 API 對 `NULL` 輸入做了保護（回傳 `NULL` 或 `0`）。

---

## 4. API 使用說明

### 4.1 建立與複製

```c
pMoment Moment_Now(void);
pMoment Moment_Second(time_t unixtime);
pMoment Moment_Millisecond(long int millisecond);
pMoment Moment_Parse(char *string);
pMoment Moment_Clone(pMoment src);
```

範例：

```c
pMoment a = Moment_Now();
pMoment b = Moment_Second(1628578601);
pMoment c = Moment_Millisecond(1628578601123);
pMoment d = Moment_Parse("2013-02-08 09:30:26.123+08:00");
pMoment e = Moment_Clone(d);
```

### 4.2 設定與讀取

```c
pMoment Moment_Set_Clone(pMoment dst, pMoment src);
pMoment Moment_Set_utcOffset(pMoment m, int utcOffset);
time_t  Moment_Get_Sec(pMoment m);
time_t  Moment_Get_Millisecond(pMoment m);
```

說明：

- `utcOffset` 單位是秒，例如 `+08:00` 請傳 `28800`。

### 4.3 比較

```c
pMoment Moment_Max(pMoment first, ...);  // 最後一個參數必須是 NULL
pMoment Moment_Min(pMoment first, ...);  // 最後一個參數必須是 NULL
```

範例：

```c
pMoment latest = Moment_Max(a, b, c, NULL);
pMoment earliest = Moment_Min(a, b, c, NULL);
```

### 4.4 格式化

```c
char *Moment_Format(pMoment m, char *format);
char *Moment_strftime(pMoment m, char *format);
int   Moment_snprintf(char *buf, size_t n, char *format, time_t sec);
```

範例：

```c
printf("%s\n", Moment_Format(m, NULL));
printf("%s\n", Moment_Format(m, "YYYY/MM/DD HH:mm:ss.SSS Z"));
printf("%s\n", Moment_strftime(m, "%Y-%m-%d %H:%M:%S"));

char out[64];
Moment_snprintf(out, sizeof(out), "YYYY-MM-DD", 1700000000);
printf("%s\n", out);
```

### 4.5 加減時間

```c
pMoment Moment_Add(pMoment m, long int number, char *unit);
pMoment Moment_Subtract(pMoment m, long int number, char *unit);
```

支援單位：

| 單位 | 縮寫 |
| --- | --- |
| years | y |
| months | M |
| weeks | w |
| days | d |
| hours | h |
| minutes | m |
| seconds | s |
| milliseconds | ms |

### 4.6 區間起訖

```c
pMoment Moment_StartOf(pMoment m, char *unit);
pMoment Moment_EndOf(pMoment m, char *unit);
```

支援單位：

- `year`
- `month`
- `week`
- `day` / `date`
- `hour`
- `minute`
- `second`

---

## 5. Moment_Format Token 對照

| 類別 | Token | 範例輸出 |
| --- | --- | --- |
| Year | `YY` / `YYYY` | `24` / `2024` |
| Month | `M` `MM` `MMM` `MMMM` `Mo` | `8` `08` `Aug` `August` `8th` |
| Day | `D` `DD` `Do` | `9` `09` `9th` |
| Day of Year | `DDD` `DDDD` `DDDo` | `221` `221` `221st` |
| Weekday | `d` `dd` `ddd` `dddd` `do` | `1` `Mo` `Mon` `Monday` `1st` |
| Week | `w` `ww` `wo` / `W` `WW` `Wo` | `32` `32` `32nd` |
| Hour | `H` `HH` `h` `hh` `k` `kk` | `3` `03` `3` `03` `4` `04` |
| Minute | `m` `mm` | `5` `05` |
| Second | `s` `ss` | `7` `07` |
| Fraction | `S` `SS` `SSS` | `1` `12` `123` |
| Meridiem | `A` / `a` | `AM` / `am` |
| Timezone | `Z` / `ZZ` | `+08:00` / `+0800` |
| Unix | `X` / `x` | `1700000000` / `1700000000123` |
| Literal | `[text]` | 直接輸出 `text` |

---

## 6. Moment_Parse 支援格式（ISO 8601 子集合）

可解析日期 + 可選時間 + 可選時區，例如：

- `2013-02-08`
- `20130208`
- `2013-02`
- `2013-W06-5`
- `2013-039`
- `2013-02-08T09`
- `2013-02-08 09:30`
- `2013-02-08 09:30:26`
- `2013-02-08 09:30:26.123`
- `20130208T080910.123`
- `2013-02-08T09:30:26+08:00`
- `2013-02-08T01:30:26Z`

不合法字串會回傳 `NULL`。

---

## 7. 測試

### 7.1 編譯並執行

```bash
gcc -Wall -Wextra -std=c11 src/moment.c tests/test_moment.c -o moment_test.exe
./moment_test.exe
```

測試涵蓋：

- 所有公開 API
- `NULL` / 無效輸入防呆
- `malloc` 失敗路徑的回傳檢查（可透過注入方式擴充）
- 格式化、加減、StartOf/EndOf、Max/Min 行為

---

## 8. 注意事項

- 本專案目前以 GCC/Clang 類環境為主（使用 `timegm`、`gmtime_r`）。
- `Moment` 結構內含狀態與輸出緩衝，不建議多執行緒同時存取同一個 `pMoment`。
- 若要長期保存 `Moment_Format` 回傳字串，請自行 `strdup` 複製。
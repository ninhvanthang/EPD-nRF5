#include "GUI.h"

#include <stdio.h>
#include <time.h>

#include "Lunar.h"
#include "fonts.h"
#include "background.h"
// #include "Adafruit_GFX.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define GFX_printf_styled(gfx, fg, bg, font, ...) \
    GFX_setTextColor(gfx, fg, bg);                \
    GFX_setFont(gfx, font);                       \
    GFX_printf(gfx, __VA_ARGS__);

// height to use larger layout
#define large_layout(data) ((data)->height >= 400)

typedef struct {
    uint8_t month;
    uint8_t day;
    char name[10];  // 3x3+1
} Festival;

static const Festival festivals[] = {
    {1, 1, "元旦节"},  {2, 14, "情人节"}, {3, 8, "妇女节"},  {3, 12, "植树节"},  {4, 1, "愚人节"},
    {5, 1, "劳动节"},  {5, 4, "青年节"},  {6, 1, "儿童节"},  {7, 1, "建党节"},   {8, 1, "建军节"},
    {9, 10, "教师节"}, {10, 1, "国庆节"}, {11, 1, "万圣节"}, {12, 24, "平安夜"}, {12, 25, "圣诞节"},
};

static const Festival festivals_lunar[] = {
    {1, 1, "春节"},    {1, 15, "元宵节"}, {2, 2, "龙抬头"},  {5, 5, "端午节"},  {7, 7, "七夕节"}, {7, 15, "中元节"},
    {8, 15, "中秋节"}, {9, 9, "重阳节"},  {10, 1, "寒衣节"}, {12, 8, "腊八节"}, {12, 30, "除夕"},
};

// 放假和调休数据，每年更新
#define HOLIDAY_YEAR 2026
static const uint16_t holidays[] = {
    0x0101
};

static bool GetHoliday(uint8_t mon, uint8_t day, bool* work) {
    for (uint8_t i = 0; i < ARRAY_SIZE(holidays); i++) {
        if (((holidays[i] >> 8) & 0xF) == mon && (holidays[i] & 0xFF) == day) {
            *work = ((holidays[i] >> 12) & 0xF) > 0;
            return true;
        }
    }
    return false;
}

static bool GetFestival(uint16_t year, uint8_t mon, uint8_t day, uint8_t week, struct Lunar_Date* Lunar,
                        char* festival) {
    // 农历节日
    for (uint8_t i = 0; i < ARRAY_SIZE(festivals_lunar); i++) {
        if (Lunar->Month == festivals_lunar[i].month && Lunar->Date == festivals_lunar[i].day) {
            strcpy(festival, festivals_lunar[i].name);
            return true;
        }
    }

    // 除夕：春节前一天（12/29 或 12/30），12/30 已在上面判断
    if (Lunar->Month == 12 && Lunar->Date == 29) {
        struct Lunar_Date nextLunar;
        struct devtm tm = {year, mon, day, 0, 0, 0, week};
        transformTime(transformTimeStruct(&tm) + 86400, &tm);
        LUNAR_SolarToLunar(&nextLunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);
        if (nextLunar.Month == 1 && nextLunar.Date == 1) {
            strcpy(festival, "除夕");
            return true;
        }
    }
    // 母亲节: 五月第二个星期日
    if (mon == 5 && week == 0 && day >= 8 && day <= 14) {
        strcpy(festival, "母亲节");
        return true;
    }
    // 父亲节: 六月第三个星期日
    if (mon == 6 && week == 0 && day >= 15 && day <= 21) {
        strcpy(festival, "父亲节");
        return true;
    }
    // 感恩节：十一月第四个星期四
    if (mon == 11 && week == 4 && day >= 22 && day <= 28) {
        strcpy(festival, "感恩节");
        return true;
    }

    // 公历节日
    for (uint8_t i = 0; i < ARRAY_SIZE(festivals); i++) {
        if (mon == festivals[i].month && day == festivals[i].day) {
            strcpy(festival, festivals[i].name);
            return true;
        }
    }

    // 二十四节气
    uint8_t JQdate;
    if (GetJieQi(year, mon, day, &JQdate) && JQdate == day) {
        uint8_t JQ = (mon - 1) * 2;
        if (day >= 15) JQ++;
        strcpy(festival, JieQiStr[JQ]);
        if (JQ == 6)  // 清明
            strcat(festival, "节");

        return true;
    }

    return false;
}

static void DrawTimeSyncTip(Adafruit_GFX* gfx, gui_data_t* data) {

}


void DrawBackground(Adafruit_GFX *gfx)
{
    uint32_t x = 0;
    uint32_t y = 0;

    for (uint32_t i = 0; i < BACKGROUND_RLE_RUNS; i++) {

        uint16_t rle = background_rle[i];
        uint8_t color = rle & 0x03;
        uint16_t count = (rle >> 2) + 1;

        for (uint16_t j = 0; j < count; j++) {

            if (color == BG_BLACK) {
                GFX_drawPixel(gfx, x, y, GFX_BLACK);
            } else if (color == BG_RED) {
                GFX_drawPixel(gfx, x, y, GFX_RED);
            }

            x++;

            if (x >= BACKGROUND_WIDTH) {
                x = 0;
                y++;

                if (y >= BACKGROUND_HEIGHT) {
                    return;
                }
            }
        }
    }
}
static int GetTextWidth(Adafruit_GFX *gfx, const uint8_t *font, const char *text)
{
    GFX_setFont(gfx, font);
    return GFX_getUTF8Width(gfx, text);
}

/*

*/
static void DrawCustom1(Adafruit_GFX* gfx, tm_t* tm, struct Lunar_Date* Lunar, gui_data_t* data) {
    DrawBackground(gfx);
    
    char hour_str[4];
    char minute_str[4];

    snprintf(hour_str, sizeof(hour_str), "%02d", tm->tm_hour);
    snprintf(minute_str, sizeof(minute_str), "%02d", tm->tm_min);

    const int gap = 18;

    int hour_width = GetTextWidth(
        gfx,
        u8g2_font_sqch_66,
        hour_str
    );
    int minute_width = GetTextWidth(
        gfx,
        u8g2_font_sqon_66,
        minute_str
    );

    int total_width = hour_width + gap + minute_width;
    int x = (400 - total_width) / 2;

    // Hour
    GFX_setFont(gfx, u8g2_font_sqch_66);
    GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
    GFX_setCursor(gfx, 200 - 9 - GFX_getUTF8Width(gfx, hour_str), 109);
    GFX_printf(gfx, hour_str);

    // Min

    GFX_setFont(gfx, u8g2_font_sqon_66);
    GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
    GFX_setCursor(gfx, 209, 109);
    GFX_printf(gfx, minute_str);

    /*
        Date - Lunar Date 
    */
   
    char solar_str[16];
    char lunar_str[16];

    snprintf(solar_str, sizeof(solar_str),
            "%02d/%02d/%04d",
            tm->tm_mday,
            tm->tm_mon + 1,
            tm->tm_year + YEAR0);

    snprintf(lunar_str, sizeof(lunar_str),
            "%02d/%02d",
            Lunar->Date,
            Lunar->Month);


    GFX_setFont(gfx, u8g2_font_syte_10);

    int solar_width = GetTextWidth(
        gfx,
        u8g2_font_syte_10,
        solar_str
    );

    int lunar_width = GetTextWidth(
        gfx,
        u8g2_font_syte_10,
        lunar_str
    );

    x = (400 - solar_width - lunar_width - 6) / 2;

    GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
    GFX_setCursor(gfx, x, 184);
    GFX_printf(gfx, "%s", solar_str);

    x += solar_width + 6;
    GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
    GFX_setCursor(gfx, x, 184);
    GFX_printf(gfx, "%s", lunar_str);

    /*
        BATTERY
    */

    GFX_setFont(gfx, u8g2_font_syte_5);
    GFX_setTextColor(gfx, GFX_RED, GFX_BLACK);
    GFX_setCursor(gfx, 72, 190);
    GFX_printf(gfx, "%.1fv", data->voltage);

    /*
        TEMP
    */

    GFX_setFont(gfx, u8g2_font_syte_5);
    GFX_setTextColor(gfx, GFX_RED, GFX_BLACK);
    GFX_setCursor(gfx, 352, 190);
    GFX_printf(gfx, "%d*c", data->temperature);



    //PRINT Day of week in Vietnamese
    static const char *wdaytxt[] = {
        "CHỦ NHẬT",
        "THỨ HAI",
        "THỨ BA",
        "THỨ TƯ",
        "THỨ NĂM",
        "THỨ SÁU",
        "THỨ BẢY",
    };

    GFX_setFont(gfx, u8g2_font_bungee_35);
    GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
    GFX_setCursor(gfx, 200 - GFX_getUTF8Width(gfx, wdaytxt[tm->tm_wday])/2, 160);
    GFX_printf(gfx, "%s", wdaytxt[tm->tm_wday]);


    /*
        WEEK CALENDAR
    */

    //print arrow for current 

    int weekday = (tm->tm_wday + 6) % 7;

    GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
    GFX_setCursor(gfx, 34 + 54 * weekday, 227);
    GFX_setFont(gfx, u8g2_font_syte_5);
    GFX_printf(gfx, "_");
    
    struct tm monday = {0};

    monday.tm_year = tm->tm_year;
    monday.tm_mon  = tm->tm_mon;
    monday.tm_mday = tm->tm_mday;
    monday.tm_wday = tm->tm_wday;

    monday.tm_mday -= (monday.tm_wday + 6) % 7;

    mktime(&monday);

    struct tm date = monday;

    x = 39;
    for (int i = 0; i < 7; i++)
    {

        char day1[3];
        char day2[3];
        snprintf(day1, sizeof(day1), "%1d", date.tm_mday);
        GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
        GFX_setFont(gfx, u8g2_font_syte_20);
        
        GFX_setCursor(gfx, x - GFX_getUTF8Width(gfx, day1)/2 , 263);
        GFX_printf(gfx, "%s", day1);

        LUNAR_SolarToLunar(Lunar, date.tm_year + YEAR0, date.tm_mon + 1, date.tm_mday);
        
        snprintf(day2, sizeof(day2), "%1d", Lunar->Date);
        GFX_setFont(gfx, u8g2_font_syte_10);
        GFX_setCursor(gfx, x - GFX_getUTF8Width(gfx, day2)/2 , 280);
        GFX_printf(gfx, "%s", day2);

        date.tm_mday++;
        mktime(&date);
        x = x + 54;
    }

    // GFX_setFont(gfx, u8g2_font_sqon_66);
    // GFX_setCursor(gfx, 0, 136);
    // GFX_printf(gfx, "0 1 23");

    // GFX_setFont(gfx, u8g2_font_syte_20);
    // GFX_setCursor(gfx, 0, 160);
    // GFX_printf(gfx, "0123456789");

    // GFX_setFont(gfx, u8g2_font_syte_10);
    // GFX_setCursor(gfx, 0, 175);
    // GFX_printf(gfx, "0123456789/");

    // GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
    // GFX_setFont(gfx, u8g2_font_syte_5);
    // GFX_setCursor(gfx, 0, 185);
    // GFX_printf(gfx, "0123456789.*_30*c 3.5v");
    
    

    // GFX_setCursor(gfx, 20, 80);
    // GFX_printf(
    //     gfx,
    //     "%02d:%02d",
    //     tm->tm_hour,
    //     tm->tm_min
    // );
}

void DrawGUI(gui_data_t* data, buffer_callback callback, void* callback_data) {
    if (data->week_start > 6) data->week_start = 0;

    tm_t tm = {0};
    struct Lunar_Date Lunar;

    transformTime(data->timestamp, &tm);

    Adafruit_GFX gfx;
    int16_t ph = (__HEAP_SIZE - 512) / (data->width / 8);

    if (data->color == 2)
        GFX_begin_3c(&gfx, data->width, data->height, ph);
    else if (data->color == 3)
        GFX_begin_4c(&gfx, data->width, data->height, ph);
    else
        GFX_begin(&gfx, data->width, data->height, ph);

    GFX_firstPage(&gfx);
    do {
        GFX_fillScreen(&gfx, GFX_WHITE);

        LUNAR_SolarToLunar(&Lunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);

        switch (data->mode) {
            case MODE_CALENDAR:
                DrawCustom1(&gfx, &tm, &Lunar, data);
                break;
            case MODE_CLOCK:
                DrawCustom1(&gfx, &tm, &Lunar, data);
                break;
            case MODE_CUSTOM_1:
                DrawCustom1(&gfx, &tm, &Lunar, data);
                break;
            default:
                break;
        }
        if (tm.tm_year + YEAR0 == 2025 && tm.tm_mon + 1 == 1) {
            DrawTimeSyncTip(&gfx, data);
        }
    } while (GFX_nextPage(&gfx, callback, callback_data));

    GFX_end(&gfx);
}

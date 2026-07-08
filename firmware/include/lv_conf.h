/* LVGL configuration for the E46 race dash.
 * Only overrides are listed; everything else falls back to
 * lv_conf_internal.h defaults. */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH        16
#define LV_MEM_SIZE           (128U * 1024U)
#define LV_DEF_REFR_PERIOD    16      /* ~60 fps */

#define LV_USE_LOG            0
#define LV_USE_ASSERT_NULL    1

/* Built-in fonts for labels / banner (big numerics are custom Bahnschrift) */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_DEFAULT       &lv_font_montserrat_14

/* Widgets we don't use — keep the build lean */
#define LV_USE_ANIMIMG        0
#define LV_USE_CALENDAR       0
#define LV_USE_CHART          0
#define LV_USE_DROPDOWN       0
#define LV_USE_KEYBOARD       0
#define LV_USE_LIST           0
#define LV_USE_MENU           0
#define LV_USE_MSGBOX         0
#define LV_USE_ROLLER         0
#define LV_USE_SLIDER         0
#define LV_USE_SPINBOX        0
#define LV_USE_SPINNER        0
#define LV_USE_SWITCH         0
#define LV_USE_TABLE          0
#define LV_USE_TABVIEW        0
#define LV_USE_TILEVIEW       0
#define LV_USE_WIN            0

#endif /* LV_CONF_H */

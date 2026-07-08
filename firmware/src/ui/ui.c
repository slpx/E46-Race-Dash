#include "ui.h"
#include "fonts/fonts.h"
#include "../faults.h"
#include <stdio.h>

// ── Palette (prototype CSS) ─────────────────────────────────────────────
#define C_BG      0x050607
#define C_FG      0xf2f3ef
#define C_DIM     0x585c64
#define C_LINE    0x212429
#define C_YELLOW  0xffc400
#define C_RED     0xff2617
#define C_GREEN   0x2ee04e
#define C_BLUE    0x4aa8ff
#define C_TAGIDLE 0x303339
#define C_LEDOFF  0x17191c
#define C_BARBG   0x101215
#define C_TICK    0x3a3e45
#define C_FILL_W  0xe6e7e3
#define C_BLACK   0x050607
#define C_RED_DK  0x3c0805

// ── Layout constants (800x480, 1cqw = 8px) ──────────────────────────────
#define ROW_LED_H   43
#define ROW_RPM_Y   43
#define ROW_RPM_H   50
#define MAIN_Y      93
#define MAIN_H      337
#define BOT_Y       430
#define BOT_H       50
#define COL_L_W     152     // left column
#define COL_C1_X    152     // combo box
#define COL_C2_X    416     // tires box
#define COL_R_X     680     // right column
#define CELL_H      112     // left/right cell height (337/3)

#define RPM_MAX     7000.0f
#define LED_ON_RPM  4800.0f
#define LED_FULL    6400.0f
#define LED_FLASH   6450.0f

// ── Widget refs ─────────────────────────────────────────────────────────
static lv_obj_t *leds[15];
static lv_obj_t *rpm_fill_w, *rpm_fill_y, *rpm_fill_r, *rpm_val;
static lv_obj_t *cell_water, *cell_oilt, *cell_oilp;      // cell bg objs
static lv_obj_t *val_water, *val_oilt, *val_oilp;
static lv_obj_t *lbl_water[2], *lbl_oilt[2], *lbl_oilp[2]; // name, unit
static lv_obj_t *val_gear, *val_speed;
static lv_obj_t *tire_cell[4], *tire_val[4], *tire_pos[4];
static lv_obj_t *cell_abs, *tag_abs;
static lv_obj_t *cell_fuel, *val_batt, *val_fuel;
static lv_obj_t *lbl_batt[2], *lbl_fuel[2];
static lv_obj_t *tps_fill, *tps_pct, *brk_lbl, *amb_val;
static lv_obj_t *banner, *banner_lbl;

// ── Helpers ─────────────────────────────────────────────────────────────
static lv_obj_t *box(lv_obj_t *parent)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, const lv_font_t *font,
                       uint32_t color, const char *txt)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_label_set_text(l, txt);
    return l;
}

static void hline(lv_obj_t *p, int x, int y, int w)
{
    lv_obj_t *o = box(p);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, 1);
    lv_obj_set_style_bg_color(o, lv_color_hex(C_LINE), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
}

static void vline(lv_obj_t *p, int x, int y, int h)
{
    lv_obj_t *o = box(p);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, 1, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(C_LINE), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
}

// A data cell: colored bg (for warn state) with name / unit / big value.
static lv_obj_t *data_cell(lv_obj_t *p, int x, int y, int w, int h,
                           const char *name, const char *unit,
                           const lv_font_t *val_font,
                           lv_obj_t **name_out, lv_obj_t **unit_out,
                           lv_obj_t **val_out)
{
    lv_obj_t *c = box(p);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_bg_color(c, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);

    lv_obj_t *n = label(c, &bahn_16, C_DIM, name);
    lv_obj_set_style_text_letter_space(n, 2, 0);
    lv_obj_align(n, LV_ALIGN_TOP_LEFT, 12, 9);

    lv_obj_t *u = label(c, &bahn_14, C_DIM, unit);
    lv_obj_align(u, LV_ALIGN_TOP_RIGHT, -12, 11);

    lv_obj_t *v = label(c, val_font, C_FG, "-");
    lv_obj_align(v, LV_ALIGN_BOTTOM_MID, 0, -14);

    if (name_out) *name_out = n;
    if (unit_out) *unit_out = u;
    if (val_out)  *val_out  = v;
    return c;
}

// Warn/cold state for a cell: warn inverts bg to red w/ black text,
// cold turns the value blue.
static void cell_state(lv_obj_t *cell, lv_obj_t *name, lv_obj_t *unit,
                       lv_obj_t *val, bool warn, bool cold)
{
    lv_obj_set_style_bg_color(cell, lv_color_hex(warn ? C_RED : C_BG), 0);
    uint32_t vc = warn ? C_BLACK : (cold ? C_BLUE : C_FG);
    uint32_t lc = warn ? C_BLACK : C_DIM;
    lv_obj_set_style_text_color(val, lv_color_hex(vc), 0);
    if (name) lv_obj_set_style_text_color(name, lv_color_hex(lc), 0);
    if (unit) lv_obj_set_style_text_color(unit, lv_color_hex(lc), 0);
}

// ── Build ───────────────────────────────────────────────────────────────
void ui_create(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // ── Shift LEDs ──
    const int led_d = 21, led_gap = 19;
    const int led_x0 = (800 - (15 * led_d + 14 * led_gap)) / 2;
    for (int i = 0; i < 15; i++) {
        lv_obj_t *o = box(scr);
        lv_obj_set_pos(o, led_x0 + i * (led_d + led_gap), 13);
        lv_obj_set_size(o, led_d, led_d);
        lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(o, lv_color_hex(C_LEDOFF), 0);
        lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(o, 1, 0);
        lv_obj_set_style_border_color(o, lv_color_hex(0x26292e), 0);
        leds[i] = o;
    }

    // ── RPM bar ──
    lv_obj_t *bar = box(scr);
    lv_obj_set_pos(bar, 19, ROW_RPM_Y + 6);
    lv_obj_set_size(bar, 610, 38);
    lv_obj_set_style_bg_color(bar, lv_color_hex(C_BARBG), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(C_LINE), 0);

    // Fill = 3 fixed-color zones revealed left-to-right (white/yellow/red)
    struct { lv_obj_t **o; uint32_t col; } seg[3] = {
        { &rpm_fill_w, C_FILL_W }, { &rpm_fill_y, C_YELLOW }, { &rpm_fill_r, C_RED },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *o = box(bar);
        lv_obj_set_pos(o, 0, 0);
        lv_obj_set_size(o, 0, 36);
        lv_obj_set_style_bg_color(o, lv_color_hex(seg[i].col), 0);
        lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
        *seg[i].o = o;
    }

    // Ticks each 1000 rpm with small labels
    for (int k = 1; k <= 6; k++) {
        int x = (int)(610.0f * k / 7.0f);
        lv_obj_t *t = box(bar);
        lv_obj_set_pos(t, x, 0);
        lv_obj_set_size(t, 1, 36);
        lv_obj_set_style_bg_color(t, lv_color_hex(C_TICK), 0);
        lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
        char d[2] = { (char)('0' + k), 0 };
        lv_obj_t *l = label(bar, &bahn_14, C_DIM, d);
        lv_obj_set_pos(l, x + 4, 36 - 17);
    }

    lv_obj_t *rpm_lbl = label(scr, &bahn_16, C_DIM, "RPM");
    lv_obj_set_style_text_letter_space(rpm_lbl, 2, 0);
    lv_obj_align(rpm_lbl, LV_ALIGN_TOP_RIGHT, -19 - 110, ROW_RPM_Y + 22);
    rpm_val = label(scr, &bahn_36, C_FG, "0");
    lv_obj_align(rpm_val, LV_ALIGN_TOP_RIGHT, -19, ROW_RPM_Y + 6);

    // ── Main grid frame ──
    hline(scr, 0, MAIN_Y, 800);
    vline(scr, COL_L_W,  MAIN_Y, MAIN_H);
    vline(scr, COL_C2_X, MAIN_Y, MAIN_H);
    vline(scr, COL_R_X,  MAIN_Y, MAIN_H);

    // Left column cells
    cell_water = data_cell(scr, 0, MAIN_Y,              COL_L_W, CELL_H, "WATER", "\xC2\xB0""F",
                           &bahn_48, &lbl_water[0], &lbl_water[1], &val_water);
    cell_oilt  = data_cell(scr, 0, MAIN_Y + CELL_H,     COL_L_W, CELL_H, "OIL T", "\xC2\xB0""F",
                           &bahn_48, &lbl_oilt[0], &lbl_oilt[1], &val_oilt);
    cell_oilp  = data_cell(scr, 0, MAIN_Y + CELL_H * 2, COL_L_W, MAIN_H - CELL_H * 2, "OIL P", "psi",
                           &bahn_48, &lbl_oilp[0], &lbl_oilp[1], &val_oilp);
    hline(scr, 0, MAIN_Y + CELL_H, COL_L_W);
    hline(scr, 0, MAIN_Y + CELL_H * 2, COL_L_W);

    // Combo: gear over speed
    val_gear = label(scr, &bahn_160, C_FG, "N");
    lv_obj_align(val_gear, LV_ALIGN_TOP_MID, (COL_C1_X + COL_C2_X) / 2 - 400, MAIN_Y + 28);
    val_speed = label(scr, &bahn_72, C_FG, "0");
    lv_obj_align(val_speed, LV_ALIGN_TOP_MID, (COL_C1_X + COL_C2_X) / 2 - 400, MAIN_Y + 205);
    lv_obj_t *sp_lbl = label(scr, &bahn_16, C_DIM, "SPEED");
    lv_obj_set_style_text_letter_space(sp_lbl, 3, 0);
    lv_obj_align(sp_lbl, LV_ALIGN_TOP_MID, (COL_C1_X + COL_C2_X) / 2 - 400, MAIN_Y + 297);

    // Tires 2x2 (nose up: FL FR / RL RR)
    const int tx = COL_C2_X, tw = COL_R_X - COL_C2_X;   // 264
    const int tgrid_h = 300;                            // label strip below
    static const char *pos_txt[4] = { "FL", "FR", "RL", "RR" };
    for (int i = 0; i < 4; i++) {
        int cx = tx + (i % 2) * (tw / 2);
        int cy = MAIN_Y + (i / 2) * (tgrid_h / 2);
        lv_obj_t *c = box(scr);
        lv_obj_set_pos(c, cx + 1, cy + 1);
        lv_obj_set_size(c, tw / 2 - 1, tgrid_h / 2 - 1);
        lv_obj_set_style_bg_color(c, lv_color_hex(C_BG), 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        tire_cell[i] = c;
        tire_pos[i] = label(c, &bahn_14, C_DIM, pos_txt[i]);
        lv_obj_set_style_text_letter_space(tire_pos[i], 2, 0);
        lv_obj_align(tire_pos[i], LV_ALIGN_TOP_MID, 0, 28);
        tire_val[i] = label(c, &bahn_48, C_FG, "-");
        lv_obj_align(tire_val[i], LV_ALIGN_TOP_MID, 0, 55);
    }
    vline(scr, tx + tw / 2, MAIN_Y, tgrid_h);
    hline(scr, tx, MAIN_Y + tgrid_h / 2, tw);
    lv_obj_t *tires_lbl = label(scr, &bahn_16, C_DIM, "TIRES psi");
    lv_obj_set_style_text_letter_space(tires_lbl, 2, 0);
    lv_obj_align(tires_lbl, LV_ALIGN_TOP_MID, tx + tw / 2 - 400, MAIN_Y + tgrid_h + 12);

    // Right column: ABS / BATT / FUEL
    cell_abs = box(scr);
    lv_obj_set_pos(cell_abs, COL_R_X + 1, MAIN_Y + 1);
    lv_obj_set_size(cell_abs, 800 - COL_R_X - 1, CELL_H - 1);
    lv_obj_set_style_bg_color(cell_abs, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(cell_abs, LV_OPA_COVER, 0);
    tag_abs = label(cell_abs, &bahn_28, C_TAGIDLE, "ABS");
    lv_obj_set_style_text_letter_space(tag_abs, 4, 0);
    lv_obj_align(tag_abs, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *c_batt = data_cell(scr, COL_R_X, MAIN_Y + CELL_H, 800 - COL_R_X, CELL_H,
                                 "BATT", "V", &bahn_36, &lbl_batt[0], &lbl_batt[1], &val_batt);
    (void)c_batt;
    cell_fuel = data_cell(scr, COL_R_X, MAIN_Y + CELL_H * 2, 800 - COL_R_X, MAIN_H - CELL_H * 2,
                          "FUEL", "gal", &bahn_36, &lbl_fuel[0], &lbl_fuel[1], &val_fuel);
    hline(scr, COL_R_X, MAIN_Y + CELL_H, 800 - COL_R_X);
    hline(scr, COL_R_X, MAIN_Y + CELL_H * 2, 800 - COL_R_X);

    // ── Bottom strip ──
    hline(scr, 0, BOT_Y, 800);
    lv_obj_t *tps_lbl = label(scr, &bahn_16, C_DIM, "TPS");
    lv_obj_set_style_text_letter_space(tps_lbl, 2, 0);
    lv_obj_align(tps_lbl, LV_ALIGN_TOP_LEFT, 19, BOT_Y + 17);
    lv_obj_t *tps_bar = box(scr);
    lv_obj_set_pos(tps_bar, 66, BOT_Y + 16);
    lv_obj_set_size(tps_bar, 170, 19);
    lv_obj_set_style_bg_color(tps_bar, lv_color_hex(C_BARBG), 0);
    lv_obj_set_style_bg_opa(tps_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tps_bar, 1, 0);
    lv_obj_set_style_border_color(tps_bar, lv_color_hex(C_LINE), 0);
    tps_fill = box(tps_bar);
    lv_obj_set_pos(tps_fill, 0, 0);
    lv_obj_set_size(tps_fill, 0, 17);
    lv_obj_set_style_bg_color(tps_fill, lv_color_hex(C_FILL_W), 0);
    lv_obj_set_style_bg_opa(tps_fill, LV_OPA_COVER, 0);
    tps_pct = label(scr, &bahn_16, C_FG, "0%");
    lv_obj_align(tps_pct, LV_ALIGN_TOP_LEFT, 246, BOT_Y + 17);

    brk_lbl = label(scr, &bahn_16, C_TAGIDLE, "BRK");
    lv_obj_set_style_text_letter_space(brk_lbl, 2, 0);
    lv_obj_align(brk_lbl, LV_ALIGN_TOP_LEFT, 330, BOT_Y + 17);

    lv_obj_t *amb_lbl = label(scr, &bahn_16, C_DIM, "AMB");
    lv_obj_set_style_text_letter_space(amb_lbl, 2, 0);
    lv_obj_align(amb_lbl, LV_ALIGN_TOP_RIGHT, -70, BOT_Y + 17);
    amb_val = label(scr, &bahn_20, C_FG, "-");
    lv_obj_align(amb_val, LV_ALIGN_TOP_RIGHT, -19, BOT_Y + 15);

    // Fault banner overlays the whole strip
    banner = box(scr);
    lv_obj_set_pos(banner, 0, BOT_Y + 1);
    lv_obj_set_size(banner, 800, BOT_H - 1);
    lv_obj_set_style_bg_color(banner, lv_color_hex(C_RED), 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
    banner_lbl = label(banner, &bahn_28, C_BLACK, "");
    lv_obj_set_style_text_letter_space(banner_lbl, 5, 0);
    lv_obj_align(banner_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
}

// ── Update ──────────────────────────────────────────────────────────────
static float c2f(float c)   { return c * 1.8f + 32.0f; }
static float bar2psi(float b) { return b * 14.5038f; }

void ui_set(const VehicleState *s, uint32_t faults, uint32_t now_ms)
{
    char buf[48];

    // Shift LEDs
    int lit = (int)((s->rpm - LED_ON_RPM) / ((LED_FULL - LED_ON_RPM) / 15.0f));
    bool flash_off = (s->rpm >= LED_FLASH) && ((now_ms / 120) & 1);
    for (int i = 0; i < 15; i++) {
        bool on = (i < lit) && !flash_off;
        uint32_t col = !on ? C_LEDOFF : (i < 5 ? C_GREEN : i < 10 ? C_YELLOW : C_RED);
        lv_obj_set_style_bg_color(leds[i], lv_color_hex(col), 0);
    }

    // RPM bar: reveal the three zones
    float frac = s->rpm / RPM_MAX;
    if (frac > 1.0f) frac = 1.0f;
    const float zy = 0.785f, zr = 0.928f;   // zone boundaries (5500 / 6500)
    float fw = frac < zy ? frac : zy;
    float fy = frac < zy ? 0 : (frac < zr ? frac - zy : zr - zy);
    float fr = frac < zr ? 0 : frac - zr;
    lv_obj_set_size(rpm_fill_w, (int)(610 * fw), 36);
    lv_obj_set_pos(rpm_fill_y, (int)(610 * zy), 0);
    lv_obj_set_size(rpm_fill_y, (int)(610 * fy), 36);
    lv_obj_set_pos(rpm_fill_r, (int)(610 * zr), 0);
    lv_obj_set_size(rpm_fill_r, (int)(610 * fr), 36);
    lv_label_set_text_fmt(rpm_val, "%d", (int)(s->rpm / 10) * 10);

    // Left column
    lv_label_set_text_fmt(val_water, "%d", (int)(c2f(s->coolant_c) + 0.5f));
    cell_state(cell_water, lbl_water[0], lbl_water[1], val_water,
               s->coolant_c >= 103.0f, s->coolant_c < 71.0f);

    if (s->oil_temp_valid) {
        lv_label_set_text_fmt(val_oilt, "%d", (int)(c2f(s->oil_temp_c) + 0.5f));
    } else {
        lv_label_set_text(val_oilt, "--");
    }
    cell_state(cell_oilt, lbl_oilt[0], lbl_oilt[1], val_oilt,
               s->oil_temp_valid && s->oil_temp_c >= 128.0f,
               s->oil_temp_valid && s->oil_temp_c < 60.0f);

    lv_label_set_text_fmt(val_oilp, "%d", (int)(bar2psi(s->oil_pressure_bar) + 0.5f));
    cell_state(cell_oilp, lbl_oilp[0], lbl_oilp[1], val_oilp,
               s->oil_pressure_bar < 1.8f && s->rpm > 2500.0f, false);

    // Gear + speed
    if (s->gear > 0) lv_label_set_text_fmt(val_gear, "%d", s->gear);
    else             lv_label_set_text(val_gear, "N");
    lv_label_set_text_fmt(val_speed, "%d", (int)(s->speed_kmh * 0.621371f + 0.5f));

    // Tires
    for (int i = 0; i < 4; i++) {
        if (s->tire_valid[i]) {
            snprintf(buf, sizeof buf, "%.1f", (double)s->tire_psi[i]);
            lv_label_set_text(tire_val[i], buf);
        } else {
            lv_label_set_text(tire_val[i], "--");
        }
        bool warn = s->tire_valid[i] && (s->tire_psi[i] < 26.0f || s->tire_psi[i] > 42.0f);
        lv_obj_set_style_bg_color(tire_cell[i], lv_color_hex(warn ? C_RED : C_BG), 0);
        lv_obj_set_style_text_color(tire_val[i], lv_color_hex(warn ? C_BLACK : C_FG), 0);
        lv_obj_set_style_text_color(tire_pos[i], lv_color_hex(warn ? C_BLACK : C_DIM), 0);
    }

    // ABS box: fault = solid red; active = flashing yellow
    bool abs_fault = (faults & FAULT_ABS) != 0;
    bool abs_on = s->abs_active && ((now_ms / 200) & 1) == 0;
    uint32_t abs_bg = abs_fault ? C_RED : (abs_on ? C_YELLOW : C_BG);
    uint32_t abs_fg = (abs_fault || abs_on) ? C_BLACK : C_TAGIDLE;
    lv_obj_set_style_bg_color(cell_abs, lv_color_hex(abs_bg), 0);
    lv_obj_set_style_text_color(tag_abs, lv_color_hex(abs_fg), 0);

    // BATT / FUEL
    snprintf(buf, sizeof buf, "%.1f", (double)s->battery_v);
    lv_label_set_text(val_batt, buf);
    float gal = s->fuel_level_raw ? s->fuel_level_raw * 0.28f : 12.4f; // TODO: sender curve
    snprintf(buf, sizeof buf, "%.1f", (double)gal);
    lv_label_set_text(val_fuel, buf);
    cell_state(cell_fuel, lbl_fuel[0], lbl_fuel[1], val_fuel, gal < 2.0f, false);

    // Bottom strip
    lv_obj_set_size(tps_fill, (int)(168 * s->throttle_pct / 100.0f), 17);
    lv_label_set_text_fmt(tps_pct, "%d%%", (int)s->throttle_pct);
    lv_obj_set_style_text_color(brk_lbl,
        lv_color_hex(s->brake_switch ? C_RED : C_TAGIDLE), 0);
    lv_label_set_text_fmt(amb_val, "%d\xC2\xB0", (int)(c2f(s->ambient_c) + 0.5f));

    // Fault banner (cycles; flashes at ~3 Hz)
    char btxt[32];
    const char *bt = faults_banner_text(faults, s, now_ms, btxt, sizeof btxt);
    if (bt) {
        bool bright = ((now_ms / 300) & 1) == 0;
        lv_obj_remove_flag(banner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(banner, lv_color_hex(bright ? C_RED : C_RED_DK), 0);
        lv_obj_set_style_text_color(banner_lbl, lv_color_hex(bright ? C_BLACK : C_RED), 0);
        lv_label_set_text(banner_lbl, bt);
    } else {
        lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
    }
}

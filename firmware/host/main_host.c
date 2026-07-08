// Host snapshot harness: renders the real dash UI (LVGL) into a memory
// framebuffer and writes BMP screenshots — visual verification with no
// hardware. Build/run via tools/build_host.py.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "../src/vehicle_state.h"
#include "../src/sim.h"
#include "../src/faults.h"
#include "../src/ui/ui.h"

#define W 800
#define H 480

static uint16_t fb[W * H];

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area; (void)px_map;           // DIRECT mode: fb already holds the frame
    lv_display_flush_ready(disp);
}

static void save_bmp(const char *path)
{
    // 24-bit BMP, bottom-up
    const int row = W * 3;
    const int img = row * H;
    uint8_t hdr[54] = { 'B', 'M' };
    uint32_t fsize = 54 + img;
    memcpy(hdr + 2, &fsize, 4);
    uint32_t off = 54;      memcpy(hdr + 10, &off, 4);
    uint32_t ihs = 40;      memcpy(hdr + 14, &ihs, 4);
    int32_t w = W, h = H;   memcpy(hdr + 18, &w, 4); memcpy(hdr + 22, &h, 4);
    uint16_t planes = 1, bpp = 24;
    memcpy(hdr + 26, &planes, 2); memcpy(hdr + 28, &bpp, 2);
    uint32_t isz = img;     memcpy(hdr + 34, &isz, 4);

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(1); }
    fwrite(hdr, 1, 54, f);
    for (int y = H - 1; y >= 0; y--) {
        for (int x = 0; x < W; x++) {
            uint16_t v = fb[y * W + x];
            uint8_t bgr[3] = {
                (uint8_t)((v & 0x1F) << 3),          // B
                (uint8_t)(((v >> 5) & 0x3F) << 2),   // G
                (uint8_t)(((v >> 11) & 0x1F) << 3),  // R
            };
            fwrite(bgr, 1, 3, f);
        }
    }
    fclose(f);
    printf("wrote %s\n", path);
}

static void render(uint32_t now_ms)
{
    lv_tick_inc(33);
    lv_refr_now(NULL);
    (void)now_ms;
}

static void snap(VehicleState *s, uint32_t now_ms, const char *dir, const char *name)
{
    uint32_t f = faults_eval(s, now_ms);
    ui_set(s, f, now_ms);
    render(now_ms);
    char path[512];
    snprintf(path, sizeof path, "%s/%s.bmp", dir, name);
    save_bmp(path);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";

    lv_init();
    lv_display_t *disp = lv_display_create(W, H);
    lv_display_set_buffers(disp, fb, NULL, sizeof fb, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    ui_create();

    VehicleState s;
    memset(&s, 0, sizeof s);
    srand(42);
    sim_init(&s);

    // Timestamps are chosen so sim freshness stamps match `now` at each snap
    // and blink phases land on their bright half.

    // ── cold: just started, temps blue ──
    for (int i = 0; i < 40; i++) sim_step(&s, 0.05f, 1000);
    snap(&s, 1000, dir, "01_cold");

    // ── race: warmed up, caught at high rpm in a tall gear ──
    for (int i = 0; i < 4800; i++) sim_step(&s, 0.05f, 2000);   // ~4 min
    for (int i = 0; i < 4000 && !(s.gear >= 4 && s.rpm > 5900); i++)
        sim_step(&s, 0.05f, 2000);
    snap(&s, 2000, dir, "02_race");

    // ── braking with ABS flutter (3200: (3200/200)&1==0 -> yellow phase) ──
    for (int i = 0; i < 4000 && !(s.brake_switch && s.speed_kmh > 130); i++)
        sim_step(&s, 0.05f, 3200);
    s.abs_active = true;
    snap(&s, 3200, dir, "03_abs");

    // ── faults: CEL + low tire + low oil pressure + dead ABS module ──
    // Chosen nows are banner-bright ((now/300) even) and cycle to a
    // different fault each: 10200 -> CEL, 11600 -> OIL PRESSURE, 14000 -> TIRE FR
    s.lamp_cel = true;
    s.tire_psi[WHEEL_FR] = 18.2f;
    s.oil_pressure_bar = 0.8f;
    s.rpm = 4200; s.gear = 3;
    s.speed_kmh = 89;                  // matches 4200 rpm in 3rd
    s.abs_active = false;
    s.last_abs_ms = 0;                 // ABS silent -> timeout fault
    static const uint32_t fault_nows[3] = { 10200, 11600, 14000 };
    static const char *fault_names[3] = { "04_faults_a", "04_faults_b", "04_faults_c" };
    for (int i = 0; i < 3; i++) {
        s.last_dme_ms = fault_nows[i] - 100;   // DME itself stays alive
        snap(&s, fault_nows[i], dir, fault_names[i]);
    }

    printf("done\n");
    return 0;
}

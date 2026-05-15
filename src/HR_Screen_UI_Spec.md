# HR Screen UI Spec — ESP32-C3 Smartwatch
## TFT_eSPI · ST7789 240×280 · Target: Modern & Clean

---

## Layout Overview

```
┌─────────────────────────────┐  y=0
│  HR  ♥ (beating)            │  y=0..65    → Row 1: label + heart icon
│                             │
│          78                 │  y=65..115  → Row 2: BPM value besar
│         BPM                 │
│ ─────────────────────────── │  y=120      → divider tipis
│  /\/\/\/\/\/\/\/\/\/\       │  y=124..210 → Row 3: waveform graph
│ ─────────────────────────── │  y=215      → divider tipis
│  SPO2          ELAPSED      │  y=215..280 → Row 4: SpO2 + timer
│  98%            0:08        │
└─────────────────────────────┘  y=280
```

---

## Color Palette

```cpp
// Background
#define C_BG        0x0000   // Pure black (RGB565)
#define C_SURFACE   0x0861   // #0a0a14 — card surface sangat gelap

// HR accent — merah dengan sedikit pink
#define C_HR_RED    0xE926   // #E24B4A — merah utama grafik & icon
#define C_HR_PINK   0xEA4B   // #D4537E — pink untuk fill gradient bawah
#define C_SPO2      0xF48E   // #ED93B1 — pink lebih terang untuk SpO2

// Text
#define C_WHITE     0xFFFF   // angka BPM
#define C_MUTED     0x8C71   // label kecil (HR, BPM, SPO2)
#define C_DIM       0x4208   // angka timer, divider
#define C_DIVIDER   0x18C3   // #1e1e2a — garis divider
```

---

## Row 1 — Header (y: 0–65)

**Posisi:**
- Label "HR" — x=16, y=28, font size 2 (TFT_eSPI), warna `C_MUTED`, letter spacing manual
- Heart icon — x=50..80, center y=42

**Heart Icon — path TFT_eSPI via Sprite:**

```cpp
// Heart icon 28×28 px, center di (65, 42)
// Gunakan Sprite 28×28, fill transparent (C_BG), lalu pushSprite

void drawHeartIcon(TFT_eSPI &tft, int cx, int cy, uint16_t color, float scale = 1.0) {
    // Gambar dengan fillTriangle + fillCircle — tidak butuh Sprite
    // Heart = 2 circle + 1 triangle bawah

    int r  = (int)(7  * scale);   // radius circle
    int cx1 = cx - r;             // center circle kiri
    int cx2 = cx + r;             // center circle kanan
    int cy1 = cy - 2;             // center circle (naik sedikit)

    // Dua circle atas
    tft.fillCircle(cx1, cy1, r, color);
    tft.fillCircle(cx2, cy1, r, color);

    // Triangle bawah — tutup bentuk hati
    // Titik kiri, kanan, bawah
    int lx = cx - r * 2 + 1;
    int rx = cx + r * 2 - 1;
    int ty = cy1;
    int by = cy + r + 4;
    tft.fillTriangle(lx, ty, rx, ty, cx, by, color);

    // Hilite kecil pojok kiri atas — biar keliatan modern
    tft.fillCircle(cx1 - 1, cy1 - 2, (int)(2.5 * scale), 0xF8CF); // pink muda
}
```

**Beat Animation — pakai millis():**

```cpp
// Di loop HR state:
static uint32_t lastBeat = 0;
static uint8_t  beatPhase = 0;    // 0=normal, 1=besar, 2=kembali
static float    heartScale = 1.0f;

void updateHeartBeat(bool newBeatDetected) {
    if (newBeatDetected) {
        lastBeat = millis();
        beatPhase = 1;
    }

    uint32_t elapsed = millis() - lastBeat;

    if (beatPhase == 1 && elapsed < 120) {
        heartScale = 1.0f + (elapsed / 120.0f) * 0.22f;   // grow ke 1.22x
    } else if (beatPhase == 1 && elapsed < 240) {
        beatPhase = 2;
        heartScale = 1.22f - ((elapsed - 120) / 120.0f) * 0.10f; // shrink ke 1.12x
    } else if (beatPhase == 2 && elapsed < 360) {
        heartScale = 1.12f - ((elapsed - 240) / 120.0f) * 0.12f; // balik ke 1.0x
    } else {
        heartScale = 1.0f;
        beatPhase  = 0;
    }

    // Redraw icon hanya kalau scale berubah signifikan
    // Hapus area icon dulu dengan fillRect warna BG, lalu redraw
    tft.fillRect(50, 28, 30, 30, C_BG);
    drawHeartIcon(tft, 65, 43, C_HR_RED, heartScale);
}
```

---

## Row 2 — BPM Value (y: 65–120)

```cpp
void drawBPM(TFT_eSPI &tft, uint8_t bpm) {
    // Hapus area dulu
    tft.fillRect(0, 65, 240, 55, C_BG);

    // Angka BPM — besar, tengah
    tft.setTextDatum(TC_DATUM);           // top center
    tft.setTextColor(C_WHITE, C_BG);
    tft.setTextSize(1);
    tft.loadFont(...);                    // atau pakai built-in font besar

    // Kalau pakai font2 bawaan TFT_eSPI (tinggi ~26px):
    // Untuk tampilan lebih besar, gunakan drawNumber dengan font yang besar
    // atau setTextSize(4) dengan font GLCD
    tft.setTextSize(4);
    tft.setTextFont(1);
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", bpm);
    tft.drawString(buf, 120, 68);         // x=center, y=top

    // Label BPM kecil di bawah
    tft.setTextSize(1);
    tft.setTextColor(C_MUTED, C_BG);
    tft.drawString("BPM", 120, 108);

    // Divider tipis
    tft.drawFastHLine(16, 122, 208, C_DIVIDER);
}
```

---

## Row 3 — Waveform Graph (y: 124–212)

**Prinsip scaling:**
- Range BPM display: 40 – 180 (total 140 unit)
- Tinggi graph area: **80px** (dari y=132 sampai y=212)
- Formula mapping: `yPixel = 212 - ((bpm - 40) / 140.0 * 80)`
- Spike ±15 BPM = ±8.5px → terlihat jelas tapi tidak mendominasi

```cpp
#define GRAPH_X      16      // x start
#define GRAPH_W      208     // lebar graph
#define GRAPH_Y_BOT  210     // y bawah graph (baseline)
#define GRAPH_H      80      // tinggi total graph area
#define BPM_MIN      40      // batas bawah scaling
#define BPM_RANGE    140     // 180 - 40

#define MAX_GRAPH_POINTS  208   // 1 pixel per sample

uint8_t  graphBuf[MAX_GRAPH_POINTS];   // circular buffer BPM values
uint8_t  graphHead = 0;
uint8_t  graphCount = 0;

// Konversi BPM ke y pixel
int bpmToY(uint8_t bpm) {
    int clamped = constrain(bpm, BPM_MIN, BPM_MIN + BPM_RANGE);
    return GRAPH_Y_BOT - (int)((clamped - BPM_MIN) / (float)BPM_RANGE * GRAPH_H);
}

// Push sample baru
void graphPushSample(uint8_t bpm) {
    graphBuf[graphHead] = bpm;
    graphHead = (graphHead + 1) % MAX_GRAPH_POINTS;
    if (graphCount < MAX_GRAPH_POINTS) graphCount++;
}

// Redraw seluruh graph — panggil saat ada sample baru
void drawGraph(TFT_eSPI &tft) {
    // Clear graph area
    tft.fillRect(GRAPH_X, GRAPH_Y_BOT - GRAPH_H, GRAPH_W, GRAPH_H, C_BG);

    if (graphCount < 2) return;

    // Draw filled area (simulasi gradient — gambar dari bawah ke atas
    // dengan opacity berkurang, pakai strip horizontal tipis)
    for (int row = 0; row < GRAPH_H; row++) {
        // Makin ke atas (dekat garis) makin opaque
        // Simplified: gambar garis full dulu, fill belakangan pakai approach lain
    }

    // Draw garis waveform
    for (int i = 1; i < graphCount; i++) {
        int idxPrev = (graphHead - graphCount + i - 1 + MAX_GRAPH_POINTS) % MAX_GRAPH_POINTS;
        int idxCurr = (graphHead - graphCount + i     + MAX_GRAPH_POINTS) % MAX_GRAPH_POINTS;

        int x1 = GRAPH_X + i - 1;
        int x2 = GRAPH_X + i;
        int y1 = bpmToY(graphBuf[idxPrev]);
        int y2 = bpmToY(graphBuf[idxCurr]);

        tft.drawLine(x1, y1, x2, y2, C_HR_RED);

        // Fill bawah garis ke baseline — buat efek area chart
        // Warna lebih gelap / muted untuk fill
        uint16_t fillColor = 0x6003;  // merah sangat gelap
        if (y1 < y2) {
            tft.drawLine(x1, y1 + 1, x1, GRAPH_Y_BOT, fillColor);
        } else {
            tft.drawLine(x2, y2 + 1, x2, GRAPH_Y_BOT, fillColor);
        }
    }

    tft.drawFastHLine(16, 214, 208, C_DIVIDER);
}
```

**Kapan update graph:**
- Push sample setiap kali `pox.update()` deteksi beat baru (`onBeatDetected` callback)
- Redraw full graph setiap ~500ms — tidak perlu tiap frame

---

## Row 4 — SpO2 + Timer (y: 214–280)

```cpp
void drawBottomRow(TFT_eSPI &tft, uint8_t spo2, uint32_t elapsedSec) {
    tft.fillRect(0, 218, 240, 62, C_BG);

    // Label SpO2
    tft.setTextDatum(BL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(C_MUTED, C_BG);
    tft.drawString("SPO2", 16, 236);

    // Nilai SpO2 — medium besar, pink
    tft.setTextColor(C_SPO2, C_BG);
    tft.setTextSize(3);
    char spo2Buf[4];
    snprintf(spo2Buf, sizeof(spo2Buf), "%d", spo2);
    tft.drawString(spo2Buf, 16, 238);

    // Simbol %
    tft.setTextSize(2);
    tft.setTextColor(C_SPO2, C_BG);
    // gambar % di sebelah kanan angka (hitung lebar manual)
    tft.drawString("%", 16 + 52, 252);

    // Label ELAPSED — kanan
    tft.setTextDatum(BR_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(C_MUTED, C_BG);
    tft.drawString("ELAPSED", 224, 236);

    // Nilai timer
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%d:%02d",
             (int)(elapsedSec / 60), (int)(elapsedSec % 60));
    tft.setTextColor(C_DIM, C_BG);
    tft.setTextSize(2);
    tft.drawString(timeBuf, 224, 238);
}
```

---

## Update Loop — State EXEC_HR

```cpp
// Di ui_manager.cpp, saat state == EXEC_HR:

static uint32_t hrStartTime   = 0;
static uint32_t lastGraphDraw = 0;
static uint32_t lastBpmRedraw = 0;
static uint8_t  lastBpm       = 0;
static uint8_t  lastSpo2      = 0;

void enterHRState(TFT_eSPI &tft) {
    hrStartTime = millis();
    tft.fillScreen(C_BG);
    // Draw static elements sekali
    tft.setTextColor(C_MUTED, C_BG);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("HR", 16, 18);
    drawHeartIcon(tft, 65, 43, C_HR_RED, 1.0f);
    drawBPM(tft, 0);
    drawBottomRow(tft, 0, 0);
}

void updateHRState(TFT_eSPI &tft) {
    uint8_t  bpm  = (uint8_t)max30100_hal_get_bpm();
    uint8_t  spo2 = max30100_hal_get_spo2();
    uint32_t elapsed = (millis() - hrStartTime) / 1000;

    max30100_hal_update();   // wajib tiap loop
    updateHeartBeat(max30100_hal_beat_detected());   // animasi jantung

    // Redraw BPM hanya kalau berubah
    if (bpm != lastBpm) {
        drawBPM(tft, bpm);
        graphPushSample(bpm);
        lastBpm = bpm;
    }

    // Redraw SpO2 kalau berubah
    if (spo2 != lastSpo2) {
        drawBottomRow(tft, spo2, elapsed);
        lastSpo2 = spo2;
    }

    // Redraw graph tiap 500ms
    if (millis() - lastGraphDraw > 500) {
        drawGraph(tft);
        lastGraphDraw = millis();
    }

    // Update timer setiap detik
    static uint32_t lastTimerDraw = 0;
    if (millis() - lastTimerDraw > 1000) {
        drawBottomRow(tft, lastSpo2, elapsed);
        lastTimerDraw = millis();
    }
}
```

---

## Catatan untuk Gemini

1. **Heart scale animation** — pakai `fillRect` warna BG untuk erase icon lama sebelum redraw. Lebih cepat dari full `fillScreen`.

2. **Graph fill** — fill area bawah garis pakai warna `0x6003` (merah sangat gelap) per kolom. Kalau mau lebih smooth, gambar dua shade: `0x8004` untuk setengah atas fill, `0x4002` untuk setengah bawah.

3. **Font BPM besar** — kalau `setTextSize(4)` font bawaan terlihat pixelated, pertimbangkan load custom font `.vlw` via TFT_eSPI. Font `NotoSans_Bold` ukuran 48 atau 56 akan jauh lebih smooth.

4. **Sprite approach** (yang sudah dipakai Gemini) — tetap bagus untuk heart icon. Cukup `heartSprite.pushSprite(50, 28)` dan scale via `heartSprite.drawSprite()` dengan width/height yang di-adjust per beat phase.

5. **Partial redraw** — jangan `fillScreen()` di loop. Erase per-region saja. Urutan draw: BPM area → graph area → bottom row. Header (HR + icon) hanya di-erase saat beat animasi aktif saja.

---

*UI Spec HR Screen — ESP32-C3 Smartwatch · TFT_eSPI ST7789 240×280*

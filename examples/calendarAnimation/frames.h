#pragma once
#include <Arduino.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*putpixel_fn)(int,int,int);

extern const uint8_t FRAME0[];
extern const uint8_t FRAME1[];
extern const uint8_t FRAME2[];
extern const uint8_t FRAME3[];
extern const uint8_t FRAME4[];
extern const uint8_t FRAME5[];
extern const uint8_t FRAME6[];
extern const uint8_t FRAME7[];
extern const uint8_t FRAME8[];
extern const uint8_t FRAME9[];
extern const uint8_t FRAME10[];
extern const uint8_t FRAME11[];
extern const uint8_t FRAME12[];
extern const uint8_t FRAME13[];
extern const uint8_t FRAME14[];
extern const uint8_t FRAME15[];
extern const uint8_t FRAME16[];
extern const uint8_t FRAME17[];
extern const uint8_t FRAME18[];
extern const uint8_t FRAME19[];
extern const uint8_t FRAME20[];
extern const uint8_t FRAME21[];
extern const uint8_t FRAME22[];
extern const uint8_t FRAME23[];
extern const uint8_t FRAME24[];
extern const uint8_t FRAME25[];
extern const uint8_t FRAME26[];
extern const uint8_t FRAME27[];
extern const uint8_t FRAME28[];
extern const uint8_t FRAME29[];

#define NUM_FRAMES 30

// Blit a PROGMEM 1-bit bitmap at (x,y). Format: header 4B (w,h LE), then packed bits.
void drawPic(const uint8_t *pgm_bitmap,int x,int y,putpixel_fn pset);

#ifdef __cplusplus
} // extern "C"
#endif

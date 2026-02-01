#pragma once

#include <Empire/assets.h>
#include <Empire/stb_truetype.h>

typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;

typedef struct emp_font_t {
    SDL_Texture* texture;
    stbtt_bakedchar cdata[96];
    float size;
} emp_font_t;

void emp_load_font(SDL_Renderer* renderer, emp_asset_t* font_asset, float size);

void emp_draw_text(float x, float y, const char* text, u8 r, u8 g, u8 b, emp_asset_t* font_asset);
#include <Empire/text.h>
#include <Empire/util.h>
#include "entities.h" // G

#include <SDL3/SDL.h>


void emp_load_font(SDL_Renderer* renderer, emp_asset_t* font_asset, float size) {
    emp_font_t* f = SDL_malloc(sizeof(emp_font_t));
    f->size = size;

	emp_buffer ttf_buffer = emp_read_entire_file(font_asset->path);

    int w = 512, h = 512;
    unsigned char* alpha_bitmap = SDL_malloc(w * h);
    stbtt_BakeFontBitmap(ttf_buffer.data, 0, size, alpha_bitmap, w, h, 32, 96, f->cdata);
	SDL_free(ttf_buffer.data);

    unsigned int* pixels = SDL_malloc(w * h * 4);
    for (int i = 0; i < w * h; i++) {
        unsigned char alpha = alpha_bitmap[i];
        pixels[i] = (0xFFFFFF << 8) | alpha;
    }

    f->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                                   SDL_TEXTUREACCESS_STATIC, w, h);

    SDL_UpdateTexture(f->texture, NULL, pixels, w * 4);
    SDL_SetTextureBlendMode(f->texture, SDL_BLENDMODE_BLEND);

    SDL_free(alpha_bitmap);
    SDL_free(pixels);
	font_asset->handle = f;
}

void emp_draw_text(float x, float y, const char* text, emp_asset_t* font_asset) {
	emp_font_t* font = font_asset->handle;
	SDL_Renderer* renderer = G->args->r;
	SDL_SetTextureColorMod(font->texture, 255, 255, 255); 

    while (*text) {
        if (*text >= 32 && *text <= 126) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->cdata, 512, 512, *text - 32, &x, &y, &q, 1);

            SDL_FRect src = {
                .x = q.s0 * 512.0f, 
                .y = q.t0 * 512.0f, 
                .w = (q.s1 - q.s0) * 512.0f, 
                .h = (q.t1 - q.t0) * 512.0f
            };

            SDL_FRect dst = {
                .x = q.x0, 
                .y = q.y0, 
                .w = q.x1 - q.x0, 
                .h = q.y1 - q.y0
            };

            SDL_RenderTexture(renderer, font->texture, &src, &dst);
        }
        text++;
    }
}
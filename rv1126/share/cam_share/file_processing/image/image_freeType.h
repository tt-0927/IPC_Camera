#ifndef IMAGE_FREE_TYPE_H
#define IMAGE_FREE_TYPE_H
#include <ft2build.h>

#include FT_FREETYPE_H
typedef struct Font_Object {
    void*        library;
     void*           face;
} Font_Object_t;

typedef struct Font_Object *Font_Handle_t;



Font_Handle_t font_create(const char *fileName);

int font_destory(Font_Handle_t hFont);

#endif

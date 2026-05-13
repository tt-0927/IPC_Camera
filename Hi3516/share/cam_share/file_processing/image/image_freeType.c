#include "image_freeType.h"
#include <stdio.h>
#include <ft2build.h>

#include FT_FREETYPE_H
Font_Handle_t font_create(const char *fileName)
{
	Font_Handle_t hFont;

	hFont = calloc(1, sizeof(*hFont));

	if(hFont == NULL) {
		//		PRINTF("Failed to allocate space for Font Object\n");
		return NULL;
	}

	if(FT_Init_FreeType((FT_Library *)(void *) &hFont->library)) {
		//		PRINTF("Failed to intialize freetype library\n");
		free(hFont);
		return NULL;
	}

	if(FT_New_Face((FT_Library)(void *) hFont->library, fileName, 0,
	               (FT_Face *)(void *) &hFont->face)) {
		//		PRINTF("Failed to load font %s\n", fileName);
		free(hFont);
		return NULL;
	}

	if(FT_Select_Charmap((FT_Face) hFont->face, FT_ENCODING_UNICODE)) {
		//		PRINTF("Invalid charmap [%d]\n", FT_ENCODING_UNICODE);
		free(hFont);
		return NULL;
	}

	//	PRINTF("++++++++++++++Font_create++++++++++++++++++\n");
	return hFont;
}

int font_destory(Font_Handle_t hFont)
{
	if(hFont)
	{
	     FT_Done_Face((FT_Face)hFont->face);

	     FT_Done_FreeType((FT_Library) hFont->library );

		 free(hFont);
		return 0;
	}
	return -1;
}


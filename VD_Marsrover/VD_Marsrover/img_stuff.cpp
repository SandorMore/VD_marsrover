#include "misc.h"
#include "raylib.h"
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IMG_ICON R"(..\x64\Debug\û.png)"


Image load_img_for_icon() {
	int w, h, channels;
	unsigned char* data = stbi_load(IMG_ICON, &w, &h, &channels, 4);

	if (!data) {
		printf("FAIL");
		return { 0 };
	}
	Image icon = { 0 };
	icon.data = data;
	icon.height = h;
	icon.width = w;
	icon.mipmaps = 1;
	icon.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

	return icon;
}
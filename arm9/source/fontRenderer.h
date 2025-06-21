#ifndef FONTRENDERER_H
#define FONTRENDERER_H

#include "nftr.h"
#include <string>
#include <vector>
#include "pixelCanvas.h"
#include "charSet.h"

class FontRenderer {
public:
	FontRenderer(const NFTR *font, PixelCanvas *canvas);
	~FontRenderer();
	
	void rend();
	
	void clear();
	void addText(char16_t chr);
	void addText(char16_t chr, uint8_t fgColor);
	void addText(const std::u16string &str);
	void addText(const std::u16string &str, uint8_t fgColor);

	struct TextRun {
		std::u16string text;
		uint8_t fgColor;
		TextRun(const std::u16string &text, uint8_t fgColor);
		TextRun(std::u16string &&text, uint8_t fgColor);
	};
	
	unsigned int startX;
	unsigned int startY;
	
	unsigned int tabStep;
	
	uint8_t defaultFgColor=1;

private:
	const NFTR *font;
	
	uint8_t *charPixelsBuff;
	
	PixelCanvas *canvas;
	
	
	unsigned int drawPosX;
	unsigned int drawPosY;
	
	void rendRun(const TextRun &run);
	void rendChar(char16_t c, uint8_t fgColor);
	
	std::vector<TextRun> runs;
};

#endif
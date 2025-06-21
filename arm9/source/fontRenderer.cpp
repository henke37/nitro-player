#include "fontRenderer.h"

#include "bitIterator.h"

#include <cassert>

FontRenderer::FontRenderer(const NFTR *font, PixelCanvas *canvas) : startX(0), startY(0), tabStep(4), font(font), canvas(canvas) {
	assert(font);
	assert(canvas);
}

FontRenderer::~FontRenderer() {
}

FontRenderer::TextRun::TextRun(const std::u16string &text, uint8_t fgColor) : text(text), fgColor(fgColor) {}
FontRenderer::TextRun::TextRun(std::u16string &&text, uint8_t fgColor) : text(std::move(text)), fgColor(fgColor) {}

void FontRenderer::addText(const std::u16string &str) {
	runs.emplace_back(str,defaultFgColor);
}
void FontRenderer::addText(const std::u16string &str, uint8_t fgColor) {
	runs.emplace_back(str,fgColor);
}

void FontRenderer::clear() {
	runs.clear();
}

void FontRenderer::addText(char16_t chr) {
	addText(chr, defaultFgColor);
}

void FontRenderer::addText(char16_t chr, uint8_t fgColor) {
	runs.emplace_back(std::u16string(1, chr), fgColor);
}

void FontRenderer::rend() {
	drawPosX=startX;
	drawPosY=startY;
	
	charPixelsBuff=new uint8_t[font->cellSize];
	for(auto itr=runs.cbegin();itr!=runs.cend();++itr) {
		auto &elm=*itr;
		rendRun(elm);
	}
	delete[] charPixelsBuff;
}

void FontRenderer::rendRun(const TextRun &run) {
	for(auto itr=run.text.cbegin();itr!=run.text.cend();++itr) {
		char16_t c=*itr;
		switch(c) {
			case L'\n':
				drawPosX=startY;
				drawPosY+=font->lineFeed;
			break;
			case L'\t': {
				drawPosX+=tabStep;
			} break;
			default:
				rendChar(c, run.fgColor);
		}
	}
}

void FontRenderer::rendChar(char16_t c, uint8_t fgColor) {
	uint16_t index=font->getGlyphIndex(c);
	const NFTR::ChrWidth &width=font->getGlyphWidth(index);
	
	font->getGlyphPixelData(index, charPixelsBuff);
	
	BitIterator bits(charPixelsBuff, font->cellSize);
		
	for(unsigned int y=0;y<font->cellHeight;++y) {
		for(unsigned int x=0;x<font->cellWidth;++x) {
			if(bits.readBit()) {
				unsigned int plotX=drawPosX+x, plotY=drawPosY+y;
				if(plotX>=canvas->getWidth()) continue;
				if(plotY>=canvas->getHeight()) continue;
				canvas->setPixelClipped(plotX, plotY, fgColor);
			}
		}
	}
	
	//GUESS!!
	drawPosX+=width.charWidth;
}
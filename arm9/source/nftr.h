#ifndef NFTR_H
#define NFTR_H

#include <vector>

#include "sectionedFile.h"
#include "binaryReader.h"

class NFTR {

public:
	NFTR(const std::string &filename);
	NFTR(std::unique_ptr<BinaryReadStream> &&stream);
	~NFTR();
	
	struct ChrWidth {
		int8_t left;
		uint8_t glyphWidth;
		int8_t charWidth;
	};
	
	enum class Encoding {
		UTF8,
		UTF16,
		SJIS,
		CP122
	};
	
	uint16_t getGlyphIndex(char16_t c) const;
	const ChrWidth &getGlyphWidth(uint16_t index) const;
	void getGlyphPixelData(uint16_t index, uint8_t *outBuff) const;
	
	unsigned int getTextWidth(const std::u16string &text) const;
	
	uint8_t lineFeed;
	uint8_t encoding;
	
	uint8_t height;
	uint8_t width;
	uint8_t ascent;
	
	uint8_t cellWidth;
	uint8_t cellHeight;
	uint16_t cellSize;
	int8_t baseLinePos;
	uint8_t maxCharWidth;
	uint8_t bpp;
	uint8_t flags;
	
private:
	SectionedFile sections;
	
	uint16_t altCharIndex;
	ChrWidth defChrWidth;
	
	std::unique_ptr<BinaryReadStream> pixelData;
	
	enum class MappingMethod {
		Direct=0,
		Table=1,
		Scan=2
	};
	
	class MappingData {
		public:
		const uint16_t ccodeBegin;
		const uint16_t ccodeEnd;
		virtual ~MappingData()=0;
		uint16_t getGlyphIndex(char16_t c) const;
		protected:
		
		MappingData(uint16_t ccodeBegin, uint16_t ccodeEnd);
		
		virtual uint16_t getGlyphIndexImpl(char16_t c) const=0;
	};
	
	class DirectMappingData : public MappingData {
		public:
		DirectMappingData(uint16_t ccodeBegin, uint16_t ccodeEnd, uint16_t offset);
		~DirectMappingData();
		uint16_t getGlyphIndexImpl(char16_t c) const;
		private:
		uint16_t offset;
	};
	
	class TableMappingData : public MappingData {
		public:
		TableMappingData(uint16_t ccodeBegin, uint16_t ccodeEnd,BinaryReader &);
		~TableMappingData();
		uint16_t getGlyphIndexImpl(char16_t c) const;
		private:
		std::vector<uint16_t> table;
	};
	
	class ScanMappingData : public MappingData {
		public:
		ScanMappingData(uint16_t ccodeBegin, uint16_t ccodeEnd,BinaryReader &);
		~ScanMappingData();
		uint16_t getGlyphIndexImpl(char16_t c) const;
		
		private:
		struct MapEntry {
			uint16_t ccode;
			uint16_t index;
			
			MapEntry(uint16_t ccode, uint16_t index);
		};
		
		std::vector<MapEntry> table;
	};
	
	class WidthData {
		public:
		WidthData(uint16_t indexBegin, uint16_t indexEnd, std::vector<ChrWidth> &&widths);
		const uint16_t indexBegin;
		const uint16_t indexEnd;
		
		const ChrWidth &getWidth(uint16_t index) const;
		
		private:
		const std::vector<ChrWidth> widths;
	};
	
	
	std::vector<WidthData> widths;
	std::vector<std::unique_ptr<MappingData>> maps;
	
	void readData();
	void parseFinfData(std::unique_ptr<BinaryReadStream> &&stream);
	void parseCglpData(std::unique_ptr<BinaryReadStream> &&stream);
	uint32_t parseCwdhData(std::unique_ptr<BinaryReadStream> &&cwdhData);
	uint32_t parseCmapData(std::unique_ptr<BinaryReadStream> &&cmapData);
};

#endif
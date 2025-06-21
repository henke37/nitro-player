#include "ncer.h"

#include "binaryReader.h"
#include "substream.h"

#include <cassert>

NCER::NCER(const std::string &filename) : sections(filename) {
	readData();
}
NCER::NCER(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
	readData();
}

void NCER::readData() {
	std::unique_ptr<BinaryReadStream> cebkData = sections.getSectionData("KBEC");
	
	assert(cebkData);
	parseCEBKData(std::move(cebkData));
}

void NCER::parseCEBKData(std::unique_ptr<BinaryReadStream> &&stream) {	
	BinaryReader hdrRead{stream.get(),false};
	
	uint16_t numCells=hdrRead.readLEShort();
	uint16_t bankFlags=hdrRead.readLEShort();
	size_t dataOffset=hdrRead.readLELong();

	mappingMode = (MappingMode) hdrRead.readLELong();
	
	//mapping mode, vram transfer data and two reserved spots for runtime pointers
	//hdr.skip(3*4); 
	
	size_t cellDataSize = ((bankFlags & 1) ? 16 : 8)*numCells;
	
	size_t oamStart = dataOffset + cellDataSize;
	
	SubStream cellDataStream(stream.get(), dataOffset, cellDataSize, false);
	BinaryReader cellRead{&cellDataStream,false};
	
	SubStream oamDataStream(stream.get(), oamStart, 0xFFFFFFFF, false);
	BinaryReader oamRead{&oamDataStream,false};
	
	cells.reserve(numCells);
	
	for(int cellIndex=0;cellIndex<numCells;++cellIndex) {
		AnimationCell &cell=cells.emplace_back(AnimationCell());
		
		int numObjs=cellRead.readLEShort();
		cell.atts=cellRead.readLEShort();
		size_t objOffset=cellRead.readLELong();
		
		if(bankFlags & 1) {
			cell.maxX=cellRead.readLEShort();
			cell.maxY=cellRead.readLEShort();
			cell.minX=cellRead.readLEShort();
			cell.minY=cellRead.readLEShort();
		}
		
		cell.objectDefs.reserve(numObjs);
		
		oamDataStream.setPos(objOffset);
		for(int objIndex=0;objIndex<numObjs;++objIndex) {
			SpriteEntry obj;
		
			obj.attribute[0]=oamRead.readLEShort();
			obj.attribute[1]=oamRead.readLEShort();
			obj.attribute[2]=oamRead.readLEShort();
			obj.filler=0;
			
			cell.objectDefs.push_back(obj);
		}
	}
}
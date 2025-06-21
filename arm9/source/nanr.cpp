#include "nanr.h"

#include <cassert>

#include "substream.h"
#include "binaryReader.h"

NANR::NANR(const std::string &filename) : sections(filename) {
	readData();
}
NANR::NANR(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
	readData();
}

void NANR::readData() {
	std::unique_ptr<BinaryReadStream> abnkData = sections.getSectionData("KNBA");	
	assert(abnkData);	
	parseABNKData(std::move(abnkData));
}

void NANR::parseABNKData(std::unique_ptr<BinaryReadStream> &&stream) {
	BinaryReader hdrRead{stream.get(),false};
	uint16_t animCount=hdrRead.readLEShort();
	hdrRead.skip(2);//totalFrames
	size_t animOffset=hdrRead.readLELong();
	size_t frameBaseOffset=hdrRead.readLELong();
	size_t positionBaseOffset=hdrRead.readLELong();
	
	anims.reserve(animCount);
	
	SubStream animStream(stream.get(), animOffset, 0xFFFFFFFF, false);
	SubStream frameBaseStream(stream.get(), frameBaseOffset, 0xFFFFFFFF, false);
	SubStream positionStream(stream.get(), positionBaseOffset, 0xFFFFFFFF, false);
	
	BinaryReader animReader(&animStream, false);
	BinaryReader frameBaseReader(&frameBaseStream, false);
	BinaryReader positionReader(&positionStream, false);
	
	for(int animIndex=0;animIndex<animCount;++animIndex) {
		uint16_t frameCount=animReader.readLEShort();
		uint16_t loopStart=animReader.readLEShort();
		uint16_t posType=animReader.readLEShort();
		animReader.skip(2);//Cell vs multicell
		uint32_t playbackType=animReader.readLELong();
		size_t frameOffset=animReader.readLELong();
		
		auto &anim=anims.emplace_back(playbackType, loopStart);
		anim.frames.reserve(frameCount);
		
		frameBaseReader.setPos(frameOffset);

		sassert(loopStart < frameCount,"Loopstart %hu out of bounds %hu", loopStart, frameCount);
		sassert(playbackType <= PlaybackMode::PingPongLoop && playbackType!=PlaybackMode::Invalid, "Bad playback type %lu", playbackType);
		
		for(int frameIndex=0;frameIndex<frameCount;++frameIndex) {
			size_t positionOffset=frameBaseReader.readLELong();
			uint16_t displayTime=frameBaseReader.readLEShort();
			frameBaseReader.skip(2);//padding
			
			positionReader.setPos(positionOffset);
			
			uint16_t cellIndex=positionReader.readLEShort();
			
			auto &frame=anim.frames.emplace_back(cellIndex, displayTime);
			switch(posType) {
				case NNS_G2D_ANIMELEMENT_INDEX:
				break;
				case NNS_G2D_ANIMELEMENT_INDEX_SRT:
					positionReader.skip(2);//rot
					positionReader.skip(4);//scaleX
					positionReader.skip(4);//scaleY
					frame.xOff=positionReader.readLEShort();
					frame.yOff=positionReader.readLEShort();
				break;
				case NNS_G2D_ANIMELEMENT_INDEX_T:
					positionReader.skip(2);//padding
					frame.xOff=positionReader.readLEShort();
					frame.yOff=positionReader.readLEShort();
				break;
				default:
					sassert(0,"Bad posType %hx", posType);
				break;
			}
		}
	}
	
	labels = sections.parseLBAL(animCount);
}
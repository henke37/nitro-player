#include "strm.h"

#include "../binaryReader.h"
#include "../substream.h"

namespace NitroComposer {

	STRM::STRM(const std::string &fileName) : sections(fileName) {
		Parse();
	}

	STRM::STRM(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {
		Parse();
	}

	void STRM::Parse() {
		BinaryReader reader(sections.getSectionData("HEAD"));
		reader.skip(4);
		encoding = (WaveEncoding)reader.readByte();
		loops = reader.readByte() != 0;
		channels = reader.readByte();
		reader.skip(1);
		sampleRate = reader.readLEShort();
		timer = reader.readLEShort();
		loopOffset = reader.readLELong();
		samples = reader.readLELong();
		reader.skip(4);
		blockCount = reader.readLELong();
		blockLen = reader.readLELong();
		blockSamples = reader.readLELong();
		lastBlockLen = reader.readLELong();
		lastBlockSamples = reader.readLELong();
	}

}
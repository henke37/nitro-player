#ifndef NITROCOMPOSER_STRM_H
#define NITROCOMPOSER_STRM_H

namespace NitroComposer {

	class STRM {

	public:
		STRM(const std::string &fileName);
		STRM(std::unique_ptr<BinaryReadStream> stream);
	};

}

#endif
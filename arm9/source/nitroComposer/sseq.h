#ifndef NITROCOMPOSER_SSEQ_H
#define NITROCOMPOSER_SSEQ_H

#include "../sectionedFile.h"

namespace NitroComposer {

	class SSEQ {

	public:
		SSEQ(const std::string &fileName);
		SSEQ(std::unique_ptr<BinaryReadStream> &&stream);

		std::unique_ptr<BinaryReadStream> GetCommandStream() const;
	private:
		SectionedFile sections;
	};

}

#endif
#include "sseq.h"

#include "../fileStream.h"

SSEQ::SSEQ(const std::string &fileName) : sections(fileName) {}

SSEQ::SSEQ(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {}

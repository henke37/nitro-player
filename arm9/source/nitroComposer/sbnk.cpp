#include "sbnk.h"

SBNK::SBNK(const std::string &fileName) : sections(fileName) {}

SBNK::SBNK(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {}

#include "swar.h"

SWAR::SWAR(const std::string &fileName) : sections(fileName) {}

SWAR::SWAR(std::unique_ptr<BinaryReadStream> &&stream) : sections(std::move(stream)) {}

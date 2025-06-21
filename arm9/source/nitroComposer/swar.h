#ifndef NITROCOMPOSER_SWAR_H
#define NITROCOMPOSER_SWAR_H

class SWAR {
public:
	SWAR(const std::string &fileName);
	SWAR(std::unique_ptr<BinaryReadStream> stream);
};

#endif
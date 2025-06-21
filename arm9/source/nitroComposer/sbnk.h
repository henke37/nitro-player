#ifndef NITROCOMPOSER_SBNK_H
#define NITROCOMPOSER_SBNK_H

class SBNK {
public:
	SBNK(const std::string &fileName);
	SBNK(std::unique_ptr<BinaryReadStream> stream);
};

#endif
#ifndef CARDBACKUPSTREAM_H
#define CARDBACKUPSTREAM_H

#include "binaryStream.h"

size_t getBackupSize();
int getBackupChipId();

class CardBackupReadStream : public ManualPosBinaryReadStream {
public:
	CardBackupReadStream();
	~CardBackupReadStream();

	size_t read(uint8_t *buff, size_t size) override;
	size_t getLength() const override;
};

class CardBackupWriteStream : public ManualPosBinaryWriteStream {
public:
	CardBackupWriteStream();
	~CardBackupWriteStream();

	size_t write(const uint8_t *buff, size_t size) override;
	size_t getLength() const override;
};

#endif
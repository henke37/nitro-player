#include "cardBackupStream.h"

#include <nds/card.h>
#include <nds/memory.h>

size_t getBackupSize();
int getBackupChipId();

CardBackupReadStream::CardBackupReadStream() {
}

CardBackupReadStream::~CardBackupReadStream() {}

size_t CardBackupReadStream::read(uint8_t *buff, size_t size) {
	sysSetCardOwner(BUS_OWNER_ARM9);
	cardReadEeprom(position, buff, size, getBackupChipId());
	position += size;
	return size;
}

size_t CardBackupReadStream::getLength() const {
	return getBackupSize();
}

CardBackupWriteStream::CardBackupWriteStream() {
}

CardBackupWriteStream::~CardBackupWriteStream() {}

size_t CardBackupWriteStream::write(const uint8_t *buff, size_t size) {
	sysSetCardOwner(BUS_OWNER_ARM9);
	cardWriteEeprom(position, const_cast<uint8_t *>(buff), size, getBackupChipId());
	position += size;
	return size;
}

size_t CardBackupWriteStream::getLength() const {
	return getBackupSize();
}

size_t getBackupSize() {
	static size_t size;
	if(size) return size;
	sysSetCardOwner(BUS_OWNER_ARM9);
	return size = cardEepromGetSize();
}

int getBackupChipId() {
	static int id;
	if(id) return id;
	sysSetCardOwner(BUS_OWNER_ARM9);
	return id = cardEepromGetType();
}

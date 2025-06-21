#include "hblankBatcher.h"

#include <nds.h>

#include "tcm.h"

#include <cassert>

DTCM_BSS HBlankBatcher hblankBatchers[2];
DTCM_BSS volatile std::uint8_t currentHBlankBatcherReader=0;
DTCM_BSS volatile unsigned int hblankOverruns = 0;

void HBlankBatcher::clear() {
	bulks.clear();
}

void HBlankBatcher::AddBulk(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint8_t[]> &&values) {
	bulks.emplace_back(addr, firstScanline, lastPlusOneScanline, std::move(values));
}
void HBlankBatcher::AddBulk(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint16_t[]> &&values) {
	bulks.emplace_back(addr, firstScanline, lastPlusOneScanline, std::move(values));
}
void HBlankBatcher::AddBulk(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint32_t[]> &&values) {
	bulks.emplace_back(addr, firstScanline, lastPlusOneScanline, std::move(values));
}

void HBlankBatcher::AddBulk(BulkPoke &&bulk) {
	bulks.emplace_back(std::move(bulk));
}

bool HBlankBatcher::isEmpty() const {
	return bulks.empty();
}

void HBlankBatcher::swapBatchers() {
	[[assume(currentHBlankBatcherReader < 2)]];
	currentHBlankBatcherReader = 1 - currentHBlankBatcherReader;

	if(getCurrentReader().isEmpty()) {
		irqDisable(IRQ_HBLANK);
	} else {
		irqEnable(IRQ_HBLANK);
	}
}

HBlankBatcher &HBlankBatcher::getCurrentWriter() {
	assert(currentHBlankBatcherReader < 2);
	return hblankBatchers[1-currentHBlankBatcherReader];
}

ITCM_CODE const HBlankBatcher &HBlankBatcher::getCurrentReader() {
	[[assume(currentHBlankBatcherReader < 2)]];
	return hblankBatchers[currentHBlankBatcherReader];
}

ITCM_CODE void HBlankBatcher::ISR() {
	uint8 startScanline = REG_VCOUNT;
	getCurrentReader().Perform(startScanline);

	if(REG_VCOUNT != startScanline) {
		hblankOverruns = hblankOverruns + 1;
	}
}

ITCM_CODE void HBlankBatcher::Perform(std::uint8_t scanline) const {
	for(const BulkPoke &bulk : bulks) {
		bulk.Perform(scanline);
	}
}

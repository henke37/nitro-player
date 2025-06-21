#ifndef HBLANKBATCHER_H
#define HBLANKBATCHER_H

#include "poke.h"
#include <vector>

class HBlankBatcher {
public:
	HBlankBatcher()=default;
	~HBlankBatcher()=default;
	
	HBlankBatcher(const HBlankBatcher &) = delete;
	void operator=(const HBlankBatcher&) = delete;

	void clear();

	void AddBulk(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint8_t[]> &&values);
	void AddBulk(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint16_t[]> &&values);
	void AddBulk(hwPtr addr, uint8_t firstScanline, uint8_t lastPlusOneScanline, std::unique_ptr<const std::uint32_t[]> &&values);
	void AddBulk(BulkPoke &&bulk);

	bool isEmpty() const;

	static void swapBatchers();
	
	static HBlankBatcher &getCurrentWriter();
	static const HBlankBatcher &getCurrentReader();

	static void ISR();
private:
	void Perform(std::uint8_t scanline) const;

	std::vector<BulkPoke> bulks;
};

#endif
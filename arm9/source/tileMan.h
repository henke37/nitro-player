#ifndef TILEMAN_H
#define TILEMAN_H

#include <nds.h>

#include <vector>

class TileReservationToken;
class TileReservationSlot;

class TileManager {
public:
	TileManager(void *addr, int maxTiles, int bpp);
	TileManager(unsigned int tileBase, unsigned int bigBase, int maxTiles, int bpp);
	~TileManager();
	
	TileManager(const TileManager &)=delete;
	TileManager &operator=(const TileManager &)=delete;
	
	[[nodiscard]] TileReservationToken reserve(int tileCount);

	int getBpp() const { return bpp; }
	void setBpp(int bpp);

	void dump() const;
private:
	friend class TileReservationToken;
	
	std::vector<TileReservationSlot>::iterator findFirstGap(int tileCount);
	
	void unreserve(int startTileIndex);
	
	void *addr;
	int bpp;
	int maxTiles;
	
	std::vector<TileReservationSlot> slots;
};

class TileReservationSlot {
public:
	int startTileIndex;
	int tileCount;
	
	TileReservationSlot(int startTileIndex, int tileCount) : startTileIndex(startTileIndex), tileCount(tileCount) {}
	
	bool operator<(const TileReservationSlot &other) const { return startTileIndex<other.startTileIndex; }
	bool operator>(const TileReservationSlot &other) const  { return startTileIndex>other.startTileIndex; }
	bool operator==(const TileReservationSlot &other) const { return startTileIndex==other.startTileIndex; }
	bool operator!=(const TileReservationSlot &other) const { return startTileIndex!=other.startTileIndex; }
	
	//One past the end of the reservation.
	int endTileIndex() const { return startTileIndex+tileCount; }
};

class TileReservationToken {
public:
	TileReservationToken();
	~TileReservationToken();
	
	TileReservationToken(TileReservationToken &&old);
	TileReservationToken &operator=(TileReservationToken &&old);
	
	[[nodiscard]] int getStartTileIndex() const;
	
	[[nodiscard]] u8 *getDataPtr() const;
	
private:
	TileReservationToken(TileManager *man, int startTileIndex);
	
	TileManager *man;
	int startTileIndex;
	friend class TileManager;
};


#endif

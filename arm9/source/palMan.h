#ifndef PALMAN_H
#define PALMAN_H

#include <nds.h>

class PalReservation;
class FullPalReservation;
class LockedFullPalReservation;

class PalMan256 {
public:
	PalMan256();
	~PalMan256();

	PalMan256(const PalMan256&) = delete;
	PalMan256& operator=(const PalMan256&) = delete;

	virtual unsigned int getSlotCount() const=0;
	virtual unsigned int getPalCount() const = 0;

	[[nodiscard]] virtual FullPalReservation reserveFullPal(unsigned int slot) = 0;
protected:
	friend class FullPalReservation;

	[[nodiscard]] virtual u16* getDataPtr(unsigned int slot) const = 0;
	virtual void acquireLock()=0;
	virtual void releaseLock()=0;
	virtual void releaseSlot(unsigned int slot) = 0;
};

class PalMan : public PalMan256 {
public:
	PalMan(u16 *);
	~PalMan();
	
	PalMan(const PalMan &) = delete;
	PalMan &operator=(const PalMan&)=delete;
	
	[[nodiscard]] PalReservation reserve();
	[[nodiscard]] FullPalReservation reserveFullPal(unsigned int slot) override;

	unsigned int getSlotCount() const override { return 1; }
	unsigned int getPalCount() const override { return 1; }
protected:
	u16* getDataPtr(unsigned int slot) const;

	void acquireLock() override;
	void releaseLock() override;
	void releaseSlot(unsigned int slot) override;
private:
	u16* palAddr;
	bool bigPalMode;
	bool reservations[16];
	
	int findEmptySlot();
	friend class PalReservation;
	friend class FullPalReservation;
};

enum class VRamBank {
	E,
	F,
	G,
	H,
	I
};

class ExPalMan : public PalMan256 {
public:
	ExPalMan(VRAM_E_TYPE mappingMode);
	ExPalMan(VRAM_F_TYPE mappingMode);
	ExPalMan(VRAM_G_TYPE mappingMode);
	ExPalMan(VRAM_H_TYPE mappingMode);
	ExPalMan(VRAM_I_TYPE mappingMode);
	~ExPalMan();

	ExPalMan(const ExPalMan&) = delete;
	ExPalMan& operator=(const ExPalMan&) = delete;

	[[nodiscard]] FullPalReservation reserveFullPal(unsigned int slot) override;

	unsigned int getSlotCount() const override;
	unsigned int getPalCount() const override { return 16; }
protected:
	u16* getDataPtr(unsigned int slot) const;

	void acquireLock() override;
	void releaseLock() override;
	void releaseSlot(unsigned int slot) override;
private:
	bool reservations[4];
	VRamBank bank;
	union {
		VRAM_E_TYPE e;
		VRAM_F_TYPE f;
		VRAM_G_TYPE g;
		VRAM_H_TYPE h;
		VRAM_I_TYPE i;
	} mappingMode;

	void mapLCD();
	void mapPal();

	u16* getLCDAddr() const;

	unsigned int skipedSlotCount() const;

	int lockCount=0;

	friend class FullPalReservation;
};

class PalReservation {
public:
	PalReservation();
	~PalReservation();
	
	void clear();
	
	PalReservation(PalReservation &&);
	PalReservation &operator=(PalReservation &&);
	
	[[nodiscard]] int getSlot() const {return slot;}
	[[nodiscard]] u16 *getDataPtr() const;
	[[nodiscard]] bool isActive() const { return man != nullptr; }

	operator bool() const { return man != nullptr; }
private:
	PalReservation(PalMan *, int slot);
	
	PalMan *man;
	int slot;
	
	friend class PalMan;
};

class FullPalReservation {
public:
	FullPalReservation();
	~FullPalReservation();

	void clear();

	FullPalReservation(FullPalReservation&&);
	FullPalReservation& operator=(FullPalReservation&&);

	bool isActive() const { return man != nullptr; }

	const PalMan256* getMan() const { return man; }
	PalMan256* getMan() { return man; }

	[[nodiscard]] LockedFullPalReservation acquireLock();
private:
	FullPalReservation(PalMan*);
	FullPalReservation(ExPalMan*, unsigned int slot);

	void releaseLock();

	[[nodiscard]] u16* getDataPtr() const;

	PalMan256* man;
	unsigned int slot;

	friend class ExPalMan;
	friend class PalMan;
	friend class LockedFullPalReservation;
};

class LockedFullPalReservation {
public:
	LockedFullPalReservation();
	~LockedFullPalReservation();

	LockedFullPalReservation(LockedFullPalReservation&&);
	LockedFullPalReservation& operator=(LockedFullPalReservation &&);

	void release();

	[[nodiscard]] u16* getDataPtr() const;
private:
	FullPalReservation* reservation;
	LockedFullPalReservation(FullPalReservation*);
	friend class FullPalReservation;
};

extern PalMan mainBgPalMan;
extern PalMan subBgPalMan;
extern PalMan mainObjPalMan;
extern PalMan subObjPalMan;
extern ExPalMan mainBgExPalMan;

#endif
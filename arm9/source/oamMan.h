#ifndef OAMMAN_H
#define OAMMAN_H

#include <vector>
#include <memory>
#include <nds.h>

class OAMManager;

class OAMWriter {
public:
	explicit OAMWriter();
	OAMWriter(OAMManager *manager);
	OAMWriter(OAMWriter const &);
	OAMWriter(OAMWriter &&);
	virtual ~OAMWriter();
	
	OAMWriter &operator=(OAMWriter const &);
	OAMWriter &operator=(OAMWriter &&);
	
	int drawOrder;
	
	virtual int getObjCount() const=0;
	virtual void drawOam(SpriteEntry **out) const=0;
private:
	OAMManager *manager;
};

class OAMManager {
public:
	OAMManager(u16 *oamAddr, unsigned int dmaChannel);
	~OAMManager();
	
	void addWriter(const OAMWriter *writer);
	void removeWriter(const OAMWriter *writer);
	
	void updateOAM();
	
private:
	std::vector<const OAMWriter *> writers;
	
	std::unique_ptr<OAMTable> oamMirror;
	u16 *oamAddr;
	unsigned int dmaChannel;
	
	void scheduleDMA();
	void buildOAMMirror();
};

extern OAMManager mainOamMan;
extern OAMManager subOamMan;

#endif
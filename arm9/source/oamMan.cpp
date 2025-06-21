#include "oamMan.h"

#include <algorithm>

#include <cassert>

OAMWriter::OAMWriter() : manager(nullptr) {
}

OAMWriter::OAMWriter(OAMManager *manager) : manager(manager) {
	assert(manager);
	
	manager->addWriter(this);
}

OAMWriter::OAMWriter(const OAMWriter &old) : manager(old.manager) {
	if(manager) {
		manager->addWriter(this);
	}
}

OAMWriter::OAMWriter(OAMWriter &&old) : manager(old.manager) {
	if(manager) {
		manager->removeWriter(&old);
		old.manager=nullptr;
		manager->addWriter(this);
	}
}

OAMWriter &OAMWriter::operator=(const OAMWriter &old) {
	assert(!manager);
	manager=old.manager;
	if(manager) {
		manager->addWriter(this);
	}
	return *this;
}

OAMWriter &OAMWriter::operator=(OAMWriter &&old) {
	assert(!manager);
	manager=old.manager;
	if(manager) {
		manager->removeWriter(&old);
		old.manager=nullptr;
		manager->addWriter(this);
	}
	return *this;
}

OAMWriter::~OAMWriter() {
	if(manager) {
		manager->removeWriter(this);
	}
}

OAMManager::OAMManager(u16 *oamAddr, unsigned int dmaChannel) : oamAddr(oamAddr), dmaChannel(dmaChannel) {
	assert(dmaChannel<4);
	assert(oamAddr==OAM || oamAddr==OAM_SUB);
	oamMirror=std::make_unique<OAMTable>();
}

OAMManager::~OAMManager() {
}

void OAMManager::addWriter(const OAMWriter *writer) {
	assert(writer);
	writers.push_back(writer);
}
void OAMManager::removeWriter(const OAMWriter *writer) {
	assert(writer);
	auto itr=std::find(writers.begin(), writers.end(), writer);
	assert(itr!=writers.end());
	writers.erase(itr,itr+1);
}

static bool orderWriters(const OAMWriter *a, const OAMWriter *b) {
	return a->drawOrder<b->drawOrder;
}

void OAMManager::updateOAM() {
	std::stable_sort(writers.begin(), writers.end(), orderWriters);

	buildOAMMirror();
	scheduleDMA();
}

void OAMManager::buildOAMMirror() {
	SpriteEntry *out=(SpriteEntry *)oamMirror.get();
	SpriteEntry *outEnd=out+SPRITE_COUNT;
	
	for(auto itr=writers.cbegin();itr!=writers.cend();++itr) {
		auto &elm=*itr;
		
		int slotsLeft=outEnd-out;
		if(elm->getObjCount()>slotsLeft) continue;
		
		elm->drawOam(&out);
	}
	
	for(;out!=outEnd;++out) {
		out->isRotateScale=false;
		out->isHidden=true;
	}
}

void OAMManager::scheduleDMA() {
	OAMTable *mirror=oamMirror.get();
	u16 *source=(u16*)mirror;
	DC_FlushRange(source, sizeof(OAMTable));
	
	assert(!dmaBusy(dmaChannel));
	
	DMA_SRC(dmaChannel) = (uint32)source;
	DMA_DEST(dmaChannel) = (uint32)oamAddr;
	DMA_CR(dmaChannel) = DMA_ENABLE | DMA_16_BIT | DMA_START_VBL | (sizeof(OAMTable)>>1);
}

OAMManager mainOamMan(OAM,1);
OAMManager subOamMan(OAM_SUB,2);
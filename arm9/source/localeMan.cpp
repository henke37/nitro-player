#include "localeMan.h"
#include "globals.h"
#include "fileStream.h"
#include "binaryReader.h"
#include "strutils.h"

#include <nds.h>

LocaleManager localeMan;

void printStr(const std::u16string &str);

std::string LocaleManager::getLocaleId() const {
	switch(PersonalData->language) {
		case 0: return "jp";
		case 1: return "en";
		case 2: return "fr";
		case 3: return "de";
		case 4: return "it";
		case 5: return "es";
		case 6: return "ch";
		default: return "__";
	}
	
}

void LocaleManager::Init() {
	LoadStrings(getLocaleId());
}

void LocaleManager::LoadStrings(const std::string &localeId) {
	FileReadStream fs{std::string("locale/")+localeId+".txt"};
	BinaryReader rdr{&fs,false};
	rdr.skip(2);
	
	while(!rdr.isAtEnd()) {
		std::u16string line=rdr.readLEUTF16Line();
		
		if(line.empty()) continue;
		
		std::u16string::size_type eqPos=line.find(u'=');
		assert(eqPos!=std::u16string::npos);
		std::string key=u16str2str(line.substr(0,eqPos));
		std::u16string val=line.substr(eqPos+1);
		
		strings.emplace(std::move(key),std::move(val));
	}
}

const std::u16string &LocaleManager::getStr(const std::string &key) const {
	auto itr=strings.find(key);
	if(itr==strings.end()) {
		sassert(0,"Bad locale str id \"%s\"!\n", key.c_str());
	}
	return itr->second;
}
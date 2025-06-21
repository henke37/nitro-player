#ifndef LOCALEMAN_H
#define LOCALEMAN_H

#include <string>
#include <unordered_map>

class LocaleManager {
public:
	void Init();
	
	const std::u16string &getStr(const std::string &id) const;
	
	std::string getLocaleId() const;
private:
	void LoadStrings(const std::string &localeId);

	std::unordered_map<std::string,std::u16string> strings;
};

#endif
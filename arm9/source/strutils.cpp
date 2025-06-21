#include "strutils.h"

void printStr(const std::u16string &str) {
	for(auto itr=str.cbegin();itr!=str.cend();++itr) {
		putwc(*itr, stdout);
	}
}

std::string u16str2str(const std::u16string &ustr) {
	std::string str;
	str.reserve(ustr.size());
	
	for(auto itr=ustr.cbegin();itr!=ustr.cend();++itr) {
		str.append(1, (char)*itr);
	}
	return str;
}

std::u16string str2u16str(const std::string &str) {
	std::u16string ustr;
	ustr.reserve(str.size());

	for(auto itr = str.cbegin(); itr != str.cend(); ++itr) {
		ustr.append(1, (char16_t)*itr);
	}
	return ustr;
}

std::u16string int2u16str(int value, unsigned int zeroPadTo) {
	auto wstr = std::to_wstring(value);
	while(wstr.size() < zeroPadTo) wstr = std::wstring(L"0") + wstr;
	return std::u16string(wstr.cbegin(),wstr.cend());
}
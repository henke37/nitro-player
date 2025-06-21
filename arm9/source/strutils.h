#ifndef STRUTILS_H
#define STRUTILS_H

#include <string>

std::string u16str2str(const std::u16string &);
std::u16string str2u16str(const std::string &);
std::u16string int2u16str(int value, unsigned int zeroPadTo = 0);

void printStr(const std::u16string &str);

#endif
#include "testNDS.h"

#include "nitroComposer/sdatFile.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>

TestNDS::TestNDS() {}

TestNDS::~TestNDS() {}

void TestNDS::Unload() {
	ndsFile.reset();
}

void TestNDS::Update() {}

void TestNDS::Load() {
	scanFileSystems();

	puts("That's all!");
}

void TestNDS::scanFileSystems() {
	std::string roots[] = {
		"fat:/",
		"sd:/",
		"nitro:/",
		"nand:/",
		"nand2:/"
	};

	for(const auto &root : roots) {
		int a=access(root.c_str(), 0);
		printf("%s %i\n",root.c_str(),a);
		scanFolder(root);
	}

}

void TestNDS::scanFolder(const std::string &path) {
	auto dir = opendir(path.c_str());
	if(!dir) return;

	struct dirent *entry;
	while((entry = readdir(dir)) != nullptr) {
		std::string fullPath = path + entry->d_name;
		if(entry->d_type == DT_DIR) {
			if(entry->d_name[0] == '.') continue;
			fullPath += "/";
			scanFolder(fullPath);
		} else if(entry->d_type == DT_REG) {
			if(entry->d_name[0] == '.') continue;
			if(!fullPath.ends_with(".nds")) continue;

			printf("%s\n", fullPath.c_str());

			scanNDSFile(fullPath);
		}
	}
	closedir(dir);
}

void TestNDS::scanNDSFile(const std::string &path) {
	ndsFile = std::make_unique<NDSFile>(path);

	auto banner = ndsFile->GetBanner();

	auto enTitle = banner.titles[(int)NDSFile::Banner::TitleLanguage::English];

	// truncate to first line
	size_t newlinePos = enTitle.find(u'\n');
	if(newlinePos != std::u16string::npos) {
		enTitle = enTitle.substr(0, newlinePos);
	}

	// Convert to UTF-8
	std::string enTitleStr = "";
	for(auto ch : enTitle) {
		if(ch == 0) break;
		if(ch < 0x80) {
			enTitleStr += static_cast<char>(ch);
		} else if(ch < 0x800) {
			enTitleStr += static_cast<char>(0xC0 | (ch >> 6));
			enTitleStr += static_cast<char>(0x80 | (ch & 0x3F));
		} else {
			enTitleStr += static_cast<char>(0xE0 | (ch >> 12));
			enTitleStr += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
			enTitleStr += static_cast<char>(0x80 | (ch & 0x3F));
		}
	}

	printf("%s %s\n", ndsFile->getGameCode().c_str(), enTitleStr.c_str());

	for(auto itr = ndsFile->getFileSystemIterator(); !itr.atEnd(); ++itr) {
		if(!itr->name.ends_with(".sdat")) continue;
		printf("%s\n", itr.getFullPath().c_str());

		auto sdatStream = ndsFile->OpenFile(itr->fileId);
		scanSDatFile(std::move(sdatStream));
	}
}

void TestNDS::scanSDatFile(std::unique_ptr<BinaryReadStream> &&sdatStream) {
	auto sdat = std::make_unique<NitroComposer::SDatFile>(std::move(sdatStream));
}

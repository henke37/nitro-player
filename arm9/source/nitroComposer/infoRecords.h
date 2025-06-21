#ifndef NINTROCOMPOSER_INFORECORDS_H
#define NINTROCOMPOSER_INFORECORDS_H

struct SequenceInfoRecord {
    std::uint16_t fatId;
    std::uint16_t bankId;
    std::uint8_t vol;
    std::uint8_t channelPriority;
    std::uint8_t playerPriority;
    std::uint8_t player;
};

struct StreamInfoRecord {
    std::uint16_t fatId;
    std::uint8_t vol;
    std::uint8_t priority;
    std::uint8_t player;
    bool forceStereo;
};

struct BankInfoRecord {
    std::uint16_t fatId;
    std::uint16_t swars[4];
};

struct WaveArchiveInfoRecord {
    std::uint16_t fatId;
};

struct PlayerInfoRecord {
    std::uint32_t heapSize;
    std::uint16_t channelMask;
    std::uint8_t maxSequences;
};

#endif
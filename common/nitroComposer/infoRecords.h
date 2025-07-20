#ifndef NINTROCOMPOSER_INFORECORDS_H
#define NINTROCOMPOSER_INFORECORDS_H

#include <vector>

namespace NitroComposer {

    struct SequenceInfoRecord {
        std::uint16_t fatId;
        std::uint16_t bankId;
        std::uint8_t vol;
        std::uint8_t channelPriority;
        std::uint8_t playerPriority;
        std::uint8_t player;
    };

    struct SequenceArchiveRecord {
        std::uint16_t fatId;
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

    struct GroupInfoRecord {
        enum ElementType {
            Sequence = 0x0700,
            SequenceArchive = 0x0803,
            InstrumentBank = 0x0601,
            WaveArchive = 0x0402
        };
        struct Element {
            ElementType type;
            unsigned int id;
        };
        std::vector<Element> elements;
    };

}

#endif
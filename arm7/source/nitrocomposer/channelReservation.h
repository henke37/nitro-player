#ifndef NITROCOMPOSER_CHANNELRESERVATION_H
#define NITROCOMPOSER_CHANNELRESERVATION_H

#include <cstdint>
#include <cassert>

namespace NitroComposer {

	class ChannelReservation {
	public:
		ChannelReservation() { channel = 0xFF; }
		~ChannelReservation();

		ChannelReservation(const ChannelReservation &) = delete;
		ChannelReservation &operator=(const ChannelReservation &) = delete;

		ChannelReservation(ChannelReservation &&old);
		ChannelReservation &operator=(ChannelReservation &&old);

		operator std::uint8_t() const { return channel; }

		bool IsAllocated() const { return channel != 0xFF; }

		std::uint8_t getChannel() const { assert(channel >= 0 && channel <= 15); return channel; }
	private:
		ChannelReservation(std::uint8_t channel);
		std::uint8_t channel;

		friend class SequencePlayer;
	};
}

#endif
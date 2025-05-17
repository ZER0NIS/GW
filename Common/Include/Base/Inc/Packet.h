#ifndef _FILE_PACKET_H_
#define _FILE_PACKET_H_

#include "ByteBuffer.h"
namespace sbase
{
	namespace msg
	{
		class CPacket : public msg::ByteBuffer
		{
		public:
			CPacket() : ByteBuffer(), m_opcode(0) {}
			CPacket(size_t res) : ByteBuffer(res), m_opcode(0) {}
			CPacket(const CPacket& packet) : ByteBuffer(packet), m_opcode(packet.m_opcode) {}

			void Initialize(uint16 opcode)
			{
				Clear();
				m_opcode = opcode;
			}

			uint16 GetOpcode() const { return m_opcode; }
			void SetOpcode(uint16 opcode) { m_opcode = opcode; }

		protected:
			uint16 m_opcode;
		};
	}
}

#endif
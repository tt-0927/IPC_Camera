#include "rtp-packet.h"
#include "rtp-util.h"
#include <string.h>
#include <assert.h>

#include "rtp_error.h"

// RFC3550 RTP: A Transport Protocol for Real-Time Applications
// 5.1 RTP Fixed Header Fields (p12)
/*
 0               1               2               3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|   CC  |M|     PT      |      sequence number          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                synchronization source (SSRC) identifier       |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|                 contributing source (CSRC) identifiers        |
|                               ....                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

int rtp_packet_deserialize(struct rtp_packet_t *pkt, const void* data, int bytes)
{
	uint32_t i, v;
	int hdrlen;
	const uint8_t *ptr;

	if (bytes < RTP_FIXED_HEADER) // RFC3550 5.1 RTP Fixed Header Fields(p12)
		return RTP_HEADSIZEERROR;
	ptr = (const unsigned char *)data;
	memset(pkt, 0, sizeof(struct rtp_packet_t));

	// pkt header
	v = nbo_r32(ptr);
	pkt->rtp.v = RTP_V(v);
	pkt->rtp.p = RTP_P(v);
	pkt->rtp.x = RTP_X(v);
	pkt->rtp.cc = RTP_CC(v);
	pkt->rtp.m = RTP_M(v);
	pkt->rtp.pt = RTP_PT(v);
	pkt->rtp.seq = RTP_SEQ(v);
	pkt->rtp.timestamp = nbo_r32(ptr + 4);
	pkt->rtp.ssrc = nbo_r32(ptr + 8);

#ifdef DEBUG_ASSERT
	assert(RTP_VERSION == pkt->rtp.v);
#endif
	if(RTP_VERSION != pkt->rtp.v)
	{
		return RTP_VERSIONERROR;
	}

	hdrlen = RTP_FIXED_HEADER + pkt->rtp.cc * 4;
	if (RTP_VERSION != pkt->rtp.v || bytes < hdrlen + (pkt->rtp.x ? 4 : 0) + (pkt->rtp.p ? 1 : 0))
		return RTP_VERSIONERROR;

	// pkt contributing source
	for (i = 0; i < pkt->rtp.cc; i++)
	{
		pkt->csrc[i] = nbo_r32(ptr + 12 + i * 4);
	}

#ifdef DEBUG_ASSERT
	assert(bytes >= hdrlen);
#endif
	pkt->payload = (uint8_t*)ptr + hdrlen;
	pkt->payloadlen = bytes - hdrlen;

	// pkt header extension
	if (1 == pkt->rtp.x)
	{
		const uint8_t *rtpext = ptr + hdrlen;
#ifdef DEBUG_ASSERT
		assert(pkt->payloadlen >= 4);
#endif
		pkt->extension = rtpext + 4;
		pkt->reserved = nbo_r16(rtpext);
		pkt->extlen = nbo_r16(rtpext + 2) * 4;
		if (pkt->extlen + 4 > pkt->payloadlen)
		{
#ifdef DEBUG_ASSERT
			assert(0);
#endif
			return RTP_EXTERNLENERROR;
		}
		else
		{
			pkt->payload = rtpext + pkt->extlen + 4;
			pkt->payloadlen -= pkt->extlen + 4;
		}
	}

	// padding
	if (1 == pkt->rtp.p)
	{
		uint8_t padding = ptr[bytes - 1];
		if (pkt->payloadlen < padding)
		{
#ifdef DEBUG_ASSERT
			assert(0);
#endif
			return RTP_PADDINGERROR;
		}
		else
		{
			pkt->payloadlen -= padding;
		}
	}

	return 0;
}

int rtp_packet_serialize_header(const struct rtp_packet_t *pkt, void* data, int bytes)
{
	int hdrlen;
	uint32_t i;
	uint8_t* ptr;

	if (RTP_VERSION != pkt->rtp.v || 0 != (pkt->extlen % 4))
	{
#ifdef DEBUG_ASSERT
		assert(0); // RTP version field must equal 2 (p66)
#endif
		return RTP_VERSIONERROR;
	}

	// RFC3550 5.1 RTP Fixed Header Fields(p12)
	hdrlen = RTP_FIXED_HEADER + pkt->rtp.cc * 4 + (pkt->rtp.x ? 4 : 0);
	if (bytes < hdrlen + pkt->extlen)
		return RTP_EXTERNLENERROR;

	ptr = (uint8_t *)data;
	nbo_write_rtp_header(ptr, &pkt->rtp);
	ptr += RTP_FIXED_HEADER;

	// pkt contributing source
	for (i = 0; i < pkt->rtp.cc; i++, ptr += 4)
	{
		nbo_w32(ptr, pkt->csrc[i]);
	}

	// pkt header extension
	if (1 == pkt->rtp.x)
	{
		// 5.3.1 RTP Header Extension
#ifdef DEBUG_ASSERT
		assert(0 == (pkt->extlen % 4));
#endif
		nbo_w16(ptr, pkt->reserved);
		nbo_w16(ptr + 2, pkt->extlen / 4);
		memcpy(ptr + 4, pkt->extension, pkt->extlen);
		ptr += pkt->extlen + 4;
	}

	return hdrlen + pkt->extlen;
}

int rtp_packet_serialize(const struct rtp_packet_t *pkt, void* data, int bytes)
{
	int hdrlen;

	hdrlen = rtp_packet_serialize_header(pkt, data, bytes);
	if (hdrlen < RTP_FIXED_HEADER || hdrlen + pkt->payloadlen > bytes)
		return RTP_HEADSIZEERROR;

	memcpy(((uint8_t*)data) + hdrlen, pkt->payload, pkt->payloadlen);
	return hdrlen + pkt->payloadlen;
}

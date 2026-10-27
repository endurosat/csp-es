#include <check.h>
#include <string.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp_id.h"
#include "../include/csp/csp_crc32.h"
#include "../src/csp_dedup.h"

/* Two different FileManager fileOpen requests captured 46 ms apart (CSP 2.0 header,
 * CSP_FCRC32 set, big-endian CRC32 trailer). Hashed together with their trailers they
 * both give 0x5EA3ABB2. */
static const uint8_t frame_cmd290[] = {
	0x00, 0x3A, 0x00, 0x28, 0xCA, 0x01, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x01, 0x00, 0x1F,
	0x2F, 0x73, 0x64, 0x2F, 0x73, 0x74, 0x72, 0x65, 0x73, 0x73, 0x5F, 0x62, 0x75, 0x6C, 0x6B, 0x2F,
	0x73, 0x74, 0x72, 0x65, 0x73, 0x73, 0x5F, 0x30, 0x30, 0x30, 0x31, 0x2E, 0x62, 0x69, 0x6E, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
	0x86, 0x3E, 0x13, 0x45,
};

static const uint8_t frame_cmd296[] = {
	0x00, 0x3A, 0x00, 0x28, 0xCA, 0x01, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x1F,
	0x2F, 0x73, 0x64, 0x2F, 0x73, 0x74, 0x72, 0x65, 0x73, 0x73, 0x5F, 0x62, 0x75, 0x6C, 0x6B, 0x2F,
	0x73, 0x74, 0x72, 0x65, 0x73, 0x73, 0x5F, 0x30, 0x30, 0x30, 0x34, 0x2E, 0x62, 0x69, 0x6E, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
	0xD3, 0x8B, 0xA6, 0x10,
};

/* Received frame -> packet, as an interface hands it to csp_qfifo_write() */
static csp_packet_t * rx_packet(const uint8_t * frame, size_t len) {
	csp_packet_t * packet = csp_buffer_get_always();
	csp_id_setup_rx(packet);
	memcpy(packet->frame_begin, frame, len);
	packet->frame_length = len;
	ck_assert_int_eq(csp_id_strip(packet), 0);
	return packet;
}

static bool is_duplicate(const uint8_t * frame, size_t len) {
	csp_packet_t * packet = rx_packet(frame, len);
	bool dup = csp_dedup_is_duplicate(packet);
	csp_buffer_free(packet);
	return dup;
}

/* The dedup ring is not reset by csp_init(), so every test that feeds a frame twice uses a
 * frame no other test feeds: the tests then also pass when they share a process (CK_FORK=no).
 * The changed byte is in the zero padding of the file name, not in the header. */
#define FRAME_TAG_OFFSET 60

/* Packet as csp_es itself sends it: CRC32 appended over the data only (no CSP_21) */
static bool is_duplicate_csp_es_packet(const char * text) {
	csp_packet_t * packet = csp_buffer_get_always();
	memset(&packet->id, 0, sizeof(packet->id));
	packet->id.pri = CSP_PRIO_NORM;
	packet->id.src = 10;
	packet->id.dst = 5;
	packet->id.dport = 10;
	packet->id.sport = 20;
	packet->id.flags = CSP_FCRC32;
	packet->length = strlen(text);
	memcpy(packet->data, text, packet->length);
	ck_assert_int_eq(csp_crc32_append(packet), CSP_ERR_NONE);

	bool dup = csp_dedup_is_duplicate(packet);
	csp_buffer_free(packet);
	return dup;
}

START_TEST(test_dedup_crc32_packets_with_colliding_trailers_are_new)
{
	csp_init();

	ck_assert(!is_duplicate(frame_cmd290, sizeof(frame_cmd290)));
	ck_assert(!is_duplicate(frame_cmd296, sizeof(frame_cmd296)));
}
END_TEST

START_TEST(test_dedup_identical_crc32_packet_is_duplicate)
{
	uint8_t frame[sizeof(frame_cmd290)];

	csp_init();

	memcpy(frame, frame_cmd290, sizeof(frame));
	frame[FRAME_TAG_OFFSET] = 0x02;

	ck_assert(!is_duplicate(frame, sizeof(frame)));
	ck_assert(is_duplicate(frame, sizeof(frame)));
}
END_TEST

START_TEST(test_dedup_packet_without_crc32_hashes_whole_frame)
{
	uint8_t frame_a[sizeof(frame_cmd290)];
	uint8_t frame_b[sizeof(frame_cmd290)];

	csp_init();

	/* Same frames with CSP_FCRC32 cleared: the last 4 bytes are payload, and frames that
	 * differ only there are different packets */
	memcpy(frame_a, frame_cmd290, sizeof(frame_a));
	frame_a[FRAME_TAG_OFFSET] = 0x03;
	frame_a[5] &= (uint8_t)~CSP_FCRC32;
	memcpy(frame_b, frame_a, sizeof(frame_b));
	frame_b[sizeof(frame_b) - 1] ^= 0xFF;

	ck_assert(!is_duplicate(frame_a, sizeof(frame_a)));
	ck_assert(!is_duplicate(frame_b, sizeof(frame_b)));
	ck_assert(is_duplicate(frame_a, sizeof(frame_a)));
}
END_TEST

START_TEST(test_dedup_csp_es_crc32_packets)
{
	csp_init();

	ck_assert(!is_duplicate_csp_es_packet("dedup native a"));
	ck_assert(!is_duplicate_csp_es_packet("dedup native b"));
	ck_assert(is_duplicate_csp_es_packet("dedup native a"));
}
END_TEST

Suite * dedup_suite(void)
{
	Suite *s;
	TCase *tc_dedup;

	s = suite_create("Dedup");

	tc_dedup = tcase_create("is_duplicate");
	tcase_add_test(tc_dedup, test_dedup_crc32_packets_with_colliding_trailers_are_new);
	tcase_add_test(tc_dedup, test_dedup_identical_crc32_packet_is_duplicate);
	tcase_add_test(tc_dedup, test_dedup_packet_without_crc32_hashes_whole_frame);
	tcase_add_test(tc_dedup, test_dedup_csp_es_crc32_packets);
	suite_add_tcase(s, tc_dedup);

	return s;
}

#include <check.h>
#include <string.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp_iflist.h"
#include "../include/csp/csp_rtable.h"
#include "../src/csp_io.h"

/* --- Dummy nexthops ------------------------------------------------------- */

static int dummy_a_tx_count;
static int dummy_b_tx_count;

static int dummy_nexthop_a(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
	(void)iface; (void)via; (void)from_me;
	dummy_a_tx_count++;
	csp_buffer_free(packet);
	return CSP_ERR_NONE;
}

static int dummy_nexthop_b(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
	(void)iface; (void)via; (void)from_me;
	dummy_b_tx_count++;
	csp_buffer_free(packet);
	return CSP_ERR_NONE;
}

static csp_iface_t iface_a;
static csp_iface_t iface_b;

/* --- Helpers -------------------------------------------------------------- */

static void setup(void) {
	csp_init();
	dummy_a_tx_count = 0;
	dummy_b_tx_count = 0;

	/* iface_a: addr 1, subnet 0.x (top 8 bits = network) */
	memset(&iface_a, 0, sizeof(iface_a));
	iface_a.name    = "DUMMY_A";
	iface_a.addr    = 1;
	iface_a.netmask = 8;
	iface_a.nexthop = dummy_nexthop_a;

	/* iface_b: addr 257 (0x0101), subnet 1.x */
	memset(&iface_b, 0, sizeof(iface_b));
	iface_b.name    = "DUMMY_B";
	iface_b.addr    = 257;
	iface_b.netmask = 8;
	iface_b.nexthop = dummy_nexthop_b;

	csp_iflist_add(&iface_a);
	csp_iflist_add(&iface_b);
}

static void teardown(void) {
	csp_rtable_clear();
	csp_iflist_remove(&iface_a);
	csp_iflist_remove(&iface_b);
}

/* Locally-originated send (from_me == 1): routed_from == NULL */
static void send_local(uint16_t dst) {
	csp_packet_t * pkt = csp_buffer_get_always();
	pkt->length = 0;
	csp_id_t id;
	memset(&id, 0, sizeof(id));
	id.dst = dst;
	id.pri = CSP_PRIO_NORM;
	csp_send_direct(&id, pkt, NULL);
}

/* Transit/forwarded send (from_me == 0): routed_from == incoming iface */
static void send_transit(uint16_t dst, csp_iface_t * routed_from) {
	csp_packet_t * pkt = csp_buffer_get_always();
	pkt->length = 0;
	csp_id_t id;
	memset(&id, 0, sizeof(id));
	id.dst = dst;
	id.pri = CSP_PRIO_NORM;
	csp_send_direct(&id, pkt, routed_from);
}

/* --- Tests ---------------------------------------------------------------- */

/* Both interfaces enabled — packet routed to the matching interface. */
START_TEST(test_enabled_routes_packet)
{
	send_local(2);   /* dest 2 is on subnet 0.x — should reach iface_a */
	ck_assert_int_eq(dummy_a_tx_count, 1);
	ck_assert_int_eq(dummy_b_tx_count, 0);
}
END_TEST

/* (a) Locally-originated traffic (from_me) STILL egresses on a routing-disabled
 *     interface, so the node can answer requests addressed to itself. */
START_TEST(test_local_reply_succeeds_on_disabled_iface)
{
	csp_iflist_set_routing_enabled(&iface_a, false);
	send_local(2);                                   /* from_me == 1 */
	ck_assert_int_eq(dummy_a_tx_count, 1);
	ck_assert_int_eq(dummy_b_tx_count, 0);
}
END_TEST

/* (b) Transit/forwarded traffic (from_me == 0) is dropped on a routing-disabled
 *     interface reached via the local-subnet lookup, and increments drop. */
START_TEST(test_transit_blocked_on_disabled_iface)
{
	csp_iflist_set_routing_enabled(&iface_a, false);
	uint32_t drop_before = iface_a.drop;
	send_transit(2, &iface_b);                       /* from_me == 0, arrived on iface_b */
	ck_assert_int_eq(dummy_a_tx_count, 0);
	ck_assert_int_gt((int)iface_a.drop, (int)drop_before);
}
END_TEST

/* (b') Transit reaching a disabled interface via the routing table is dropped
 *      and increments drop. (dst 768 is off both interface subnets, so it can
 *      only be reached through the routing-table entry.) */
START_TEST(test_transit_blocked_via_rtable_increments_drop)
{
	csp_rtable_set(768, 8, &iface_a, CSP_NO_VIA_ADDRESS);
	csp_iflist_set_routing_enabled(&iface_a, false);
	uint32_t drop_before = iface_a.drop;
	send_transit(768, &iface_b);                     /* from_me == 0 */
	ck_assert_int_eq(dummy_a_tx_count, 0);
	ck_assert_int_gt((int)iface_a.drop, (int)drop_before);
}
END_TEST

/* (c) Transit to a subnet served ONLY by a routing-disabled interface must still
 *     fall through to a routing-table route — regression guard ensuring the
 *     local-subnet loop does not set local_found for the skipped interface and
 *     early-return before the rtable/default fallback. */
START_TEST(test_transit_falls_back_when_subnet_iface_disabled)
{
	/* Fallback route for dst 2 via iface_b */
	csp_rtable_set(2, 8, &iface_b, CSP_NO_VIA_ADDRESS);
	csp_iflist_set_routing_enabled(&iface_a, false);

	/* Incoming from a third subnet so split-horizon does not skip iface_b */
	csp_iface_t routed_src;
	memset(&routed_src, 0, sizeof(routed_src));
	routed_src.name    = "SRC";
	routed_src.addr    = 513;   /* subnet 2.x */
	routed_src.netmask = 8;

	send_transit(2, &routed_src);                    /* from_me == 0, dst on disabled iface_a's subnet */
	ck_assert_int_eq(dummy_a_tx_count, 0);           /* not via disabled iface_a */
	ck_assert_int_eq(dummy_b_tx_count, 1);           /* fell back to iface_b via rtable */
}
END_TEST

/* Re-enable iface_a — routing restored (transit works again). */
START_TEST(test_reenable_restores_routing)
{
	csp_iflist_set_routing_enabled(&iface_a, false);
	csp_iflist_set_routing_enabled(&iface_a, true);
	send_transit(2, &iface_b);
	ck_assert_int_eq(dummy_a_tx_count, 1);
}
END_TEST

/* Disabling one interface does not affect the other. */
START_TEST(test_disable_one_does_not_affect_other)
{
	csp_iflist_set_routing_enabled(&iface_a, false);
	send_transit(258, &iface_a);   /* dest 258 is on subnet 1.x — should reach iface_b */
	ck_assert_int_eq(dummy_b_tx_count, 1);
	ck_assert_int_eq(dummy_a_tx_count, 0);
}
END_TEST

/* NULL iface passed to setter is a no-op (must not crash). */
START_TEST(test_null_iface_noop)
{
	csp_iflist_set_routing_enabled(NULL, false);  /* must not crash */
	send_local(2);
	ck_assert_int_eq(dummy_a_tx_count, 1);
}
END_TEST

Suite * routing_enable_disable_suite(void)
{
	Suite * s = suite_create("Routing Enable/Disable");
	TCase * tc = tcase_create("csp_iflist_set_routing_enabled");
	tcase_add_checked_fixture(tc, setup, teardown);
	tcase_add_test(tc, test_enabled_routes_packet);
	tcase_add_test(tc, test_local_reply_succeeds_on_disabled_iface);
	tcase_add_test(tc, test_transit_blocked_on_disabled_iface);
	tcase_add_test(tc, test_transit_blocked_via_rtable_increments_drop);
	tcase_add_test(tc, test_transit_falls_back_when_subnet_iface_disabled);
	tcase_add_test(tc, test_reenable_restores_routing);
	tcase_add_test(tc, test_disable_one_does_not_affect_other);
	tcase_add_test(tc, test_null_iface_noop);
	suite_add_tcase(s, tc);
	return s;
}

#ifndef LIGHTNING_PLUGINS_ASKRENE_CHILD_CHILD_H
#define LIGHTNING_PLUGINS_ASKRENE_CHILD_CHILD_H
#include "config.h"
#include <ccan/compiler/compiler.h>
#include <ccan/short_types/short_types.h>
#include <ccan/time/time.h>
#include <common/amount.h>
#include <common/fp16.h>
#include <stdbool.h>

struct additional_cost_htable;
struct gossmap;
struct json_filter;
struct layer;
struct reserve_htable;

/* This is the child.  Do the thing.
 *
 * `circular` indicates that the parent spliced a fake destination
 * node into the gossmap to support source == destination
 * (self-rebalance) routing.  After flow conversion, the trailing
 * (peer -> fake_us_in) hop is stripped from each returned route so
 * the caller sees an open-cycle path ending at peer. */
void run_child(const struct gossmap *gossmap,
	       const struct layer **layers,
	       const s8 *biases,
	       const struct additional_cost_htable *additional_costs,
	       struct reserve_htable *reserved,
	       fp16_t *capacities TAKES,
	       bool single_path,
	       struct timemono deadline,
	       const struct gossmap_node *srcnode,
	       const struct gossmap_node *dstnode,
	       struct amount_msat amount, struct amount_msat maxfee,
	       u32 finalcltv, u32 maxdelay, size_t maxparts,
	       bool include_fees,
	       bool circular,
	       const char *cmd_id,
	       struct json_filter *cmd_filter,
	       int reply_fd) NORETURN;

#endif /* LIGHTNING_PLUGINS_ASKRENE_CHILD_CHILD_H */


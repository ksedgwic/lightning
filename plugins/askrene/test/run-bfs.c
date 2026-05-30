#include "config.h"
#include <assert.h>
#include <ccan/tal/tal.h>
#include <common/setup.h>
#include <inttypes.h>
#include <plugins/askrene/child/graph.h>
#include <stdio.h>

#define ASKRENE_UNITTEST
#include "../child/algorithm.c"

#define MAX_NODES 256
#define MAX_ARCS 256
#define DUAL_BIT 7

#define CHECK(arg) if(!(arg)){fprintf(stderr, "failed CHECK at line %d: %s\n", __LINE__, #arg); abort();}

static void show(struct graph *graph, struct node node)
{
	printf("Showing node %" PRIu32 "\n", node.idx);
	for (struct arc arc = node_adjacency_begin(graph, node);
	     !node_adjacency_end(arc); arc = node_adjacency_next(graph, arc)) {
		printf("arc id: %" PRIu32 ", (%" PRIu32 " -> %" PRIu32 ")\n",
		       arc.idx, arc_tail(graph, arc).idx,
		       arc_head(graph, arc).idx);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	common_setup(argv[0]);
	printf("Allocating a memory context\n");
	tal_t *ctx = tal(NULL, tal_t);
	assert(ctx);

	printf("Allocating a graph\n");
	struct graph *graph = graph_new(ctx, MAX_NODES, MAX_ARCS, DUAL_BIT);
	assert(graph);

	s64 *capacity = tal_arrz(ctx, s64, MAX_ARCS);
	struct arc *prev = tal_arr(ctx, struct arc, MAX_NODES);

	graph_add_arc(graph, arc_obj(0), node_obj(1), node_obj(2));
	capacity[0] = 1;
	graph_add_arc(graph, arc_obj(1), node_obj(1), node_obj(3));
	capacity[1] = 1;
	graph_add_arc(graph, arc_obj(2), node_obj(1), node_obj(6));
	capacity[2] = 1;
	graph_add_arc(graph, arc_obj(3), node_obj(2), node_obj(3));
	capacity[3] = 1;
	graph_add_arc(graph, arc_obj(4), node_obj(2), node_obj(4));
	capacity[4] = 0; /* disable this arc */
	graph_add_arc(graph, arc_obj(5), node_obj(3), node_obj(4));
	capacity[5] = 1;
	graph_add_arc(graph, arc_obj(6), node_obj(3), node_obj(6));
	capacity[6] = 1;
	graph_add_arc(graph, arc_obj(7), node_obj(4), node_obj(5));
	capacity[7] = 1;
	graph_add_arc(graph, arc_obj(8), node_obj(5), node_obj(6));
	capacity[8] = 1;

	show(graph, node_obj(1));
	show(graph, node_obj(2));
	show(graph, node_obj(3));
	show(graph, node_obj(4));
	show(graph, node_obj(5));
	show(graph, node_obj(6));

	struct node src = {.idx = 1};
	struct node dst = {.idx = 5};

	bool result = BFS_path(ctx, graph, src, dst, capacity, 1, prev);
	assert(result);

	int pathlen = 0;
	int arc_sequence[] = {7, 5, 1};
	int node_sequence[] = {4, 3, 1};

	printf("path: ");
	for (struct node cur = dst; cur.idx != src.idx;) {
		struct arc arc = prev[cur.idx];
		printf("node(%" PRIu32 ") arc(%" PRIu32 ") - ", cur.idx,
		       arc.idx);
		cur = arc_tail(graph, arc);
		CHECK(pathlen < 3);
		CHECK(cur.idx == node_sequence[pathlen]);
		CHECK(arc.idx == arc_sequence[pathlen]);
		pathlen ++;
	}
	CHECK(pathlen == 3);
	printf("node(%" PRIu32 ") arc(NONE)\n", src.idx);
	printf("path length: %d\n", pathlen);

	/* ---------- circular BFS (src == dst) ---------- *
	 *
	 * Build a fresh triangle graph:
	 *     10:   11 -> 12
	 *     11:   12 -> 13
	 *     12:   13 -> 11
	 *
	 * Run BFS with src == dst == 11.  The only viable closed walk is
	 * 11 -> 12 -> 13 -> 11.  Pre-patch, BFS returned a zero-length
	 * "path" because cur.idx == destination.idx matched on the very
	 * first iteration with prev[cur] still INVALID.  Post-patch the
	 * destination check requires prev[cur] != INVALID, so BFS keeps
	 * searching until an arc terminates at source and prev[source]
	 * gets set.
	 */
	printf("\nCircular BFS test (src == dst)\n");

	struct graph *cgraph = graph_new(ctx, MAX_NODES, MAX_ARCS, DUAL_BIT);
	assert(cgraph);
	s64 *ccap = tal_arrz(ctx, s64, MAX_ARCS);
	struct arc *cprev = tal_arr(ctx, struct arc, MAX_NODES);

	graph_add_arc(cgraph, arc_obj(10), node_obj(11), node_obj(12));
	ccap[10] = 1;
	graph_add_arc(cgraph, arc_obj(11), node_obj(12), node_obj(13));
	ccap[11] = 1;
	graph_add_arc(cgraph, arc_obj(12), node_obj(13), node_obj(11));
	ccap[12] = 1;

	struct node csrc = {.idx = 11};
	struct node cdst = {.idx = 11};

	bool cresult = BFS_path(ctx, cgraph, csrc, cdst, ccap, 1, cprev);
	CHECK(cresult);

	/* Walk the cycle backwards from destination.  do-while form
	 * because src == dst -- the first iteration must execute. */
	int carc_sequence[] = {12, 11, 10};
	int cnode_sequence[] = {13, 12, 11};
	int cpathlen = 0;
	printf("circular path: ");
	struct node cur_node = cdst;
	do {
		struct arc carc = cprev[cur_node.idx];
		printf("node(%" PRIu32 ") arc(%" PRIu32 ") - ",
		       cur_node.idx, carc.idx);
		cur_node = arc_tail(cgraph, carc);
		CHECK(cpathlen < 3);
		CHECK(cur_node.idx == cnode_sequence[cpathlen]);
		CHECK(carc.idx == carc_sequence[cpathlen]);
		cpathlen++;
	} while (cur_node.idx != csrc.idx);
	CHECK(cpathlen == 3);
	printf("node(%" PRIu32 ") arc(START)\n", csrc.idx);
	printf("circular path length: %d\n", cpathlen);

	printf("Freeing memory\n");
	ctx = tal_free(ctx);

	common_shutdown();
	return 0;
}


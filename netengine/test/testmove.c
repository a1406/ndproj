#include <stdio.h>
#include "ne_common/ne_os.h"
#include "ne_common/ne_rbtree.h"
#include "ne_quadtree/quadtree.h"

#define QUAD_TEST_NUM (2000000)

const int MAP_LENGTH = 100;
const int NPC_VIEW_LENGTH = 10;
typedef struct npc_
{
	struct list_head quad_lst;
	int pos_x, pos_y;
	char name[32];
	int id;
} npc;

static rb_node_t* name_map;
static rb_node_t *id_map;
static ne_rwlock map_lock;
static quadtree_t *map_tree;
static int id_index;

void printnpc(data_t t, void *param)
{
	if (!t)
		return;
	printf("name[%s] id[%d] x[%d] y[%d]\n",
		((npc *)t)->name, ((npc *)t)->id, ((npc *)t)->pos_x, ((npc *)t)->pos_y);
}

void printnpc_v2(data_t t, void *param)
{
	npc *data;
	if (!t)
		return;
	data = container_of(t, npc, name);
	printnpc(data, NULL);
}

void printnpc_v3(struct list_head *l, void *param)
{
	struct list_head *n;
	npc *data;
	if (!l)
		return;
	n = l->next;
	while (n != l) {
		data = (npc *)( (char *)n - offsetof(npc,quad_lst) );
		printnpc(data, NULL);	
		n = n->next;
	}

		//	data = (npc *)( (char *)l - offsetof(npc,quad_lst) );
//	data = container_of(l, npc, quad_lst);
}

void dumpallnpc()
{
	char c;
	printf("a: dump by id_map\n");
	printf("b: dump by name_map\n");
	printf("c: dump by quad_tree\n");	
	printf("default: dump all\n");

	while (c = getchar()) {
		if (c == '\n')
			continue;
		break;
	}
	switch (c)
	{
		case 'a':
			rb_travel(id_map, printnpc, NULL);
			break;
		case 'b':
			rb_travel(name_map, printnpc_v2, NULL);			
			break;
		case 'c':
			quad_travel(map_tree->_root, printnpc_v3, NULL);			
			break;
		default:
			rb_travel(id_map, printnpc, NULL);
			rb_travel(name_map, printnpc_v2, NULL);
			quad_travel(map_tree->_root, printnpc_v3, NULL);			
			break;
	}
}

int addnpc(char *name, int id, int pos_x, int pos_y)
{
	quadbox_t pos_box = {pos_x, pos_y, pos_x + 2, pos_y + 2};
	npc *p = (npc *)malloc(sizeof(npc));
	if (!p)
		return (-1);

	p->id = id;
	p->pos_x = pos_x;
	p->pos_y = pos_y;
	strncpy(p->name, name, sizeof(p->name));
	p->name[sizeof(p->name)-1] = '\0';
	
	id_map = rb_insert(id, p, id_map);
	name_map = rb_insert_v2(sizeof(p->name), p->name, name_map);
	if (quadtree_insert(map_tree, &p->quad_lst, &pos_box) == NULL)
	{
		printf("add to map fail\n");
	}
		
	return (0);
}

int movenpc(int id, int pos_x, int pos_y)
{
	quadbox_t pos_box = {pos_x, pos_y, pos_x + 2, pos_y + 2};
	npc *p;
	rb_node_t *rb_ret;

	rb_ret = rb_search(id, id_map);
	if (!rb_ret || !rb_ret->data)
		return (-1);
	p = (npc *)rb_ret->data;
	p->pos_x = pos_x;
	p->pos_y = pos_y;
	if (quadtree_move(map_tree, &p->quad_lst, &pos_box) == NULL)
	{
		printf("add to map fail\n");
	}
	return (0);
}

void searchnpc()
{
	char name[32];
	int x, y, xmax, ymax;
	char c;
	struct list_head *quad_ret[100];
	int index = 0;
	quadbox_t box;
	rb_node_t *rb_ret;
	printf("a id: search by id_map\n");
	printf("b name: search by name_map\n");
	printf("c x xmax y ymax: search by quad_tree\n");	

	while (c = getchar()) {
		if (c == '\n')
			continue;
		break;
	}
	switch (c)
	{
		case 'a':
			scanf("%d", &x);
			rb_ret = rb_search(x, id_map);
			if (rb_ret)
				printnpc(rb_ret->data, NULL);			
			break;
		case 'b':
			scanf("%s", name);
			rb_ret = rb_search_v2(name, name_map);
			if (rb_ret)
				printnpc_v2(rb_ret->data, NULL);
			break;
		case 'c':
			scanf("%d %d %d %d", &x, &xmax, &y, &ymax);
			box._xmin = x; box._xmax = xmax; box._ymin = y; box._ymax = ymax;
			quadtree_search(map_tree, &box, quad_ret, &index, 100);
			for (x = 0; x < index; ++x)
			{
				printnpc_v3(quad_ret[x], NULL);
			}
			break;
		default:
			break;
	}
}

static npc *testnpc;
void test_quadtree2()
{
	int i, index;
	quadbox_t box;	
	struct list_head *retlist[10];	
	for (i = 0; i < QUAD_TEST_NUM; ++i) {
		ne_rdlock(&map_lock);
		index = 0;
		box._xmin = testnpc[i].pos_x;
		box._ymin = testnpc[i].pos_y;
		box._xmax = box._xmin + 3;
		box._ymax = box._ymin + 3;		
		quadtree_search(map_tree, &box, retlist, &index, 10);
		ne_rwunlock(&map_lock);		
	}
}

void test_quadtree()
{
	int i;
	quadbox_t box;
	NEINT16 seed[3];
	
	memset(testnpc, 0, sizeof(npc) * QUAD_TEST_NUM);
	seed[1] = getpid() & 0xFFFF;
	seed[2] = (time((time_t *)0) >> 16) & 0xFFFF;
	(void)seed48( seed );

	for (i = 0; i < QUAD_TEST_NUM; ++i) {
		ne_wrlock(&map_lock);
		box._xmin = lrand48() % (MAP_LENGTH - 10);
		box._ymin = lrand48() % (MAP_LENGTH - 10);
		box._xmax = box._xmin + 2;
		box._ymin = box._ymin + 2;
		testnpc[i].pos_x = box._xmin;
		testnpc[i].pos_y = box._ymin;		
		quadtree_insert(map_tree, &testnpc[i].quad_lst, &box);
		ne_rwunlock(&map_lock);		
	}
}

void show_npc_view(int id)
{
	npc *p;
	rb_node_t *rb_ret;
	quadbox_t box;
	struct list_head *quad_ret[100];
	int index = 0;
	
	rb_ret = rb_search(id, id_map);
	if (!rb_ret || !rb_ret->data)
		return;
	p = rb_ret->data;
	box._xmin = p->pos_x - NPC_VIEW_LENGTH;
	box._xmax = p->pos_x + 2 +  NPC_VIEW_LENGTH;
	box._ymin = p->pos_y - NPC_VIEW_LENGTH;
	box._ymax = p->pos_y + 2 + NPC_VIEW_LENGTH;
	quadtree_search(map_tree, &box, quad_ret, &index, 100);
	for (id = 0; id < index; ++id)
	{
		printnpc_v3(quad_ret[id], NULL);
	}
}

void addnpc_bytxt(char *name)
{
	char npcname[32];
	int x, y;
	FILE *f = fopen(name, "r");
	if (!name) {
		printf("can not open file %s\n", name);
		return;
	}
	while (fscanf(f, "%s %d %d", npcname, &x, &y) == 3) {
		addnpc(npcname, ++id_index, x, y);
	}
}

void usage()
{
	printf("i: add npc from txt\n");
	printf("a: add npc\n");
	printf("s: search npc\n");
	printf("q: exit\n");
	printf("d: dump all npc info\n");
	printf("m: move npc pos\n");
	printf("v: show npc's view\n");
	printf("t: test quadtree\n");
}

int main(int argc, char *argv[])
{
	char c;
	char name[32];
	int id;
	int pos_x, pos_y;

	quadbox_t mapbox = {0,0,MAP_LENGTH,MAP_LENGTH};
	map_tree = quadtree_create(mapbox, 5, 0.1);
	ne_rwlock_init(&map_lock);

	testnpc = malloc(QUAD_TEST_NUM * sizeof(npc));
	test_quadtree();
	test_quadtree2();	

	return (0);
	while (c = getchar()) {
		switch(c)
		{
			case 'i':
				printf("input txt = ?\n");
				scanf("%s", name);
				addnpc_bytxt(name);
				break;
			case 'a':
				printf("name = ?, pos_x = ?, pos_y = ?\n");
				scanf("%s %d %d", name, &pos_x, &pos_y);
				addnpc(name, ++id_index, pos_x, pos_y);
				break;
			case 's':
				searchnpc();
				break;
			case 'q':
				printf("exit\n");
				exit(0);
			case 'd':
				dumpallnpc();
				break;
			case 'm':
				printf("id = ?, pos_x = ?, pos_y = ?\n");
				scanf("%d %d %d", &id, &pos_x, &pos_y);
				movenpc(id, pos_x, pos_y);
				break;
			case 'v':
				printf("id = ?\n");
				scanf("%d", &id);
				show_npc_view(id);
				break;
			case 't':
				test_quadtree();
				break;
			case '\n':
				break;
			default:
				usage();
			break;
		}
	}
	return (0);
}

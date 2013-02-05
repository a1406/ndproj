#include "ne_common/ne_os.h"
#include "ne_common/ne_common.h"

/* Creates and returns a new threaded binary tree.  Returns a null
   pointer if a memory allocation error occurs. */
struct tbin_tree *tbin_init(struct tbin_tree *tree,	alloc_tbin_node fa,	free_tbin_node ff)
{
	if (tree == NULL)
		return NULL;
	tree->root = NULL;
	tree->count = 0;
	tree->alloc_node = fa ;
	tree->free_node = ff ;
	return tree;
}

/* Searches TREE for matching ITEM.  Returns 1 if found, 0
   otherwise. */
struct tbin_node* tbin_search(const struct tbin_tree *tree, TTREE_DATA item)
{
	struct tbin_node *node;

	ne_assert(tree != NULL);

	node = tree->root;
	if (node == NULL)
		return 0;

	for (;;) {
		if (item == node->data)
			return node;
		else if (item > node->data) {
			if (node->r_thread == 0)
				node = node->right;
			else
				return 0;
		}
		else {
			if (node->l_thread == 0)
				node = node->left;
			else
				return 0;
		}
	}
}

/* Allocates a new node in TREE.  Sets the node's data to ITEM and
   initializes the other fields appropriately. */
static struct tbin_node *new_node(struct tbin_tree *tree,
				  TTREE_DATA item)
{
  //struct tbin_node *node = malloc(sizeof *node);
	struct tbin_node *node = tree->alloc_node() ;
	if (node == NULL)
		return NULL;

	node->data = item;
	tree->count++;
	return node;
}

/* Inserts ITEM into TREE.  Returns new node of inserted if the item was inserted, NULL if
   an identical item already existed in TREE, or  if a memory
   allocation error occurred. */
struct tbin_node* tbin_insert(struct tbin_tree *tree, TTREE_DATA item)
{
	struct tbin_node *z, *y;

	ne_assert(tree != NULL);
	ne_assert(tree->alloc_node) ;
	if(!tree->alloc_node) {
		return NULL ;
	}
	z = tree->root;
	if (z == NULL) {
		y = tree->root = new_node(tree, item);
		if (y == NULL)
			return 0;

		y->left = y->right = NULL;
		y->l_thread = y->r_thread = 1;
		return y;
	}

	for (;;) {
		if (item == z->data)
			return NULL;
		else if (item > z->data) {
			if (z->r_thread == 1) {
				y = new_node(tree, item);
				if (y == NULL)
					return 0;

				y->right = z->right;
				y->r_thread = 1;
				z->right = y;
				z->r_thread = 0;
				y->left = z;
				y->l_thread = 1;

				return y;
			}

			z = z->right;
		}
		else {
			if (z->l_thread == 1) {
				y = new_node(tree, item);
				if (y == NULL)
					return 0;

				y->left = z->left;
				y->l_thread = 1;
				z->left = y;
				z->l_thread = 0;
				y->right = z;
				y->r_thread = 1;

				return y;
			}

			z = z->left;
		}	//end else
	}		//end for
}

/* 在树中插入一个节点，不需要分配空间
 */
struct tbin_node* tbin_insert_node(struct tbin_tree *tree, struct tbin_node *node)
{
	TTREE_DATA item = node->data;
	struct tbin_node *z, *y;

	ne_assert(tree != NULL);

	z = tree->root;
	if (z == NULL) {
		y = tree->root = node ;
		//if (y == NULL)
		//	return 0;

		y->left = y->right = NULL;
		y->l_thread = y->r_thread = 1;
		tree->count++ ;
		return y;
	}

	for (;;) {
		if (item == z->data)
			return NULL;
		else if (item > z->data) {
			if (z->r_thread == 1) {
				y = node ;
				//if (y == NULL)
				//	return 0;

				y->right = z->right;
				y->r_thread = 1;
				z->right = y;
				z->r_thread = 0;
				y->left = z;
				y->l_thread = 1;
				tree->count++ ;
				return y;
			}

			z = z->right;
		}
		else {
			if (z->l_thread == 1) {
				y = node ;
				//if (y == NULL)
				//	return 0;

				y->left = z->left;
				y->l_thread = 1;
				z->left = y;
				z->l_thread = 0;
				y->right = z;
				y->r_thread = 1;

				tree->count++ ;
				return y;
			}

			z = z->left;
		}	//end else
	}		//end for

}

NEINT32 tbin_delete_node(struct tbin_tree *tree, struct tbin_node *node)
{
	TTREE_DATA item = node->data ;
	struct tbin_node **q, *s, *t;

	ne_assert(tree != NULL);

	t = tree->root;
	if (t == NULL)
		return 0;
	s = NULL;

	for (;;) {
		if (item == t->data)
			break;
		else if (item > t->data) {
			if (t->r_thread == 1)
				return 0;
			s = t;
			t = t->right;
		}
		else {
			if (t->l_thread == 1)
				return 0;
			s = t;
			t = t->left;
		}
	}

	if (s == NULL)
		q = &tree->root;
	else if (item > s->data)
		q = &s->right;
	else
		q = &s->left;

	if (t->l_thread == 0 && t->r_thread == 1) {
		struct tbin_node *r = t->left;
		while (r->r_thread == 0)
			r = r->right;
		if (r->right == t)
			r->right = t->right;

		*q = t->left;
	}
	else if (t->r_thread == 1) {
		if (s == NULL)
			*q = NULL;
		else if (item > s->data) {
			s->r_thread = 1;
			*q = t->right;
		}
		else {
			s->l_thread = 1;
			*q = t->left;
		}
	}
	else {
		struct tbin_node *r = t->right;
		if (r->l_thread == 1) {
			r->left = t->left;
			r->l_thread = t->l_thread;
			if (r->l_thread == 0) {
				struct tbin_node *p = r->left;
				while (p->r_thread == 0)
					p = p->right;
				p->right = r;
			}
			*q = r;
		}
		else {
			struct tbin_node *p = r->left;
			while (p->l_thread == 0) {
				r = p;
				p = r->left;
			}
	
			t->data = p->data;
			if (p->r_thread == 1) {
				r->l_thread = 1;
				r->left = t;
			}
			else {
				s = r->left = p->right;
				while (s->l_thread == 0)
					s = s->left;
				s->left = t;
			}
			t = p;
		}
	}

	//tree->free_node(t);
	//free(t);
	tree->count--;
	return 1;
}

/* Deletes any item matching ITEM from TREE.  Returns 1 if such an
   item was deleted, 0 if none was found. */
NEINT32 tbin_delete(struct tbin_tree *tree, TTREE_DATA item)
{
	struct tbin_node **q, *s, *t;

	ne_assert(tree != NULL);
	
	ne_assert(tree->free_node) ;
	if(!tree->free_node) {
		return 0 ;
	}
	t = tree->root;
	if (t == NULL)
		return 0;
	s = NULL;

	for (;;) {
		if (item == t->data)
			break;
		else if (item > t->data) {
			if (t->r_thread == 1)
				return 0;
			s = t;
			t = t->right;
		}
		else {
			if (t->l_thread == 1)
				return 0;
			s = t;
			t = t->left;
		}
	}

	if (s == NULL)
		q = &tree->root;
	else if (item > s->data)
		q = &s->right;
	else
		q = &s->left;

	if (t->l_thread == 0 && t->r_thread == 1) {
		struct tbin_node *r = t->left;
		while (r->r_thread == 0)
			r = r->right;
		if (r->right == t)
			r->right = t->right;

		*q = t->left;
	}
	else if (t->r_thread == 1) {
		if (s == NULL)
			*q = NULL;
		else if (item > s->data) {
			s->r_thread = 1;
			*q = t->right;
		}
		else {
			s->l_thread = 1;
			*q = t->left;
		}
	}
	else {
		struct tbin_node *r = t->right;
		if (r->l_thread == 1) {
			r->left = t->left;
			r->l_thread = t->l_thread;
			if (r->l_thread == 0) {
				struct tbin_node *p = r->left;
				while (p->r_thread == 0)
					p = p->right;
				p->right = r;
			}
			*q = r;
		}
		else {
			struct tbin_node *p = r->left;
			while (p->l_thread == 0) {
				r = p;
				p = r->left;
			}
	
			t->data = p->data;
			if (p->r_thread == 1) {
				r->l_thread = 1;
				r->left = t;
			}
			else {
				s = r->left = p->right;
				while (s->l_thread == 0)
					s = s->left;
				s->left = t;
			}
			t = p;
		}
	}

	tree->free_node(t);
	//free(t);
	tree->count--;
	return 1;
}

/* Helper function for tbin_walk().  Recursively prints data from each
   node in tree rooted at NODE in in-order. */
static void walk(const struct tbin_node *node,walk_func cb_func)
{
	if (node->l_thread == 0)
		walk(node->left,cb_func);
	cb_func(node) ; //printf("%d ", node->data);
	if (node->r_thread == 0)
		walk(node->right,cb_func);
}

/* Prints all the data items in TREE in in-order. */
void tbin_walk(const struct tbin_tree *tree,walk_func cb_func)
{
	ne_assert(tree != NULL);
	if (tree->root != NULL)
		walk(tree->root, cb_func);
}

/* Returns the next node in in-order in the tree after X, or NULL if X
   is the greatest node in the tree. */
struct tbin_node *tbin_successor(struct tbin_node *x)
{
	struct tbin_node *y;
	
	ne_assert(x != NULL);
	
	y = x->right;
	if (x->r_thread == 0)
		while (y->l_thread == 0)
			y = y->left;
	return y;
}

/* Return the node with the least value in the tree rooted at X. */
struct tbin_node *tbin_minimum(struct tbin_node *x)
{
	while (x->l_thread == 0)
		x = x->left;
	return x;
}

/* Returns the previous node in in-order in the tree after X, or NULL if X
   is the greatest node in the tree. */
struct tbin_node *tbin_predecessor(struct tbin_node *x)
{
	struct tbin_node *y;
	
	ne_assert(x != NULL);
	
	y = x->left;
	if (x->l_thread == 0)
		while (y->r_thread == 0)
			y = y->right;
	return y;
}

/* Return the node with the greatest value in the tree rooted at X. */
 struct tbin_node *tbin_maximum(struct tbin_node *x)
{
	while (x->r_thread == 0)
		x = x->right;
	return x;
}

/* Prints all the data items in TREE in in-order, using an iterative
   algorithm. */
void tbin_traverse(const struct tbin_tree *tree,walk_func cb_func)
{
	struct tbin_node *node;
	
	ne_assert(tree != NULL);
	if (tree->root != NULL)
		for (node = tbin_minimum(tree->root); node != NULL; node = tbin_successor(node))
			cb_func(node) ;//printf("%d ", node->data);
}

/* Destroys tree rooted at NODE. */
static void destroy(struct tbin_tree *tree, struct tbin_node *node)
{
	if (node->l_thread == 0)
		destroy(tree, node->left);
	if (node->r_thread == 0)
		destroy(tree, node->right);
	//free(node);
	if(tree->free_node)
		tree->free_node(node);

}

/* Destroys TREE. */
void tbin_destroy(struct tbin_tree *tree)
{
	ne_assert(tree != NULL);
	if (tree->root != NULL)
		destroy(tree, tree->root);
}

/* Returns the number of data items in TREE. */
NEINT32 tbin_count(const struct tbin_tree *tree)
{
	ne_assert(tree != NULL);
	return tree->count;
}

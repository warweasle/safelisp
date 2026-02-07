#include "rb-tree.h"

int rb_direction(cc node) {
  cc parent = (cc)cdr((cc)cdr(node));
  if (!parent) {
	printf("NO DIRECTION!\n");
	return -1;
  }// Root or invalid
  return (cdr(cdr(parent)) == node) ? 1 : 0;
}

cc create_rb_node(void* key, cc left, cc right, cc parent) {

  return cons(key, cons(left, cons(right, parent)));
}

#include "rbtree_template_c"

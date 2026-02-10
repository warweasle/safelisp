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

int objectCompare(void* data, void* a, void* b) {
  (void)data; // unused for now

  return 0;
}


void* map_get(cc map, void* object) {

  return NULL;
}

void* map_set(cc map, void* object, void* value) {

  return NULL;
}

void* map_rm(cc map, void* object) {

  return NULL;
}

#include "rbtree_template.c"

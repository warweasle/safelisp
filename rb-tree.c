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

int rbObjectCompare(void* data, void* a, void* b) {
  (void)data; // unused for now

  return compare(a, b);
}

void* mapget(void* map, void* object) {
  return cc_rb_find(map, object, rbObjectCompare, NULL);
}

void* mapadd(void* map, void* object, void* value) {

  cc c = to_cons(mapget(map, object));

  if(c) {
    return ERROR("KEY ALREADY EXISTS!");
  }
  // Add instead...
  else {
    cc_rb_insert(map, object, rbObjectCompare, NULL);
  }
  return value;
}

void* mapset(void* map, void* object, void* value) {

  cc c = to_cons(mapget(map, object));

  if(c) {
    
    if(c && is_cons(c)) {
      cdr(cons) = value;
    }
    else {
      return ERROR("Not a valid cons cell!");
    }
  }
  // Add instead...
  else {
    return ERROR("KEY DOESN'T EXIST!");
  }
  return value;
}

void* mapdel(void* map, void* object) {

  cc_rb_delete(map, object, rbObjectCompare, NULL);
  return NULL;
}

#include "rbtree_template.c"

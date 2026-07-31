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

int rbObjectPairCompare(void* data, void* a, void* b) {
  (void)data; // unused for now

  if(!a && !b) return 0;
  if(!a) return -1;
  if(!b) return 1;
  
  return compare(car(a), car(b));
}

int rbObjectCompare(void* data, void* a, void* b) {
  (void)data; // unused for now

  if(!b) return 1;
  
  return compare(a, car(b));
}



void* mapget(void* map, void* object) {
  // cc_rb_find returns the tree NODE (key . (left . (right . parent))),
  // not the key pair itself -- the stored value lives at cdr(car(node)).
  void* ret = cc_rb_find(map, object, rbObjectCompare, NULL);
  if(ret) return cdr(car(ret));
  else    return NULL;
}

// Like mapget, but returns the mutable (key . value) pair itself, so a
// caller can update the value in place via cdr(pair) = newValue.
void* mapget_pair(void* map, void* object) {
  void* ret = cc_rb_find(map, object, rbObjectCompare, NULL);
  return ret ? car(ret) : NULL;
}

void* mapadd(void* map, void* object, void* value) {

  /* cc c = to_cons(mapget(map, object)); */

  /* if(c) { */
  /*   return ERROR("KEY ALREADY EXISTS!"); */
  /* } */
  /* // Add instead... */
  /* else { */
  cc_rb_insert(map, (void*) cons(object, value), rbObjectPairCompare, NULL);
    //}
  return value;
}

void* mapset(void* map, void* object, void* value) {

  cc pair = to_cons(mapget_pair(map, object));
  if(pair) {
    cdr(pair) = value;
  }
  // Add instead...
  else {
    cc_rb_insert(map, (void*) cons(object, value), rbObjectPairCompare, NULL);
  }
  return value;
}

void* mapdel(void* map, void* object) {

  cc_rb_delete(map, object, rbObjectCompare, NULL);
  return NULL;
}

#include "rbtree_template.c"

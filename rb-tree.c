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

// A tree's optional custom comparator lives in cdr(map) -- unused by the
// rb-tree machinery itself, which only ever reads/writes car(map) as the
// root pointer (see RB_ROOT/RB_SET_ROOT in rb-tree.h). Only MAPMAKE ever
// sets this slot; every other map operation only ever reads it, never
// changes it. NULL there means "no custom comparator" -- the plain
// default compare() we've always used.
//
// RB_CMP_FUNC's signature only gives a comparator (data, left, right) --
// it never receives the tree object itself, so rbObjectCompare/
// rbObjectPairCompare can't look up cdr(map) directly. This small context
// carries both the tree's comparator and the calling Lisp code's env
// through that existing "data" slot instead of changing RB_CMP_FUNC.
typedef struct {
  void* comparator; // NULL, or a TYPE_NATIVE / TYPE_LAMBDA / TYPE_MACRO value
  void* env;
} cmp_context;

// A non-NULL comparator is called as a real Lisp value: a bare C function
// (TYPE_NATIVE) is called directly, no interpreter round-trip -- the fast
// path; anything else (TYPE_LAMBDA/TYPE_MACRO) goes through
// apply_with_values. Its result is read back as a signed integer, matching
// compare()'s own negative/zero/positive convention. A comparator that
// errors or returns a non-integer is treated as "equal" rather than
// crashing or corrupting the tree's ordering invariant.
static int call_lisp_comparator(void* comparator, void* a, void* b, void* env) {
  void* args = cons(a, cons(b, NULL));
  void* result;

  if(is_type(comparator, TYPE_NATIVE)) {
    native_func f = to_pointer(comparator)->p;
    result = f(args, env);
  }
  else {
    result = apply_with_values(comparator, args, env);
  }

  if(is_error(result) || !is_int(result)) return 0;

  return mpz_sgn(to_int(result)->num);
}

int rbObjectPairCompare(void* data, void* a, void* b) {
  if(!a && !b) return 0;
  if(!a) return -1;
  if(!b) return 1;

  cmp_context* ctx = data;
  if(ctx && ctx->comparator) {
    return call_lisp_comparator(ctx->comparator, car(a), car(b), ctx->env);
  }

  return compare(car(a), car(b));
}

int rbObjectCompare(void* data, void* a, void* b) {
  if(!b) return 1;

  cmp_context* ctx = data;
  if(ctx && ctx->comparator) {
    return call_lisp_comparator(ctx->comparator, a, car(b), ctx->env);
  }

  return compare(a, car(b));
}

// Every map operation builds a context from the tree's own stashed
// comparator (cdr(map), set once at MAPMAKE time) plus the caller's env,
// and passes it as "data" -- rbObjectCompare/rbObjectPairCompare read it
// back out. The context itself is a small GC-allocated struct, not a
// stack local, so it stays valid for exactly the duration of the one
// rb-tree call it's passed into (no different from any other GC value).
static cmp_context* make_cmp_context(void* map, void* env) {
  cmp_context* ctx = GC_malloc(sizeof(cmp_context));
  ctx->comparator = cdr((cc)map);
  ctx->env = env;
  return ctx;
}

void* mapget(void* map, void* object, void* env) {
  // cc_rb_find returns the tree NODE (key . (left . (right . parent))),
  // not the key pair itself -- the stored value lives at cdr(car(node)).
  void* ret = cc_rb_find(map, object, rbObjectCompare, make_cmp_context(map, env));
  if(ret) return cdr(car(ret));
  else    return NULL;
}

// Like mapget, but returns the mutable (key . value) pair itself, so a
// caller can update the value in place via cdr(pair) = newValue.
void* mapget_pair(void* map, void* object, void* env) {
  void* ret = cc_rb_find(map, object, rbObjectCompare, make_cmp_context(map, env));
  return ret ? car(ret) : NULL;
}

void* mapadd(void* map, void* object, void* value, void* env) {

  /* cc c = to_cons(mapget(map, object)); */

  /* if(c) { */
  /*   return ERROR("KEY ALREADY EXISTS!"); */
  /* } */
  /* // Add instead... */
  /* else { */
  cc_rb_insert(map, (void*) cons(object, value), rbObjectPairCompare, make_cmp_context(map, env));
  //}
  return value;
}

void* mapset(void* map, void* object, void* value, void* env) {

  cc pair = to_cons(mapget_pair(map, object, env));
  if(pair) {
    cdr(pair) = value;
  }
  // Add instead...
  else {
    cc_rb_insert(map, (void*) cons(object, value), rbObjectPairCompare, make_cmp_context(map, env));
  }
  return value;
}

void* mapdel(void* map, void* object, void* env) {

  cc_rb_delete(map, object, rbObjectCompare, make_cmp_context(map, env));
  return NULL;
}

#include "rbtree_template.c"

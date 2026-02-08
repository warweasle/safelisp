#ifndef RB_TREE_H
#define RB_TREE_H

#include "taco.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RB_NODE_TYPE       cons_cell
#define RB_TREE_TYPE       cons_cell
#define RB_KEY_TYPE        void*
#define RB_PREFIX          cc_rb_
  
#define RB_GET_KEY(node)        (car(node))
#define RB_LEFT(node)           ((cc)car(cdr(node)))
#define RB_RIGHT(node)          ((cc)car(cdr(cdr(node))))
#define RB_PARENT(node)         ((cc)cdr(cdr(cdr(node))))

#define RB_SET_KEY(node, val)   (car(node) = (val))
#define RB_SET_LEFT(node, val)  (car(cdr(node)) = (val))
#define RB_SET_RIGHT(node, val) (car(cdr(cdr(node))) = (val))
#define RB_SET_PARENT(node, val) (cdr(cdr(cdr(node))) = (val))

#define RB_ROOT(tree)      ((tree) ? (tree)->car : NULL)
#define RB_SET_ROOT(tree, val) do { if ((tree)) (tree)->cdr = (val); } while (0)
  
  /* #define RB_GET_COLOR(node) ((node) && is_red_black_flag_set(node) ? RB_RED : RB_BLACK) */
#define RB_GET_COLOR(p)   ((p) ? (is_red_black_flag_set(p) ? 1 : 0)	\
			   : 0)

#define RB_SET_COLOR(node, color)			\
  do {							\
    if (node) {						\
      if ((color) == RB_RED) set_red_black_flag(node);	\
      else clear_red_black_flag(node);			\
    }							\
  } while (0)

#define RB_RED             1
#define RB_BLACK           0

#define RB_DIRECTION(node) (RB_RIGHT(RB_PARENT(node)) == node)

#define RB_CREATE_NODE(key) (create_rb_node(key, NULL, NULL, NULL))

  cc create_rb_node(void* key, cc left, cc right, cc parent);
  
  typedef int (*RB_CMP_FUNC)(void* data, RB_KEY_TYPE* left, RB_KEY_TYPE* right);
    
#include "rbtree_template_h"

  /* cc rb_search(cc root, void* object, int (*compare)(void* data, const void*, const void*)); */
  /* cc rb_raw_search(cc root, void* object, int (*compare)(void* data, const void* obj, const void* raw)); */
  /* void rb_inorder(cc root, void (*visit)(void*)); */
  /* cc rb_insert(cc root, void* object, int (*compare)(void* data, const void*, const void*)); */
  /* cc rb_delete(cc root, void* object, int (*compare)(void* data, const void*, const void*)); */

#ifdef __cplusplus
}
#endif

#endif // RB_TREE_H


#ifndef __RBTREE_TEMPLATE_H__
#define __RBTREE_TEMPLATE_H__

#ifndef RB_NODE_TYPE
#error "You must define RB_NODE_TYPE before including this header."
#endif

#ifndef RB_TREE_TYPE
#error "You must define RB_TREE_TYPE before including this header."
#endif

#ifndef RB_LEFT
#error "You must define RB_LEFT(node) to access the left child."
#endif

#ifndef RB_RIGHT
#error "You must define RB_RIGHT(node) to access the right child."
#endif

#ifndef RB_PARENT
#error "You must define RB_PARENT(node) to access the parent pointer."
#endif

#ifndef RB_SET_LEFT
#error "You must define RB_SET_LEFT(node, val) to assign the left child."
#endif

#ifndef RB_SET_RIGHT
#error "You must define RB_SET_RIGHT(node, val) to assign the right child."
#endif

#ifndef RB_SET_PARENT
#error "You must define RB_SET_PARENT(node, val) to assign the parent pointer."
#endif

#ifndef RB_ROOT
#error "You must define RB_ROOT(tree) to access the root pointer."
#endif

#ifndef RB_SET_ROOT
#error "You must define RB_SET_ROOT(tree, node) to assign the root pointer."
#endif

#ifndef RB_GET_KEY
#error "You must define RB_GET_KEY(node) to extract the node's key."
#endif

#ifndef RB_DIRECTION
#error "You must define RB_DIRECTION(node) to get node's direction relative to parent."
#endif

#ifndef RB_CREATE_NODE
#error "You must define RB_CREATE_NODE(key) to create a node for a key."
#endif

#ifndef RB_GET_COLOR
#error "You must define RB_GET_COLOR(node) to get the node color."
#endif

#ifndef RB_SET_COLOR
#error "You must define RB_SET_COLOR(node, color) to set the node color."
#endif

#ifndef RB_RED
#error "You must define RB_RED for the red color value."
#endif

#ifndef RB_BLACK
#error "You must define RB_BLACK for the black color value."
#endif

#ifndef RB_KEY_TYPE
#error "You must define RB_KEY_TYPE before including this header."
#endif

#ifndef RB_PREFIX
#error "You must define RB_PREFIX before including this header."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RB_CONCAT2(a, b) a ## b
#define RB_CONCAT(a, b)  RB_CONCAT2(a, b)
#define RB_FUNC(name)    RB_CONCAT(RB_PREFIX, name)
  
  RB_NODE_TYPE* RB_FUNC(rotate_subtree)(RB_TREE_TYPE* tree, RB_NODE_TYPE* node, int right);
  void RB_FUNC(insert)(RB_TREE_TYPE* tree, RB_KEY_TYPE* node, RB_CMP_FUNC cmp, void* data);
  void RB_FUNC(remove)(RB_TREE_TYPE* tree, RB_NODE_TYPE* node);
  void RB_FUNC(delete)(RB_TREE_TYPE* tree, RB_KEY_TYPE* key, RB_CMP_FUNC cmp, void* data);
  RB_NODE_TYPE* RB_FUNC(find)(RB_TREE_TYPE* tree, RB_KEY_TYPE* key, RB_CMP_FUNC cmp, void* data);
  RB_NODE_TYPE* RB_FUNC(find_first)(RB_TREE_TYPE* tree);
  RB_NODE_TYPE* RB_FUNC(find_last)(RB_TREE_TYPE* tree);
  RB_NODE_TYPE* RB_FUNC(find_next)(RB_NODE_TYPE* node);
  RB_NODE_TYPE* RB_FUNC(find_prev)(RB_NODE_TYPE* node);
  int RB_FUNC(max_depth)(RB_TREE_TYPE* tree);
  void RB_FUNC(pretty_print)(RB_TREE_TYPE* tree, void (*printFunc)(RB_KEY_TYPE* key));
  
#ifdef __cplusplus
}
#endif

#endif // END RB_TREE_TEMPLATE

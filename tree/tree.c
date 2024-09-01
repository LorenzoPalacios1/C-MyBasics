#include "tree.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

tree_t *_new_tree(const void *const data, const size_t elem_size,
                  const size_t length) {
  const size_t NODE_SIZE = sizeof(node_t) + elem_size;
  /*
   * The first portion of the below expression determines the minimum amount of
   * memory required for the tree and its nodes. The second portion determines
   * the minimum amount of memory required to contain a list of pointers to each
   * node's children.
   *
   * `length - 1` exists in the second expression since the root node will have
   * two children, while every other node will have only one, except for the
   * last two, which will have no children. This gives the expression:
   *
   * (n + 1 - 2) * ptr_size
   *
   * where `n` is the number of nodes.
   */
  const size_t REQUIRED_MEM =
      (length * NODE_SIZE + sizeof(tree_t)) + ((length - 1) * sizeof(node_t *));
  tree_t *const tree_obj = malloc(REQUIRED_MEM);
  if (tree_obj == NULL) return NULL;

  /* Adding one since the root node is counted in the depth. */
  tree_obj->depth = length / 2 + 1;
  tree_obj->num_nodes = length;
  tree_obj->bytes_allocated = REQUIRED_MEM;
  tree_obj->bytes_used = REQUIRED_MEM;

  node_t *nodes_mem = (node_t *)((char *)tree_obj + sizeof(tree_t));
  node_t **children_mem = (node_t **)((char *)tree_obj + REQUIRED_MEM -
                                      ((length - 1) * sizeof(node_t *)));
  nodes_mem[0].parent = NULL;
  nodes_mem[0].num_children = 0;
  nodes_mem[0].value = (char *)nodes_mem + sizeof(node_t);
  memcpy(nodes_mem[0].value, data, elem_size);
  tree_obj->root = nodes_mem;
  nodes_mem = (node_t *)((char *)nodes_mem + NODE_SIZE);
  for (size_t i = 1; i < length; i++) {
    /*
     * This ensures the root node is the parent to two nodes and that all other
     * nodes are the parent to no more than one node.
     */
    if (i <= 2) {
      nodes_mem->parent = tree_obj->root;
      children_mem[i - 1] = nodes_mem;
      tree_obj->root->children = children_mem;
      tree_obj->root->num_children++;
      if (i == 2)
        children_mem = (node_t **)((char *)children_mem + 2 * sizeof(node_t *));
    } else {
      node_t *parent_node = (node_t *)((char *)nodes_mem - 2 * NODE_SIZE);
      nodes_mem->parent = parent_node;
      parent_node->children = children_mem;
      parent_node->children[0] = nodes_mem;
      parent_node->num_children = 1;
      children_mem = (node_t **)((char *)children_mem + sizeof(node_t *));
      /* The last two nodes in the tree have no children. */
      if (i >= length - 2) nodes_mem->num_children = 0;
    }
    nodes_mem->value = (char *)nodes_mem + sizeof(node_t);
    memcpy(nodes_mem->value, (char *)data + i * elem_size, elem_size);
    nodes_mem = (node_t *)((char *)nodes_mem + NODE_SIZE);
  }
  return tree_obj;
}

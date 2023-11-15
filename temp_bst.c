#include <stdio.h>
#include <stdlib.h>
#include "bst.h"

/* 삽입 */
void InsertNode(Bst *tree, Node *node)
{
    Node *y = NULL;
    Node *x = tree->root;
    while (x != NULL)
    {
        y = x;  // y에 x 이전 값 계속 저장
        if (node->key < x->key)
            x = x->left;
        else
            x = x->right;
    }
    node->parent = y;
    // printf("%d\n", node->parent->key);
    if (y == NULL)
        tree->root = node;
    else if (node->key < y->key)
        y->left = node;
    else
        y->right = node;
}

/* 이식 */
void TransplantNode(Bst *tree, Node *old_node, Node *new_node)
{
    if (old_node->parent == NULL)
        tree->root = new_node;
    else if (old_node == old_node->parent->left)
        old_node->parent->left = new_node;
    else
        old_node->parent->right = new_node;
    if (new_node != NULL)
        new_node->parent = old_node->parent;
}

/* 삭제 */
void DeleteNode(Bst *tree, Node *node)
{
    Node *y;
    if (node->left == NULL)
        TransplantNode(tree, node, node->right);
    else if (node->right == NULL)
        TransplantNode(tree, node, node->left);
    else{
        y = FindMinimum(node->right);
        if (y->parent != node){
            TransplantNode(tree, y, y->right);
            y->right = node->right;
            y->right->parent = y;
        }
        TransplantNode(tree, node, y);
        y->left = node->left;
        y->left->parent = y;
    }
    free(node);
}
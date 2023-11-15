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
typedef struct Node
{
    int key;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
} Node;
typedef struct Bst
{
    Node *root;
    Node *nil;
} Bst;
/* 노드 생성*/
Node *InitNode(int key);
/* 트리 생성 */
Bst *InitTree();
/* 중위 순회 */
void InorderTreeWalk(Node *node);
/* 노드 검색 */
Node *SearchNode(Node *root, int key);
/* 최소 원소 반환 */
Node *FindMinimum(Node *node);
/* 최대 원소 반환*/
Node *FindMaximum(Node *node);
/* Successor 반환 */
Node *FindSuccessor(Node *node);
/* Predecessor 반환 */
Node *FindPredecessor(Node *node);
/* 삽입 */
void InsertNode(Bst *tree, Node *node);
/* 이식 */
void TransplantNode(Bst *tree, Node *old_node, Node *new_node);
/* 삭제 */
void DeleteNode(Bst *tree, Node *node);
void DeleteTree(Bst *tree);
void postOrderDelete(Node* node);
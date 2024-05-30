#ifndef SET_H
#define SET_H

#include "mini_uart.h"
#include "alloc.h"

typedef struct nodeList {
    int key;
    struct nodeList *left;
    struct nodeList *right;
    int height;
} nodeList;

int height(nodeList *N) {
    if (N == NULL) return 0;
    return N->height;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

nodeList* newNode(int key) {
    nodeList *node = (nodeList*) simple_malloc(1, sizeof(nodeList));
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    return(node);
}

nodeList* rightRotate(nodeList *y) {
    nodeList *x = y->left;
    nodeList *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x;
}

nodeList* leftRotate(nodeList *x) {
    nodeList *y = x->right;
    nodeList *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    return y;
}

int getBalance(nodeList *N) {
    if (N == NULL) return 0;
    return height(N->left) - height(N->right);
}

nodeList* insert(nodeList* node, int key) {
    if (node == NULL) return newNode(key);

    if (key < node->key)
        node->left = insert(node->left, key);
    else if (key > node->key)
        node->right = insert(node->right, key);
    else
        return node;

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = getBalance(node);

    if (balance > 1 && key < node->left->key)
        return rightRotate(node);

    if (balance < -1 && key > node->right->key)
        return leftRotate(node);

    if (balance > 1 && key > node->left->key) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && key < node->right->key) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

nodeList * minValueNode(nodeList* node) {
    nodeList* current = node;

    while (current->left != NULL)
        current = current->left;

    return current;
}

nodeList* deleteNode(nodeList* root, int key) {
    if (root == NULL) return root;

    if ( key < root->key )
        root->left = deleteNode(root->left, key);
    else if( key > root->key )
        root->right = deleteNode(root->right, key);
    else {
        if( (root->left == NULL) || (root->right == NULL) ) {
            nodeList *temp = root->left ? root->left : root->right;

            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else
             *root = *temp;
        } else {
            nodeList* temp = minValueNode(root->right);
            root->key = temp->key;
            root->right = deleteNode(root->right, temp->key);
        }
    }

    if (root == NULL)
      return root;

    root->height = 1 + max(height(root->left), height(root->right));
    int balance = getBalance(root);

    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);

    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);

    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

// Function to get any arbitrary element
int getAnyElement(nodeList* root) {
    if (root != NULL) {
        return root->key; // Simply returns the root key as an arbitrary element
    }
    return -1; // or handle appropriately if the tree is empty
}

void preOrder(nodeList* root) {
    if (root != NULL) {
        uart_printf("%d ", root->key);
        preOrder(root->left);
        preOrder(root->right);
    }
}

#endif


#define NULL (void*)0

/* AVL Tree */
typedef struct AVLNode{
  int key;
  int height;
  // memory_block *block;
  struct AVLNode *left;
  struct AVLNode *right;
}AVLNode;

AVLNode *leftRotate(AVLNode *node){
  // uart_sends("leftRotate\n");
  AVLNode *right = node->right;
  AVLNode *right_left = right->left;

  right->left = node;
  node->right = right_left;

  if(!node->left && !node->right)
    node->height = 1;
  else if(!node->left)
    node->height = 1 + node->right->height;
  else if(!node->right)
    node->height = 1 + node->left->height;
  else
    node->height = 1 + (node->left->height > node->right->height ? node->left->height : node->right->height);

  if(!right->right)
    right->height = 1 + right->left->height;
  else
    right->height = 1 + (right->left->height > right->right->height ? right->left->height : right->right->height);

  return right;
}

AVLNode *rightRotate(AVLNode *node){
  // uart_sends("rightRotate\n");
  AVLNode *left = node->left;
  AVLNode *left_right = left->right;

  left->right = node;
  node->left = left_right;

  if(!node->right && !node->left)
    node->height = 1;
  else if(!node->right)
    node->height = 1 + node->left->height;
  else if(!node->left)
    node->height = 1 + node->right->height;
  else
    node->height = 1 + (node->left->height > node->right->height ? node->left->height : node->right->height);

  if(!left->left)
    left->height = 1 + left->right->height;
  else left->height = 1 + (left->left->height > left->right->height ? left->left->height : left->right->height);

  return left;
}

int getBalance(AVLNode *node){
  if(!node)
    return 0;
  if(!node->left && !node->right)
    return 0;
  if(!node->left)
    return -node->right->height;
  if(!node->right)
    return node->left->height;
  return node->left->height - node->right->height;
}

int minimum(AVLNode *root){
  AVLNode *tmp = root;
  while(tmp->left != NULL)
    tmp = tmp->left;
  return tmp->key;
}

void insert(AVLNode **root, int index){
  if(!(*root)){
    AVLNode *node = (AVLNode *)simple_malloc(sizeof(AVLNode));
    if(!node){
      uart_sends("Error: malloc failed\n");
      return;
    }
    node->key = index;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    *root = node;
  }
  else{
    if(index < (*root)->key)
      insert(&((*root)->left), index);
    else if(index > (*root)->key)
      insert(&((*root)->right), index);
    else return;

    int balance = 0;

    if(!(*root)->left && !(*root)->right)
      (*root)->height = 1;
    else if(!(*root)->left){
      (*root)->height = 1 + (*root)->right->height;
      balance = -(*root)->right->height;
    }
    else if(!(*root)->right){
      (*root)->height = 1 + (*root)->left->height;
      balance = (*root)->left->height;
    }
    else{
      (*root)->height = 1 + ((*root)->left->height > (*root)->right->height ? (*root)->left->height : (*root)->right->height);
      balance = (*root)->left->height - (*root)->right->height;
    }

    // LL
    if(balance > 1 && index < (*root)->left->key){
      (*root) = rightRotate(*root);
    }
    // LR
    else if(balance > 1 && index > (*root)->left->key){
      (*root)->left = leftRotate((*root)->left);
      (*root) = rightRotate(*root);
    }
    // RR
    else if(balance < -1 && index > (*root)->right->key){
      (*root) = leftRotate(*root);
    }
    // RL
    else if(balance < -1 && index < (*root)->right->key){
      (*root)->right = rightRotate((*root)->right);
      (*root) = leftRotate(*root);
    }
  }

}

AVLNode *delete(AVLNode **root, int index){
  if(!(*root))
    return NULL;

  if(index < (*root)->key)
    (*root)->left = delete(&((*root)->left), index);
  else if(index > (*root)->key)
    (*root)->right = delete(&((*root)->right), index);
  else{ // index == (*root)->key
    if(!(*root)->left || !(*root)->right){
      AVLNode *temp = (*root)->left ? (*root)->left : (*root)->right;
      if(temp == NULL){
        temp = *root;
        *root = NULL;
      }
      else
        *root = temp;
      // simple_free(temp);
    }
    else{
      AVLNode *temp = (*root)->right;
      while(temp->left != NULL)
        temp = temp->left;
      (*root)->key = temp->key;
      (*root)->right = delete(&((*root)->right), temp->key);
    }
  }
  if(!(*root))
    return NULL;

  int balance = 0;

  if(!(*root)->left && !(*root)->right)
    (*root)->height = 1;
  else if(!(*root)->left){
    (*root)->height = 1 + (*root)->right->height;
    balance = -(*root)->right->height;
  }
  else if(!(*root)->right){
    (*root)->height = 1 + (*root)->left->height;
    balance = (*root)->left->height;
  }
  else{
    (*root)->height = 1 + ((*root)->left->height > (*root)->right->height ? (*root)->left->height : (*root)->right->height);
    balance = (*root)->left->height - (*root)->right->height;
  }

  // LL
  if(balance > 1 && getBalance((*root)->left) >= 0)
    (*root) = rightRotate(*root);
  // LR
  else if(balance > 1 && getBalance((*root)->left) < 0){
    (*root)->left = leftRotate((*root)->left);
    (*root) = rightRotate(*root);
  }
  // RR
  else if(balance < -1 && getBalance((*root)->right) <= 0)
    (*root) = leftRotate(*root);
  // RL
  else if(balance < -1 && getBalance((*root)->right) > 0){
    (*root)->right = rightRotate((*root)->right);
    (*root) = leftRotate(*root);
  }

  return *root;
}

void inOrder(AVLNode* root) {
    if (root != NULL) {
        inOrder(root->left);
        uart_sendi(root->key);
        uart_sends(" ");
        inOrder(root->right);
    }
}

void preOrder(AVLNode* root) {
    if (root != NULL) {
      uart_sendi(root->key);
      uart_sends(" ");
      preOrder(root->left);
      preOrder(root->right);
    }
}
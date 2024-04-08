#include "uart.h"
#include "allocator.h"
#include "timer_heap.h"

heap *createHeap(int capacity)
{
    // Allocating memory to heap h
    heap *h = (heap *)simple_malloc(sizeof(heap));

    // Checking if memory is allocated to h or not
    if (h == 0)
    {
        uart_puts("Memory error\n");
        return 0;
    }
    // set the values to size and capacity
    h->size = 0;
    h->capacity = capacity;

    // Allocating memory to array
    h->arr = (timer *)simple_malloc(capacity * sizeof(timer));

    // Checking if memory is allocated to h or not
    if (h->arr == 0)
    {
        uart_puts("Memory error\n");
        return 0;
    }
    return h;
}

// Defining insertHelper function
void insertHelper(heap *h, int index)
{

    // Store parent of element at index
    // in parent variable
    int parent = (index - 1) / 2;

    if (h->arr[parent].expire > h->arr[index].expire)
    {
        // Swapping when child is smaller
        // than parent element
        timer temp = h->arr[parent];
        h->arr[parent]= h->arr[index];
        h->arr[index] = temp;

        // Recursively calling insertHelper
        insertHelper(h, parent);
    }
}

void heapify(heap *h, int index)
{
    int left = index * 2 + 1;
    int right = index * 2 + 2;
    int min = index;

    // Checking whether our left or child element
    // is at right index or not to avoid index error
    if (left >= h->size || left < 0)
        left = -1;
    if (right >= h->size || right < 0)
        right = -1;

    // store left or right element in min if
    // any of these is smaller that its parent
    if (left != -1 && h->arr[left].expire < h->arr[index].expire)
        min = left;
    if (right != -1 && h->arr[right].expire < h->arr[min].expire)
        min = right;

    // Swapping the nodes
    if (min != index)
    {
        timer temp = h->arr[min];
        h->arr[min] = h->arr[index];
        h->arr[index] = temp;

        // recursively calling for their child elements
        // to maintain min heap
        heapify(h, min);
    }
}

timer extractMin(heap *h)
{
    timer deleteItem;

    // Checking if the heap is empty or not
    if (h->size == 0)
    {
        deleteItem.expire = -1;
        return deleteItem;
    }

    // Store the node in deleteItem that
    // is to be deleted.
    deleteItem = h->arr[0];

    // Replace the deleted node with the last node
    h->arr[0] = h->arr[h->size - 1];
    // Decrement the size of heap
    h->size--;

    // Call minheapify_top_down for 0th index
    // to maintain the heap property
    heapify(h, 0);
    return deleteItem;
}

// Define a insert function
void insert(heap *h, timer t)
{
    // Checking if heap is full or not
    if (h->size < h->capacity)
    {
        // Inserting data into an array
        h->arr[h->size] = t;
        // Calling insertHelper function
        insertHelper(h, h->size);
        // Incrementing size of array
        h->size++;
    }
}
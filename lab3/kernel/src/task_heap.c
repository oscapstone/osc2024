#include "uart.h"
#include "allocator.h"
#include "task_heap.h"

task_heap *create_task_heap(int capacity)
{
    // Allocating memory to heap h
    task_heap *h = (task_heap *)simple_malloc(sizeof(task_heap));

    // set the values to size and capacity
    h->size = 0;
    h->capacity = capacity;

    // Allocating memory to array
    h->arr = (task *)simple_malloc(capacity * sizeof(task));

    return h;
}

// Defining insertHelper function
void task_heap_insertHelper(task_heap *h, int index)
{

    // Store parent of element at index
    // in parent variable
    int parent = (index - 1) / 2;

    if (h->arr[parent].priority > h->arr[index].priority)
    {
        // Swapping when child is smaller
        // than parent element
        task temp = h->arr[parent];
        h->arr[parent] = h->arr[index];
        h->arr[index] = temp;

        // Recursively calling insertHelper
        task_heap_insertHelper(h, parent);
    }
}

void task_heap_heapify(task_heap *h, int index)
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
    if (left != -1 && h->arr[left].priority < h->arr[index].priority)
        min = left;
    if (right != -1 && h->arr[right].priority < h->arr[min].priority)
        min = right;

    // Swapping the nodes
    if (min != index)
    {
        task temp = h->arr[min];
        h->arr[min] = h->arr[index];
        h->arr[index] = temp;

        // recursively calling for their child elements
        // to maintain min heap
        task_heap_heapify(h, min);
    }
}

task task_heap_extractMin(task_heap *h)
{
    task deleteItem;

    // Checking if the heap is empty or not
    if (h->size == 0)
    {
        deleteItem.priority = -1;
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
    task_heap_heapify(h, 0);
    return deleteItem;
}

// Define a insert function
void task_heap_insert(task_heap *h, task t)
{
    // Checking if heap is full or not
    if (h->size < h->capacity)
    {
        // Inserting data into an array
        h->arr[h->size] = t;
        // Calling insertHelper function
        task_heap_insertHelper(h, h->size);
        // Incrementing size of array
        h->size++;
    }
}
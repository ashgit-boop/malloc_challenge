//
// >>>> malloc challenge! <<<<
//
// Your task is to improve utilization and speed of the following malloc
// implementation.
// Initial implementation is the same as the one implemented in simple_malloc.c.
// For the detailed explanation, please refer to simple_malloc.c.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<limits.h>

//
// Interfaces to get memory pages from OS
//


// sizeはOSからもらいたい領域の大きさ、OSはメモリ領域を確保してその先頭アドレスを返す
void *mmap_from_system(size_t size);

// ptrバンチからsizeバイトだけ領域をOSを返す。
void munmap_to_system(void *ptr, size_t size);

//
// Struct definitions
//


// 空き領域の前に、その領域のサイズと次の空き領域へのポインタの情報を置く。
typedef struct my_metadata_t {
  size_t size; // サイズ
  struct my_metadata_t *next; // 次のmeta_dataへのポインタ
} my_metadata_t;

typedef struct my_heap_t {
  my_metadata_t *free_head; // freeするかもしれない領域のmeta_dataのリストのリスト（Linked_List）の先頭ノードを返す
  my_metadata_t dummy; // リストの終わりを示すために使われているけれど、正直いらない？
} my_heap_t;

//
// Static variables (DO NOT ADD ANOTHER STATIC VARIABLES!)
//
//my_heap_t my_heap;
my_heap_t my_heap[5]; // free list binを作成0~4の添え字になること想定(size/1000を添え字にして分けてみる)

//
// Helper functions (feel free to add/remove/edit!)
//


// 開いた領域のメタデータを受け取り、空きリストに追加
void my_add_to_free_list(my_metadata_t *metadata) {
  int idx; // free list binの添え字をidxとする
  assert(!metadata->next);
  idx = metadata->size / 1000;
  metadata->next = my_heap[idx].free_head;
  my_heap[idx].free_head = metadata;
  //metadata->next = my_heap.free_head; // 先頭に（から）追加していくイメージ
  //my_heap.free_head = metadata;
}

// free_listに入っていたメタデータを（使うことになったから）空きリストから削除する
void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  int idx;
  if (prev) { // もし消すやつが先頭じゃなくてprevがいれば
    prev->next = metadata->next;
  } else {
    idx = metadata->size / 1000;
    my_heap[idx].free_head = metadata->next;
  }
  metadata->next = NULL;
}

//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//

// This is called at the beginning of each challenge.
void my_initialize() {
  for(int idx=0;idx<5;idx++){
  my_heap[idx].free_head = &my_heap[idx].dummy;
  my_heap[idx].dummy.size = 0;
  my_heap[idx].dummy.next = NULL;
  }
} 

// my_malloc() is called every time an object is allocated.
// |size| is guaranteed to be a multiple of 8 bytes and meets 8 <= |size| <=
// 4000. You are not allowed to use any library functions other than
// mmap_from_system() / munmap_to_system().
void *my_malloc(size_t size) {
  my_metadata_t *metadata; //= my_heap.free_head;
  my_metadata_t *prev = NULL;
  my_metadata_t *min_metadata = NULL;
  my_metadata_t *min_prev = NULL;
  int min_size = INT_MAX; // 最初はこの値より小さいサイズの空き領域をmin_metadataにする
  int idx;
  int found = 0; // bestfitが見つかった時に1、見つかっていないときには０


  for(idx=size / 1000;idx<5;idx++){
    metadata = my_heap[idx].free_head;

    // best fit // 恐らくスピードは落ちる（最後まで見ていって最小のサイズのものを見つけるから）、Utilizationは上がってほしい...
    while (metadata ) { // metadata(tmp)が存在してmetadataの持つサイズが指定されたサイズよりも小さい間移動させる。(first fit)
      if(size <= metadata->size && metadata->size < min_size){ // この最初の条件size <= metadata->sizeを忘れると正しい大きさのメモリ確保ができない
        min_prev = prev;
        min_metadata = metadata;
        //min_size = min_metadata->size;
        break;
      }  
      //prev = metadata;
      //metadata = metadata->next;
    }
  }
  prev = min_prev; // prevをmin_metadataのprevに変更
  metadata = min_metadata; // metadataをmin_metadataに変更


  // now, metadata points to the best free slot
  // and prev is the previous entry.

  if (!metadata) { // metadataがNULLだった
    // There was no free slot available. We need to request a new memory region
    // from the system by calling mmap_from_system().
    //
    //     | metadata | free slot |
    //     ^
    //     metadata
    //     <---------------------->
    //            buffer_size
    size_t buffer_size = 4096;
    my_metadata_t *metadata = (my_metadata_t *)mmap_from_system(buffer_size);
    metadata->size = buffer_size - sizeof(my_metadata_t); // metadataの分だけサイズを除く
    metadata->next = NULL;
    // Add the memory region to the free list.
    my_add_to_free_list(metadata);
    // Now, try my_malloc() again. This should succeed.
    return my_malloc(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = metadata + 1;
  size_t remaining_size = metadata->size - size;
  // Remove the free slot from the free list.
  my_remove_from_free_list(metadata, prev);

  if (remaining_size > sizeof(my_metadata_t)) {
    // Shrink the metadata for the allocated object
    // to separate the rest of the region corresponding to remaining_size.
    // If the remaining_size is not large enough to make a new metadata,
    // this code path will not be taken and the region will be managed
    // as a part of the allocated object.
    metadata->size = size;
    // Create a new metadata for the remaining free slot.
    //
    // ... | metadata | object | metadata | free slot | ...
    //     ^          ^        ^
    //     metadata   ptr      new_metadata
    //                 <------><---------------------->
    //                   size       remaining size
    my_metadata_t *new_metadata = (my_metadata_t *)((char *)ptr + size);
    new_metadata->size = remaining_size - sizeof(my_metadata_t);
    new_metadata->next = NULL;
    // Add the remaining free slot to the free list.
    my_add_to_free_list(new_metadata);
  }
  return ptr;
} // my_mallocここまで

// This is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
  // Look up the metadata. The metadata is placed just prior to the object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  // Add the free slot to the free list.
  my_add_to_free_list(metadata);
}

// This is called at the end of each challenge.
void my_finalize() {
  // Nothing is here for now.
  // feel free to add something if you want!
}

void test() {
  // Implement here!
  assert(1 == 1); /* 1 is 1. That's always true! (You can remove this.) */
}

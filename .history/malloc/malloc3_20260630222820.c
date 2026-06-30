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

void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev);

// freeされた領域のメタデータを受け取り、空きリストに追加
void my_add_to_free_list(my_metadata_t *metadata) {
  //printf("my_add_to_free_list\n");
  //printf("metadata:%p\n",metadata);
  //printf("metadata->size:%ld\n",metadata->size);
  int idx; // free_list binの添え字をidxとする

  //printf("metadata->size:%ld\n",metadata->size);

  assert(!metadata->next);

  idx = metadata->size / 256;
  if(idx>= 4){
    idx = 4;
  }
  metadata->next = my_heap[idx].free_head; // 先頭に（から）追加していくイメージ
  my_heap[idx].free_head = metadata;

  my_metadata_t *tmp_metadata ; // リスト上で見ていくときの今見ているノード(今見ているメタデータ)
  my_metadata_t *prev_metadata; // リスト上でのtmp_metadataの一つ前のノード
  my_metadata_t *next_metadata; // リスト上でのtmp_metadataの次のノード
  my_metadata_t *new_metadata; // リスト上での,tmp_metadataが探していた隣合っている空き領域のノードだったとき、metadataのサイズを変える前のノード

  next_metadata = metadata->next; 
  prev_metadata = metadata;

  // 右側マージ 
  for(tmp_metadata = next_metadata; tmp_metadata!= &my_heap[idx].dummy;tmp_metadata = tmp_metadata->next){ // リストでつながっているmetadataを順に追っていく
    next_metadata = tmp_metadata->next; // next_metadataを用意しているのは、途中でmetadataが削除されるとmetadata->nextはNULLになる(セグフォになった気がする)
    // 右隣のマージできそうなものが見つかった
    if(tmp_metadata == (my_metadata_t*)((char*)metadata + sizeof(*metadata) + metadata->size)){ // 今リスト上で確認しているメタデータが、メモリ上でmetadataの右隣にある空き領域のメタデータだったら
      //printf("found\n");
      new_metadata->size = metadata->size + tmp_metadata->size + sizeof(*metadata); // 左側のmetadataのサイズを更新 // これのサイズを変えたら別のidxになるかもだから入れ直さなきゃ..?
      new_metadata->next = tmp_metadata -> next;
      //printf("before remove\n");
      my_remove_from_free_list(tmp_metadata,prev_metadata); // 右側にあるメタデータtmpをfree_listから削除
      my_add_to_free_list(new_metadata); // sizeが変わったmetadataをfree_list_binに入れ直す
      my_remove_from_free_list(metadata,NULL); // metadataはリストの先頭に入れられるはずだからprevはNULL
      //printf("after remove\n");
      // サイズを拡張した左側のmetadataも一度削除して入れ直す->更にその右のがつながっているかわかる?
      //continue;
      return; 
    }
    //printf("After if \n");
    prev_metadata = tmp_metadata; // これだけだと次にfor文最初に行ったときにtmp_metadata->nextが存在しなくなる
  }
  // 特にマージできそうなやつはなかった
  return;
  //printf("Finished adding metadata->size:%ld\n",metadata->size);

}


// free_listに入っていたメタデータを（使うことになったから）空きリストから削除する
void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  //printf("my_remove_from_free_list\n");
  int idx;

  if (prev) { // もし消すやつが先頭じゃなくてprevがいれば
    prev->next = metadata->next;
  } 
  else {
    idx = metadata->size / 256;
    if(idx>=4){
      idx = 4;
    }
    my_heap[idx].free_head = metadata->next;
  }
  metadata->next = NULL;
}

//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//

/*
// This is called at the beginning of each challenge.
void my_initialize() {
  //printf("my_initialize\n");
  for(int idx=0;idx<5;idx++){
  my_heap[idx].free_head = &my_heap[idx].dummy;
  my_heap[idx].dummy.size = 0;
  my_heap[idx].dummy.next = NULL;
  }
} 
  */


void my_initialize() {
  //printf("my_initialize\n");
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
  //printf("my_malloc\n");
  //printf("mallocしたいsize:%ld\n",size);
  my_metadata_t *metadata; //= my_heap.free_head;
  my_metadata_t *prev = NULL;
  my_metadata_t *min_metadata = NULL;
  my_metadata_t *min_prev = NULL;
  int min_size = INT_MAX; // 最初はこの値より小さいサイズの空き領域をmin_metadataにする
  int index;
  
  index=size / 256;
  if(index >= 4){
    index = 4;
  }

  //printf("1\n");
  for(int idx=index;idx<5;idx++){
    metadata = my_heap[idx].free_head;
    //printf("metadata:%p\n",metadata);
    //printf("idx:%d\n",idx);
    prev = NULL;
    min_prev = NULL;
    int best_fit_found = 0; // bestfitが見つかった時に1、見つかっていないときには０
    //if(size!=128){
    //printf("size:%ld\n",size);
    //}

    // best fit 
    //printf("size:%ld\n",size);
    //printf("before while\n");

    while (metadata!=NULL) { 
      //printf("while start\n");
      //printf("metadata->size:%ld\n",metadata->size);
      // このif文の条件は、最初はmetadata == dummyだからmetadata->size = 0になる。if文に入らない
      //printf("size:%ld,metadata->size:%ld,min_size:%d\n",size,metadata->size,min_size);
      if(size <= metadata->size && metadata->size < min_size){ // この最初の条件size <= metadata->sizeを忘れると正しい大きさのメモリ確保ができない
        min_prev = prev;
        min_metadata = metadata;
        min_size = min_metadata->size;
        best_fit_found = 1; // このリストの中でfitするものは一応見つかった
        //printf("size:%ld\n",size);
        //printf("found\n");
        //break;
        //printf("idx:%d\n",idx);
        //printf("min_size:%d\n",min_size);
      }  
      prev = metadata;
      metadata = metadata->next;
    }
    //printf("idx:%d\n",idx);
    if (best_fit_found == 1){// もしbest_fitが見つかっていたらfor文を抜ける
      //printf("break\n");
      break;
    }
  }// for文抜け
  //printf("min_size:%ld\n",min_size);

  prev = min_prev; // prevをmin_metadataのprevに変更
  metadata = min_metadata; // metadataをmin_metadataに変更

  //mmap_from_system(metadata->size); OSからメモリはすでにもらっているから呼ばないか...

  // now, metadata points to the best free slot
  // and prev is the previous entry.

  if (!metadata) { // metadataがNULLだった（OSからメモリをもらわなきゃいけない、逆に言えばここ以外でOSからメモリをもらいに行く必要ない...?）
    //printf("metadata was NULL!!\n");
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
    //printf("before my_add_to_list  metadata->size : %ld\n",metadata->size);
    my_add_to_free_list(metadata); // ここは一度しか呼ばれていない
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
    //printf("new_metadata->size:%ld\n",new_metadata->size);
    my_add_to_free_list(new_metadata);
  }
  return ptr;
} // my_mallocここまで

// This is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
  //printf("my_free\n");
  // Look up the metadata. The metadata is placed just prior to the object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  //munmap_to_system(ptr,metadata->size); // OSに返すわけではない、あくまで今の確保済みの領域を空き領域に追加するだけ
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

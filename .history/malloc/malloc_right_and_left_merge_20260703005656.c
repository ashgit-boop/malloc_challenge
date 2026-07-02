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
typedef struct my_metadata_t { // ここの構造体のサイズが大きいからutilization上がらないんじゃないか
  size_t size; // サイズ
  struct my_metadata_t *next; // リスト上における次のmeta_dataへのポインタ
  struct my_metadata_t *previous; // リスト上における前のmetadataへのポインタ
  struct my_metadata_t *left; // メモリ上で左隣にある領域のメタデータ 
  struct my_metadata_t *right; // メモリ上で右隣にある領域のメタデータ
  bool is_free; // このメタデータをもつ領域が空き領域かどうかを表す
} my_metadata_t; // 


typedef struct my_heap_t {
  my_metadata_t *free_head; // freeするかもしれない領域のmeta_dataのリストのリスト（Linked_List）の先頭ノードを返す
  my_metadata_t dummy; // リストの終わりを示すために使われているけれど、正直いらない？
} my_heap_t;

//
// Static variables (DO NOT ADD ANOTHER STATIC VARIABLES!)
//
//my_heap_t my_heap;
my_heap_t my_heap[50]; // free list binを作成0~4の添え字になること想定(size/1000を添え字にして分けてみる)

//
// Helper functions (feel free to add/remove/edit!)
//

// 与えられたメモリのサイズから、free_list_binのどこのindexのリストに入るかを計算
int calculate_index(size_t size){
    int index;
    index = size / 80;
    
    if(index >=49){
        index = 49;
    }
    return index;
}



// freeされた領域のメタデータを受け取り、空きリストに追加
void my_add_to_free_list(my_metadata_t *metadata) {
  int idx; // free_list binの添え字をidxとする

  assert(!metadata->next);
  idx = calculate_index(metadata->size);

  metadata->previous = NULL;

  metadata->next = my_heap[idx].free_head; // 先頭に（から）追加していくイメージ
  if(metadata->next != NULL){
    metadata->next->previous = metadata;
  }

  my_heap[idx].free_head = metadata;
  metadata->is_free = true;
}


// free_listに入っていたメタデータを（使うことになったから）空きリストから削除する
void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  int idx;

  idx = calculate_index(metadata->size);

  if (prev) { // もし消すやつが先頭じゃなくてprevがいれば
    prev->next = metadata->next;
    
    if(metadata->next != &my_heap[idx].dummy){        
        metadata->next->previous = prev;
    }

  }

  else { // もし消すやつが先頭だったら(metadata->previouはすでにNULLのハズ) //このなかがうまく行っていない
    my_heap[idx].free_head = metadata->next;


    if (my_heap[idx].free_head != &my_heap[idx].dummy && my_heap[idx].free_head != NULL)
    my_heap[idx].free_head->previous = NULL; // ここで落ちている

    if (metadata->next != NULL)
    metadata->next->previous = NULL;
  }
  metadata->previous = NULL;
  metadata->next = NULL;
}


// 右結合をする
void* right_merge(my_metadata_t *metadata){ // 引数はfreeしたメタデータ

  my_metadata_t *right_metadata=NULL; // metadataの右側にある領域

    // もしmetadataのメモリ上での右隣の領域が空き領域ではなかったら、何もせずにサヨナラ

    
    if(metadata->right == NULL){ // metadataの右隣がない、つまりmetadataが一番右端っこのとき右側マージできない
        return metadata;
    }

    right_metadata = metadata->right; // この時点でright_metadataはNULLでないことが確定

    if(right_metadata -> is_free == true){ // もしmetadataの右隣の領域が空き領域だったら、その領域をリスト上で探して空き領域リストに加える
        //old_metadata = metadata;
        my_remove_from_free_list(right_metadata,right_metadata->previous); // 統合する右側の領域をリストから削除 

        // 領域を統合
        metadata->size = metadata->size + right_metadata->size + sizeof(*right_metadata); // サイズを拡張
        // サイズを更新したmetadataを入れ直す
        metadata->right = right_metadata->right;

        if (metadata->right!=NULL)
          metadata->right->left = metadata;

        // 統合したあとのメモリ領域上で、さらに右隣が空き領域のとき、もう一度right_mergeする。
        if(metadata->right != NULL && metadata -> right -> is_free == true){ 
            //metadata->right->left = metadata;
            right_merge(metadata);
        }
    }
    
    return ;
}

// 左結合をする
my_metadata_t *left_merge(my_metadata_t *metadata){
    my_metadata_t *left_metadata=NULL; // metadataの左側にある領域

    // もしmetadataのメモリ上での左隣の領域が空き領域ではなかったら、何もせずにサヨナラ

    if(metadata->left == NULL){ // metadataの左隣がない、つまりmetadataが一番左端っこのとき左側マージできない
        left_metadata = metadata;
        return left_metadata;
    }

    left_metadata = metadata -> left; // この時点でleft_metadataはNULLでないことが確定

    if(left_metadata -> is_free == true){ // もしmetadataの左隣の領域が空き領域だったら、その領域をリスト上で探して空き領域リストに加える
        
        // 領域を統合
        my_remove_from_free_list(left_metadata,left_metadata->previous);
        left_metadata->size = left_metadata->size + metadata->size + sizeof(*metadata); // サイズを拡張
        
        // サイズを更新したleft_metadataを入れ直す
        left_metadata->right = metadata->right; // これがNULLらしい

        if(left_metadata->right != NULL){
            left_metadata->right->left = left_metadata;
        }

        // 統合したあとのメモリ領域上で、さらに右隣が空き領域のとき、もう一度right_mergeする。
        if(left_metadata->left != NULL && left_metadata -> left -> is_free == true){ 
            left_metadata = left_merge(left_metadata);
        }
    }
    else{
        left_metadata = metadata;
    }
    return left_metadata;
}



//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//


void my_initialize() {
  for(int idx=0;idx<50;idx++){
  my_heap[idx].free_head = &my_heap[idx].dummy; // 先頭はdummy
  my_heap[idx].dummy.size = 0; // dummyのサイズは0
  my_heap[idx].dummy.next = NULL; // dummyの次のノードは無い
  my_heap[idx].dummy.previous = NULL; // dummyの前のノードは無い
  my_heap[idx].dummy.is_free = false; // このノードは使えない
  }
} 



// my_malloc() is called every time an object is allocated.
// |size| is guaranteed to be a multiple of 8 bytes and meets 8 <= |size| <=
// 4000. You are not allowed to use any library functions other than
// mmap_from_system() / munmap_to_system().

void *my_malloc(size_t size) {

  my_metadata_t *metadata; 
  my_metadata_t *min_metadata=NULL;
  my_metadata_t *min_prev=NULL;
  size_t min_size = 10000; // 最初はこの値より小さいサイズの空き領域をmin_metadataにする
  int index;
  
  index = calculate_index(size);

  for(int idx=index;idx<50;idx++){
    metadata = my_heap[idx].free_head;


    int best_fit_found = 0; // bestfitが見つかった時に1、見つかっていないときには０

    while (metadata!=&my_heap[idx].dummy) { 
        // もし条件に合うものが見つかったら使用するメモリのメタデータを変更
      if(size <= metadata->size && metadata->size < min_size ){ // この最初の条件size <= metadata->sizeを忘れると正しい大きさのメモリ確保ができない

        min_prev = metadata->previous;
        min_metadata = metadata;
        min_size = min_metadata->size;
        best_fit_found = 1; // このリストの中でfitするものは一応見つかった

      }
      
      metadata = metadata->next; // 次の探索へ
      
    }

    if (best_fit_found == 1){// もしbest_fitが見つかっていたらfor文を抜ける
      break;
    }
  }// for文抜け

  metadata = min_metadata; // metadataをmin_metadataに変更

  // now, metadata points to the best free slot
  // and prev is the previous entry.

  if (!min_metadata) { // metadataがNULLだった（OSからメモリをもらわなきゃいけない）
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

    metadata->left = NULL;
    metadata->right = NULL;
    metadata->is_free = true;
    // Add the memory region to the free list.

    my_add_to_free_list(metadata); // ここは一度しか呼ばれていない
    // Now, try my_malloc() again. This should succeed.
    return my_malloc(size);
  }

  // |ptr| is the beginning of the allocated object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr
  void *ptr = min_metadata + 1;
  size_t remaining_size = min_metadata->size - size;
  // Remove the free slot from the free list.

  my_remove_from_free_list(min_metadata, min_prev);

  min_metadata -> is_free = false;

  if (remaining_size > sizeof(my_metadata_t)) { // 残っている空き領域がもうメタデータも置けないくらい小さいというわけではなければ

    // Shrink the metadata for the allocated object
    // to separate the rest of the region corresponding to remaining_size.
    // If the remaining_size is not large enough to make a new metadata,
    // this code path will not be taken and the region will be managed
    // as a part of the allocated object.
    min_metadata -> size = size;
    //metadata -> is_free = false;
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
    // metadataは確保済み領域のメタデータを表す。
    my_metadata_t *old_right=NULL;
    old_right = min_metadata -> right; // metadataが右端の領域だったときにold_rightはNULLになる

    min_metadata -> right = new_metadata;
    min_metadata -> is_free = false;
    // new_metadataは残った空き領域のメタデータを表す。
    new_metadata -> left = min_metadata;

    new_metadata -> right = old_right; 

    if(old_right != NULL){
        old_right -> left = new_metadata; // ここでセグフォ->old_rightがNULLだった
    }
    
    new_metadata->previous = NULL;
    new_metadata -> is_free = true;

    // Add the remaining free slot to the free list.
    my_metadata_t* left_metadata;
    right_merge(new_metadata); // 右結合
    left_metadata = left_merge(new_metadata); // 左結合
    my_add_to_free_list(left_metadata);
  }
  return ptr;
} // my_mallocここまで

// This is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
    my_metadata_t* left_metadata;

  // Look up the metadata. The metadata is placed just prior to the object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr

  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  metadata -> is_free = true;
  // もし
  if(metadata->size == 4096 - sizeof(my_metadata_t)){
    munmap_to_system(metadata,4096); 
    return;
  }
  // Add the free slot to the free list.
  right_merge(metadata); // 右結合
  left_metadata = left_merge(metadata); // 左結合
  my_add_to_free_list(left_metadata); //　右結合したら空きリストに追加
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

/*
Challenge done!
Please copy & paste the following data in the score sheet!
225,57,265,20,226,30,48,74,73,75,
*/
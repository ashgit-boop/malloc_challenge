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
my_heap_t my_heap[5]; // free list binを作成0~4の添え字になること想定(size/1000を添え字にして分けてみる)

//
// Helper functions (feel free to add/remove/edit!)
//
void my_add_to_free_list(my_metadata_t *metadata);
void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev);


void right_merge(my_metadata_t *metadata){ // 引数はfreeしたメタデータ

  my_metadata_t *right_metadata; // metadataの右側にある領域
  my_metadata_t *old_metadata; // metadataの更新前のやつ
    printf("start right_merge\n");
    //my_metadata_t *next_metadata; // サイズ変更前のmetadataをremoveすると、nextがNULLになっちゃうからNULLになる前の値を保存
    // もしmetadataのメモリ上での右隣の領域が空き領域ではなかったら、何もせずにサヨナラ

    // ここの条件文でセグフォ？ metadata -> rightがない?
    //printf("metadata:%p\n",metadata);
    //printf("metadata->right:%p\n",metadata->right);// metadata -> right がNULLだったからそのis_freeはもちろんセグフォ
    
    if(metadata->right == NULL){ // metadataの右隣がない、つまりmetadataが一番右端っこのとき
        printf("metadata : %p\n",metadata);
        printf("metadata->right : %p\n",metadata->right); 
        my_add_to_free_list(metadata);
        return;
    }

    right_metadata = metadata->right;

    if(right_metadata -> is_free == true){ // もしmetadataの右隣の領域が空き領域だったら、その領域をリスト上で探して削除、merge、空き領域リストに加える
        //printf("if 文に入りました！\n");
        old_metadata = metadata;
        my_remove_from_free_list(right_metadata,right_metadata->previous); // 統合する右側の領域をリストから削除 

        // サイズ変更前の旧metadataをリストから削除しておく
        //printf("before remove in right merge\n");
        my_remove_from_free_list(old_metadata,old_metadata->previous);
        // 領域を統合
        printf("after remove2\n");
        metadata->size = old_metadata->size + right_metadata->size + sizeof(*right_metadata); // サイズを拡張
        // サイズを更新したmetadataを入れ直す
        //my_add_to_free_list(metadata); 
        //printf("after updating size\n");
        metadata->right = right_metadata->right;
        //printf("after changing right\n");
        //printf("metadata->right:%p\n",metadata->right);
        if(metadata->right != NULL){
            metadata->right->left = metadata; // ここでセグフォ、多分metadata->rightがNULLだと思う
        }
        //metadata->right->left = metadata; // ここでセグフォ、多分metadata->rightがNULLだと思う
        //printf("after changing left of the right\n");
        //metadata->next = NULL; // ここをNULLにしておかないと、次のmy_add_to_free_list でassertionに引っかかる->my_removeですでにNULLにされているのでは
        // サイズを更新したmetadataを入れ直す
        //my_add_to_free_list(metadata);
        //printf("after add in my_free()\n"); 
        //printf("metadata->right : %p\n",metadata->right); // こいつがNULL
        if(metadata->right != NULL && metadata -> right -> is_free == true){ // 統合したあとのメモリ領域上で、さらに右隣が空き領域のとき、もう一度right_mergeする。
            //printf("metadata : %p\n",metadata);
            right_merge(metadata);
            printf("after add in if in my_free()\n");
        }
        printf("after if in my_free()\n");
    }
    //printf("metadata : %p\n",metadata);
    my_add_to_free_list(metadata);
    return;
}


// freeされた領域のメタデータを受け取り、空きリストに追加
void my_add_to_free_list(my_metadata_t *metadata) {
  printf("my_add_to_free_list\n");
  //printf("metadata:%p\n",metadata);
  //printf("metadata->size:%ld\n",metadata->size);
  int idx; // free_list binの添え字をidxとする

  //printf("metadata->size:%ld\n",metadata->size);

  assert(!metadata->next);

  idx = metadata->size / 256;
  if(idx>= 4){
    idx = 4;
  }
  metadata->previous = NULL;
  metadata->next = my_heap[idx].free_head; // 先頭に（から）追加していくイメージ
  if(metadata->next != NULL){
    metadata->next->previous = metadata;
  }
  printf("after metadata->next = my_heap[idx].free_head\n");
  my_heap[idx].free_head = metadata;
  metadata->is_free = true;
}


// free_listに入っていたメタデータを（使うことになったから）空きリストから削除する
void my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) {
  printf("my_remove_from_free_list\n");
  int idx;

  if (prev) { // もし消すやつが先頭じゃなくてprevがいれば
    //printf("if\n");
    prev->next = metadata->next;
    if(metadata->next != NULL){        
        metadata->next->previous = prev;
    }
    //metadata->previous = NULL;
  } 
  else { // もし消すやつが先頭だったら(metadata->previouはすでにNULLのハズ)
    //printf("else\n");
    idx = metadata->size / 256;
    //printf("after deciding idx in else\n");
    if(idx>=4){
      idx = 4;
    }
    //printf("before removing metadata\n");
    my_heap[idx].free_head = metadata->next;
    //printf("after removing metadata\n");
  }
  metadata->previous = NULL;
  //printf("before metadata->next = NULL\n");
  metadata->next = NULL;
  //printf("after metadata->next = NULL\n");
  //metadata -> is_free = false;
  //printf("after metadata -> is_free = false\n");
}

//
// Interfaces of malloc (DO NOT RENAME FOLLOWING FUNCTIONS!)
//


void my_initialize() {
  //printf("my_initialize\n");
  for(int idx=0;idx<5;idx++){
  my_heap[idx].free_head = &my_heap[idx].dummy;
  my_heap[idx].dummy.size = 0;
  my_heap[idx].dummy.next = NULL;
  my_heap[idx].dummy.previous = NULL;
  my_heap[idx].dummy.is_free = true; // 本当に？
  }
} 



// my_malloc() is called every time an object is allocated.
// |size| is guaranteed to be a multiple of 8 bytes and meets 8 <= |size| <=
// 4000. You are not allowed to use any library functions other than
// mmap_from_system() / munmap_to_system().

void *my_malloc(size_t size) {
  printf("my_malloc\n");

  //printf("mallocしたいsize:%ld\n",size);
  my_metadata_t *metadata; //= my_heap.free_head;
  my_metadata_t *prev = NULL;
  my_metadata_t *min_metadata = NULL;
  my_metadata_t *min_prev = NULL;
  size_t min_size = SIZE_MAX; // 最初はこの値より小さいサイズの空き領域をmin_metadataにする
  int index;
  
  index=size / 256;
  if(index >= 4){
    index = 4;
  }

  //printf("1\n");
  for(int idx=index;idx<5;idx++){
    metadata = my_heap[idx].free_head;

    prev = NULL;
    min_prev = NULL;
    int best_fit_found = 0; // bestfitが見つかった時に1、見つかっていないときには０

    while (metadata!=NULL) { 
      //printf("while\n");
      // このif文の条件は、最初はmetadata == dummyだからmetadata->size = 0になる。if文に入らない
      //printf("size:%ld,metadata->size:%ld,min_size:%d\n",size,metadata->size,min_size);
      printf("first  metadata : %p\n",metadata);
      if(size <= metadata->size && metadata->size < min_size ){ // この最初の条件size <= metadata->sizeを忘れると正しい大きさのメモリ確保ができない
        printf("start if in while in my_malloc()\n");
        min_prev = prev;
        min_metadata = metadata;
        //printf("min_metadata : %p\n",min_metadata);
        min_size = min_metadata->size;
        //printf("after if in while in my_malloc()\n");
        best_fit_found = 1; // このリストの中でfitするものは一応見つかった

      }
      //printf("after if in while in my_malloc()\n");
      prev = metadata;
      printf("metadata : %p\n",metadata);
      
      metadata = metadata->next;
      
    }
    printf("after while in my_malloc()\n");
    //printf("idx:%d\n",idx);
    if (best_fit_found == 1){// もしbest_fitが見つかっていたらfor文を抜ける
      //printf("break\n");
      break;
    }
  }// for文抜け
  //printf("min_size:%ld\n",min_size);

  prev = min_prev; // prevをmin_metadataのprevに変更
  metadata = min_metadata; // metadataをmin_metadataに変更

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

    metadata->left = NULL;
    metadata->right = NULL;
    metadata->is_free = true;
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
  printf("before remove in my_malloc\n");
  my_remove_from_free_list(metadata, prev);
  printf("after remove in my_malloc\n");
  metadata -> is_free = false;
  printf("after metadata -> is_free = false;\n");

  printf("before if in my_malloc\n");
  if (remaining_size > sizeof(my_metadata_t)) { // 残っている空き領域がもうメタデータも置けないくらい小さいというわけではなければ
    printf("after if in my_malloc\n");
    // Shrink the metadata for the allocated object
    // to separate the rest of the region corresponding to remaining_size.
    // If the remaining_size is not large enough to make a new metadata,
    // this code path will not be taken and the region will be managed
    // as a part of the allocated object.
    metadata -> size = size;
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
    my_metadata_t *old_right;
    old_right = metadata -> right; // metadataが右端の領域だったときにold_rightはNULLになる
    //printf("after making old_right in my_malloc\n");

    metadata -> right = new_metadata;
    //metadata -> left = NULL; // これはまずい
    metadata -> is_free = false;
    // new_metadataは残った空き領域のメタデータを表す。
    //printf("before new_metadata update in my_malloc\n");
    new_metadata -> left = metadata;
    //printf("1 in my_malloc\n");
    new_metadata -> right = old_right; // 本当に？？
    //printf("2 in my_malloc\n");
    //printf("old_right : %p\n",old_right );

    if(old_right != NULL){
        old_right -> left = new_metadata; // ここでセグフォ->old_rightがNULLだった
    }
    
    printf("3 in my_malloc\n");
    new_metadata->previous = NULL;
    new_metadata -> is_free = true;

    // Add the remaining free slot to the free list.
    //printf("new_metadata->size:%ld\n",new_metadata->size);
    printf("before my_add in my_malloc\n");
    my_add_to_free_list(new_metadata);
    printf("after my_add in my_malloc\n");
  }
  return ptr;
} // my_mallocここまで

// This is called every time an object is freed.  You are not allowed to
// use any library functions other than mmap_from_system / munmap_to_system.
void my_free(void *ptr) {
  printf("my_free\n");
  // Look up the metadata. The metadata is placed just prior to the object.
  //
  // ... | metadata | object | ...
  //     ^          ^
  //     metadata   ptr

  my_metadata_t *metadata = (my_metadata_t *)ptr - 1;
  
  // Add the free slot to the free list.
  printf("before add in my_free()\n");
  //my_add_to_free_list(metadata);
  //if(right_merge(metadata)==false){
  printf("before right_merge in my_free()\n");
  
  right_merge(metadata); // right_mergeがなければassertなしで"An allocated object is broken!"もなしで動きそう->構造体のright,leftの管理がうまく行っていないもしくはright_mergeの構造がやらかしている
  printf("finished right_merge\n");
  //my_add_to_free_list(metadata);
  //}
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

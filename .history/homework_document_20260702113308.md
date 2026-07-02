# Step Week7 HomeWork
## Objective: Implement malloc  
## Functions:
- ### my_malloc(size_t size) :
    ### Allocate memory. This function takes the size of metadata to allocate as an argument. If there was no memory to be allocated, get a new memory area from OS.
```c
void *my_malloc(size_t size) {

  my_metadata_t *metadata; 
  my_metadata_t *min_metadata=NULL;
  my_metadata_t *min_prev=NULL;
  size_t min_size = 10000; // 最初はこの値より小さいサイズの空き領域をmin_metadataにする
  int index;
  
  index = calculate_index(size);

  for(int idx=index;idx<5;idx++){
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

    // 残っている空き領域がもうメタデータも置けないくらい小さいというわけではなければ
  if (remaining_size > sizeof(my_metadata_t)) { 

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
    my_add_to_free_list(new_metadata);
  }
  return ptr;
} // my_mallocここまで
```
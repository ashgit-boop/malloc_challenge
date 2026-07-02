# Step Week7 HomeWork
## Objective: Implement malloc !!
## Functions:


- ### void right_merge(my_metadata_t *metadata) : 
    #### Implement merge to the right. This function takes metadata which was freed as an argument.
    ```c
    // 右結合をする
    void right_merge(my_metadata_t *metadata){ // 引数はfreeしたメタデータ

    my_metadata_t *right_metadata; // metadataの右側にある領域

        // もしmetadataのメモリ上での右隣の領域が空き領域ではなかったら、何もせずにサヨナラ

        
        if(metadata->right == NULL){ // metadataの右隣がない、つまりmetadataが一番右端っこのとき右側マージできない
            return;
        }

        right_metadata = metadata->right; // この時点でright_metadataはNULLでないことが確定

        if(right_metadata -> is_free == true){ // もしmetadataの右隣の領域が空き領域だったら、その領域をリスト上で探して空き領域リストに加える
            //old_metadata = metadata;
            my_remove_from_free_list(right_metadata,right_metadata->previous); // 統合する右側の領域をリストから削除 

            // 領域を統合
            metadata->size = metadata->size + right_metadata->size + sizeof(*right_metadata); // サイズを拡張
            // サイズを更新したmetadataを入れ直す
            metadata->right = right_metadata->right;

        
            // 統合したあとのメモリ領域上で、さらに右隣が空き領域のとき、もう一度right_mergeする。
            if(metadata->right != NULL && metadata -> right -> is_free == true){ 
                metadata->right->left = metadata;
                right_merge(metadata);
            }
        }
        return;
    }

    ```

- ### void initialize() : 
    #### Initialize the information of the my_heap.
    ```c
    void my_initialize() {
        for(int idx=0;idx<5;idx++){
        my_heap[idx].free_head = &my_heap[idx].dummy; // 先頭はdummy
        my_heap[idx].dummy.size = 0; // dummyのサイズは0
        my_heap[idx].dummy.next = NULL; // dummyの次のノードは無い
        my_heap[idx].dummy.previous = NULL; // dummyの前のノードは無い
        my_heap[idx].dummy.is_free = false; // このノードは使えない
        }
    } 
    ```    
- ### int calculate_index(size_t size) :
     #### Calculate the index of the free_list bin from given size.

    ```c
    // 与えられたメモリのサイズから、free_list_binのどこのindexのリストに入るかを計算
    int calculate_index(size_t size){
        int index;

        index = size / 256;
        
        if(index >= 4){
            index = 4;
        }
        return index;
    }
    ``` 
- ### void *my_malloc(size_t size) :
    #### Allocate memory. This function takes the size of metadata to allocate as an argument. If there was no memory to be allocated, get a new memory area from OS and allocate memory.
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


- ### my_free(void *ptr) :
    #### Free memory which was specified by the ptr. ptr is the pointer which points the head of the memory.
    
    ```c
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
    right_merge(metadata); // 右結合
    my_add_to_free_list(metadata); //　右結合したら空きリストに追加
    }
    ```

# Step Week7 HomeWork
## Objective: Implement malloc !!
malloc_bestfit.c , malloc_free_list_bin.c ,malloc_right_merge.c , malloc_right_and_left_merge.c
## Functions:

- ### typedef struct my_metadata_t : 
    - #### Created the `is_free` so that immediately determine (without checking the `free_list`) whether the area pointed to by `metadata` is free.
    - #### Created `next` , `previous` , `left` and `right` to reduce the times of calculations.
    - #### I thought that the memory utilization wasn't increasing because this structure is so large.
    ```c
    // 空き領域の前に、その領域のサイズと次の空き領域へのポインタの情報を置く。
    typedef struct my_metadata_t { // ここの構造体のサイズが大きいからutilization上がらないんじゃないか
    size_t size; // サイズ
    struct my_metadata_t *next; // リスト上における次のmeta_dataへのポインタ
    struct my_metadata_t *previous; // リスト上における前のmetadataへのポインタ
    struct my_metadata_t *left; // メモリ上で左隣にある領域のメタデータ 
    struct my_metadata_t *right; // メモリ上で右隣にある領域のメタデータ
    bool is_free; // このメタデータをもつ領域が空き領域かどうかを表す
    } my_metadata_t; // 
    ```
- ### int calculate_index(size_t size) :
     - #### Calculate the index of the free list bin from given size.
     - #### I adjusted the number of bins and tried various values, but when the size of the `my_heap_t` array was 50 and the number of beams was 80, I found the best balance between speed and utilization.

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

- ### my_metadata_t* left_merge(my_metadata_t *metadata):
    - #### Implement merge to the left. This function takes metadata which was freed as an argument, and return `left_metadata`, which is the metadata on the far left.
    - #### If the area immediately to the left of the `metadata` in memory is not free, do nothing.
    - #### If the area to the left of that is also free space, do `left_merge()` again.

    ```c
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


    ```

- ### void right_merge(my_metadata_t *metadata) : 
    - #### Implement merge to the right. This function takes `metadata` which was freed as an argument.
    - #### If the area immediately to the right of the `metadata` in memory is not free, do nothing.
    - #### If the area to the left of that is also free space, do `right_merge()` again.
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
    #### Initialize the information of the `my_heap[]`.
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

- ### void my_add_to_free_list(my_metadata_t *metadata) : 
    - #### Add metadata which was freed to the appropriate free_list bin.

    ```c
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
    ```    


- ### void *my_remove_from_free_list(my_metadata_t *metadata, my_metadata_t *prev) :  
    - #### Remove specified metadata from free_list.
    ```c
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
    }
    ```     

- ### void *my_malloc(size_t size) :
    - #### Allocate memory. This function takes the size of metadata to allocate as an argument. If there was no memory to be allocated, get a new memory area from OS and allocate memory.
    - #### In `my_heap[]`, the size of a node increases as the index increases, so once an element that meets the condition is found, there is no need to search further through the list.(best-fit)
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
    - #### Free memory which was specified by the `ptr`. `ptr` is the pointer which points the head of the memory.
    - #### If the sum of the size of the released metadata and the size of the metadata itself matches the total memory size of the page, return the memory to the OS.(munmap_to_system())
    - #### Do `right_merge()` , `left_merge()`, and `add_to_free_list()`
    
    ```c
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
    ```


## What I found :
- #### After applying best-fit, utilization got better sharply.
- #### After applying free_list_bin, the speed got faster.
- #### After applying `right_merge()` and `left_merge()`, the utilization wasn't good for Challenges 1–3.(The utilization when applying best-fit and free_list_bin was much better.) For Challenges 4 and 5, the values were slightly better, but there wasn't much of a difference.
- #### I thought that the memory utilization wasn't increasing because this structure is so large. Perhaps the `right` in this structure is not necessary. I checked the size of this structure. The result is as follows.
    ```c
    int main(){
    typedef struct my_metadata_t_1 { // ここの構造体のサイズが大きいからutilization上がらないんじゃないか
    size_t size; // サイズ
    struct my_metadata_t_1 *next; // リスト上における次のmeta_dataへのポインタ
    struct my_metadata_t_1 *previous; // リスト上における前のmetadataへのポインタ
    struct my_metadata_t_1 *left; // メモリ上で左隣にある領域のメタデータ 
    struct my_metadata_t_1 *right; // メモリ上で右隣にある領域のメタデータ
    bool is_free; // このメタデータをもつ領域が空き領域かどうかを表す
    } my_metadata_t_1;


    typedef struct my_metadata_t_2 { // ここの構造体のサイズが大きいからutilization上がらないんじゃないか
    size_t size; // サイズ
    struct my_metadata_t_2 *next; // リスト上における次のmeta_dataへのポインタ
    struct my_metadata_t_2 *previous; // リスト上における前のmetadataへのポインタ
    struct my_metadata_t_2 *left; // メモリ上で左隣にある領域のメタデータ 
    //struct my_metadata_t *right; // メモリ上で右隣にある領域のメタデータ
    bool is_free; // このメタデータをもつ領域が空き領域かどうかを表す -> 消す
    } my_metadata_t_2;


        printf("sizeof(my_metadata_t_1):%ld\n",sizeof(my_metadata_t_1));
        printf("sizeof(my_metadata_t_2):%ld\n",sizeof(my_metadata_t_2));
        /*
        sizeof(my_metadata_t_1):48
        sizeof(my_metadata_t_2):40
        */

    }
    ```
    #### The difference between the 2 structure is 8B. If this 8B were not, the utilization will soar.

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<limits.h>
#include<math.h>

typedef struct my_metadata_t_1 { // ここの構造体のサイズが大きいからutilization上がらないんじゃないか
  size_t size; // サイズ
  struct my_metadata_t *next; // リスト上における次のmeta_dataへのポインタ
  struct my_metadata_t *previous; // リスト上における前のmetadataへのポインタ
  struct my_metadata_t *left; // メモリ上で左隣にある領域のメタデータ 
  struct my_metadata_t *right; // メモリ上で右隣にある領域のメタデータ
  bool is_free; // このメタデータをもつ領域が空き領域かどうかを表す
} my_metadata_t;


typedef struct my_metadata_t_2 { // ここの構造体のサイズが大きいからutilization上がらないんじゃないか
  size_t size; // サイズ
  struct my_metadata_t *next; // リスト上における次のmeta_dataへのポインタ
  struct my_metadata_t *previous; // リスト上における前のmetadataへのポインタ
  struct my_metadata_t *left; // メモリ上で左隣にある領域のメタデータ 
  struct my_metadata_t *right; // メモリ上で右隣にある領域のメタデータ
  bool is_free; // このメタデータをもつ領域が空き領域かどうかを表す
} my_metadata_t;
#include <sys/mman.h>
#include <stddef.h>


// INIT with 4 * 16-bytes, 2 * 32-bytes blocks 
//  int - 4 bytes
//  pointer - 8 bytes
//  BlockHeader - 4 + 4 + 8 = 16 bytes ( damn easy multiple :) )

typedef struct BlockHeader {
  size_t size;
  int is_free;
  struct BlockHeader *next_block;
} block_header_t;


void * req_mmap(size_t rsize) {
  void * ptr = mmap(
    NULL, 
    rsize,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS,
    -1,
    0
  );

  if (ptr == MAP_FAILED) {
    return NULL;
  }

  return ptr;

}



block_header_t *firstheader;//  = req_mmap(sizeof(block_header_t));
void init() {
  firstheader = req_mmap(sizeof(block_header_t));
  (*firstheader).size = 0;
  (*firstheader).is_free = 0;
  (*firstheader).next_block = firstheader + 1;
}

int chain_length =1;




void * my_malloc(size_t rsize) {
  block_header_t *nblock;
  block_header_t *cblock = firstheader;
  size_t reqsize = 8*((rsize+sizeof(block_header_t)+7) / 8);
  for(int i = 0; i<chain_length;i++) {
    if ((*cblock).is_free==1 && (*cblock).size >= rsize){
      if ((*cblock).size  >= (reqsize + (sizeof(block_header_t) + sizeof(1)))) {
        chain_length++;
        (cblock->next_block) = cblock + ((reqsize)/sizeof(*cblock));
      }
      return cblock + 1;
    }else{
      cblock = (*cblock).next_block;
    }    
  };


  nblock = req_mmap(reqsize);
  cblock->next_block = nblock;
  (*nblock).size = reqsize - sizeof(block_header_t);
  (*nblock).is_free = 0;
  (*nblock).next_block = nblock + reqsize;
  chain_length++;
  return  nblock + 1;
}

void my_free(void *ptr) {
  block_header_t *cblock = firstheader;
  block_header_t *nblock;
  for (int i = 0; i < chain_length; i++) {
    if ((cblock + 1) == ptr) {
      cblock->is_free = 1;
      break;
    }else {
      nblock = (cblock->next_block);
    }
  }
  cblock = firstheader;
  if (chain_length > 1){
    nblock = (cblock->next_block);
  }
  for (int i = 0; i < chain_length; i++) {
    if ((cblock->is_free * nblock->is_free)==1){
      cblock->size += nblock->size + sizeof(block_header_t);
      chain_length--;
      cblock->next_block = nblock->next_block;
    }
  }
}

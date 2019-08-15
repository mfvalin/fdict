/*
   Copyright (C) 2019, Recherche en Prevision Numerique

   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   
   uses xxHash (Copyright (C) 2019-present, Yann Collet)
   xxHash source repository : https://github.com/Cyan4973/xxHash
*/



#include <xxh3.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

XXH64_hash_t FastHash64(const void* data, uint32_t l){
  size_t len = l;
  return XXH3_64bits(data, len);
}

XXH128_hash_t FastHash128(const void* data, uint32_t l){
  size_t len = l;
  return XXH3_128bits(data, len);
}

#define DEFAULT_DATA_PAGE 32768
struct data_save{              // saved data page
  struct data_save *next;      // pointer to next page
  uint8_t *last;               // pointer to last data inserted
  uint8_t *end;                // pointer to end of buffer
  uint8_t *cur;                // pointer to insertion point
  uint8_t  data[0];            // data buffer
};
typedef struct data_save data_save;

struct hash_entry{             // hash table entry
  uint64_t hash;               // 64 bit hash of key
  void *key;                   // pointer to key
  void *data;                  // pointer to data
  struct hash_entry *next;     // link to next hash entry for a key with same hash (bucket hash)
  uint32_t keylen;             // length (in bytes) of key
  uint32_t datalen;            // length (in bytes) of data
};
typedef struct hash_entry hash_entry;

#define HASH_PAGE_SIZE 1024
struct hash_page{              // hash table page
  struct hash_page *next;      // pointer to next page of hash entried
  int32_t last;                // last valid entry in page
  int32_t n;                   // max number of entries in page
  hash_entry e[HASH_PAGE_SIZE];// hash entries
};
typedef struct hash_page hash_page;

// N.B. the number of uint32_t at beginning of struct should be EVEN for alignment purposes on 64 bit platforms
typedef struct{                // hash context
  uint32_t tsize;              // max number of entries expected in array p
  uint32_t hashtype;           // hash algorithm used (1 for now)
  uint32_t mask;               // mask used to convert hash into index into array p
  uint32_t nbits;              // shift count used to convert hash into index into array p
  uint32_t ncoll;              // number of collisions
  uint32_t nentr;              // number of entries in table
  uint32_t depth;              // max depth of collision bucket
  uint32_t match;              // number of successful matches
  data_save *keys;             // pointer to saved keys (may be NULL)
  hash_page *first;            // first hash page in chain
  hash_page *cur;              // current hash page being filled
  hash_entry *p[0];            // table of pointers to hash entries (size = tsize)
}hash_context;

// create a new saved data page, link old_page to it
// old_page   : pointer to current saved data page
// pgsz       : size in bytes of new data page buffer
// return pointer to new saved data page
// NULL is returned if an error occurs
static data_save *data_newpage(data_save *old_page, uint32_t pgsz){ 
  data_save *temp;

  temp = (data_save *)malloc( sizeof(data_save) + pgsz );  // try to allocate new saved data page of size pgsz
  if(DEBUG > 0) printf("new data page allocated, len = %d\n",pgsz);
  if(temp != NULL){
    temp->last = NULL;                // no last entry
    temp->end = &(temp->data[pgsz]);  // page size
    temp->cur = &(temp->data[0]);     // insertion index at start of page
    temp->next = old_page;            // link to old page
  }
  return temp;  // will return NULL if malloc failed
}

// copy data into saved data pages, allocate new page if needed
// old_page   : pointer to current saved data page
// data       : pointer to data to be saved
// sz         : size of data
// return pointer to (possibbly new) data page
// NULL is returned if an error occurs
static data_save *data_copy(data_save *old_page, void *data, uint32_t sz){  
  data_save *temp = old_page;
  uint32_t newpgsz;

  if(DEBUG > 0) printf("data_copy: copying %d bytes\n",sz);
  if(temp == NULL) return temp;     // ERROR
  newpgsz = old_page->end - &(old_page->data[0]); // new page size = old page size by default
  if(newpgsz < sz) newpgsz = sz;                  // new page size must be large enough to contain at least sz bytes

  if(old_page->cur + sz > old_page->end){       // data will not fit into current data page
  if(DEBUG > 0) printf("NO FIT, need new page\n");
    temp = data_newpage(old_page, newpgsz);       // allocate a new data page
    if(temp == NULL) return NULL;                 // ERROR
  }
  temp->last = temp->cur;
  memcpy( temp->last, data, sz);                  // copy data into saved data page
  temp->cur += sz;                                // bump current data pointer by data size
  return temp;
}

// create a new empty hash table page
// return pointer ot initialized new page, NULL if an error occurred
static hash_page *hash_newpage(){   
  hash_page *temp = (hash_page *) malloc(sizeof(hash_page));
  if(temp != NULL){
    temp->next = NULL;
    temp->last = 0;
    temp->n = HASH_PAGE_SIZE;
  }
  return temp;  // will return NULL if malloc failed
}

// return pointer to a new hash entry, NULL if error
static hash_entry *new_hash_entry(hash_context *c){
  hash_page *p;
  hash_entry *e = NULL;

  if(c == NULL) return NULL;
  p = c->cur;
  if(p->last >= p->n){           // current page is full
    p = hash_newpage();          // get a new page
    if(p == NULL) return NULL;   // ERROR
    p->next = c->first;          // link current first page as next page
    c->first = p;                // p becomes first page
    c->cur = p;                  // new current page
  }
  e = &(p->e[p->last]);          // address of last entry
  p->last++;                     // bump last entry number
  return e;
}

// create a hash context
// n    : number of items expected in hash table
// return pointer to hash context , NULL if an error occurred
hash_context *CreateHashContext(int32_t n){
  int i = 1;

  if(n < 0) return NULL;
  while( (1 << i) < n ) i++;     // 1 << i is first power of 2 larger than n
  hash_context *temp = (hash_context *) malloc(sizeof(hash_context) + (1 << i) * sizeof(void *));

  if(temp == NULL) return NULL;  // allocation failed

  temp->first = hash_newpage();  // allocate first page
  if(temp->first == NULL){       // allocation failed
    free(temp);                  // free context pointer
    return NULL;
  }

  temp->keys  = NULL;            // data pages if copies of keys are needed
  temp->cur   = temp->first;     // current page is first page
  temp->tsize = n;               // dictionary size
  temp->hashtype = 1;            // only value for now (xxhash type)
  temp->nbits = i;               // first power of 2 larger than n
  temp->ncoll = 0;               // no collisions so far !!
  temp->nentr = 0;               // no entries in table
  temp->depth = 0;               // max depth of collision bucket
  temp->match = 0;               // number of successful matches
  temp->mask = ~((-1) << temp->nbits);  // mask associated with nbits
  return temp;
}

static int32_t is_same_key(uint8_t *s1, uint8_t *s2, int32_t len){
  int i;
  uint8_t *k1 = (uint8_t *) s1;
  uint8_t *k2 = (uint8_t *) s2;

  for(i=0 ; i<len ; i++) {
    if(k1[i] != k2[i]) return 0;
  }

  return 1;
}

// insert pointers to key and data into hash table (if entry does not already exists)
// c         : pointer to hash context created by create_hash_context
// key       : pointer to key
// keylen    : length of key (in bytes)
// data      : pointer to data associated with key (NULL is OK)
// datalen   : data length in bytes ( >= 0 )
// keycopy   : save a copy of key if keycopy != 0
// return 0 if new entry
// return index in table if entry exists
// return -1 if an error occurred
int32_t HashInsert(hash_context *c, void *key, uint32_t keylen, void *data, uint32_t datalen, uint32_t keycopy){
  uint64_t hash, tmp0, tmp1;
  uint64_t nb = 64;
  hash_entry *e = NULL;
  data_save *old_page;
  uint32_t depth = 1;   // collision bucket depth

  if(keylen < 0 || datalen <0 || key == NULL || c == NULL) return -1; // ERROR

  hash = FastHash64(key, keylen);  // make hash and convert ot index into table
  tmp0 = hash;
  tmp1 = tmp0 & c->mask;
  while(nb > c->nbits) {           // convert 64 bit hash into index into table
    nb = nb - c->nbits;
    tmp0 >>= c->nbits;
    tmp1 ^= (tmp0 & c->mask);
  }
  e = c->p[tmp1];
  if(e != NULL) c->ncoll++;  // we have a collision
  while(e != NULL){          // entry may exist
  if(DEBUG > 0) printf("index collision\n");
    if(e->hash == hash && e->keylen == keylen){     // possible match
      if( is_same_key(key, e->key, keylen) ) {
        c->ncoll--;     // not a collision after all
        c->match++;     // match count
  if(DEBUG > 0) printf("match found\n");
        return tmp1 ;   // match found
      }
    }
    depth++;      // collision bucket depth
    e = e->next;  // try next for match until NULL
  }
  if(depth > c->depth) c->depth = depth;  // keep track of maximum collision bucket depth
  e = new_hash_entry(c); // entry does not exist, insert at position tmp1
  c->nentr++;            // one more entry in table
  e -> key = key;
  e->keylen = keylen;
  e->data = data;
  e->datalen = datalen;
  e->hash = hash;
  if(keycopy){
  if(DEBUG > 0) printf("copying key '%s', len = %d\n",(char *)key, keylen);
    old_page = c->keys;
    if(old_page == NULL) old_page = data_newpage(NULL, keylen < DEFAULT_DATA_PAGE ? DEFAULT_DATA_PAGE : keylen);
    c->keys = data_copy(old_page, key, keylen);   // old_page or address of new page 
    e->key = c->keys->last;
  }
  e->next = c->p[tmp1];
  c->p[tmp1] = e;
  return 0;
}

// dump context statistics
void PrintHashContextStats(hash_context *c, char *header){
  printf("===== %s ===== (Hash Context)\n",header);
  printf("hash table size      = %d (%d)\n", 1<<c->nbits,c->tsize);
  printf("number of entries    = %d\n",c->nentr);
  printf("number of collisions = %d\n",c->ncoll);
  printf("number of duplicates = %d\n",c->match);
  printf("max collision bucket = %d\n",c->depth);
  printf("key copy             = %s\n",c->keys?"YES":"NO");
  printf("index mask           = %8.8x\n",c->mask);
  printf("==============================\n");
}

#if defined (SELF_TEST)

static inline uint64_t rdtsc()  // get tsc/socket/processor
{
   unsigned int a, d, c;
   // rdtscp instruction
   // EDX:EAX contain TimeStampCounter
   // ECX contains IA32_TSC_AUX[31:0] (MSR_TSC_AUX value set by OS, lower 32 bits contain socket+processor)
   __asm__ volatile("rdtscp" : "=a" (a), "=d" (d), "=c" (c));

   return ((uint64_t)a) | (((uint64_t)d) << 32);;
}

static char *str="The canonical representation will solve the issue of identical byte-level representation";
int main(){
  int i, j;
  XXH64_hash_t the_sum;
  uint64_t t0, t1;

  the_sum = 0;
  for(i=32 ; i<128 ; i++){
    t0 = rdtsc();
      for(j=0 ; j<100 ; j++) { the_sum += FastHash64(str+j,10); }
    t1 = rdtsc();
    printf("%5d %Lu \n",i,(unsigned long long)((t1-t0)/100));
  }
  printf("%Lu\n",(unsigned long long)the_sum);
  return 0;
}
#endif

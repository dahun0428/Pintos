#include "userprog/pagedir.h"
#include <hash.h>



struct page{

 struct hash_elem pelem; /* hash_elem in page.c */

 void * addr; /* address of frame ; frame number (?) */

 bool accessed;
 bool dirty;
};




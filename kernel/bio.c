// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
extern uint ticks;

#define lock_lab 1

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

#if lock_lab
static struct buf * cachehashtable[HASHSIZ];
static struct spinlock cachelocks[HASHSIZ];

void
binit(void)
{
  initlock(&bcache.lock, "bcache");
  for (int i=0; i< HASHSIZ; i++)
  {
    initcachehashlock(&cachelocks[i], "bcache_%d", i);
    cachehashtable[i] = &bcache.head;
  }
/*
  for (int i=0; i<HASHSIZ; i++)
  {
    printf("%s\n",cachelocks[i].name);
  }
*/
}

void
evict(struct buf * be)
{
  int id=be->blockno % HASHSIZ;
  struct buf * b;
  struct buf * bpre;
  acquire(&cachelocks[id]);
  for (bpre=b=cachehashtable[id]; b!=&bcache.head; bpre=b, b=b->next)
  {
    //if (b->dev == be->dev && b->blockno == be->blockno)
    if ( b == be)
    {
      if (bpre==b)
      {
        cachehashtable[id]=be->next;
      }
      else
      {
        bpre->next = be->next;
      }
      //printf("index: %d, evict, block_address: %p， b->next address: %p\n", id, be, be->next);
      break;
    }
  }
  release(&cachelocks[id]);
}

void
install(struct buf *b)
{
  int id = b->blockno % HASHSIZ;
  acquire(&cachelocks[id]);
  b->next = cachehashtable[id];
  cachehashtable[id]=b;
  //printf("index: %d, install, block address: %p, b->next address: %p\n", id, b, b->next);
  release(&cachelocks[id]);
}
int
find(struct buf *bf)
{
  struct buf *b;
  int id = bf->blockno % HASHSIZ;  
  acquire(&cachelocks[id]);
  for(b=cachehashtable[id]; b!=&bcache.head; b=b->next)
  {
    if(b==bf)
    {
      release(&cachelocks[id]);
      return 1;
    }
  }
  release(&cachelocks[id]);
  return 0;

}

static struct buf *
bget(uint dev, uint blockno)
{
  int id=blockno % HASHSIZ;
  struct buf * b;
  struct buf * bcan;
  struct buf * bcan_min;
  /*
  int ref_0_count=0;
  uint ticksmin;
  */
  acquire(&cachelocks[id]);
  //int temp=0;
  for (b=cachehashtable[id]; b!= &bcache.head; b=b->next)
  {
    //printf("index: %d, loop: %d, block address: %p, block no: %d", id, temp++,b, b->blockno);
    if (b->blockno == blockno && b->dev == dev)
    {
      //printf("match, dev:%d blockno:%d\n", b->dev, b->blockno);
      b->refcnt++;
      release(&cachelocks[id]);
      acquiresleep(&b->lock);
      return b;
    }
    /*else
    {
      printf(" not match,");
    }
    printf("\n");*/
  }
  release(&cachelocks[id]);

  acquire(&bcache.lock);
  for (int i=0; i<NBUF; i++)
  {
    //printf("index: %d, loop: %d, block address: %p exit\n", id,temp,b);
    bcan=&bcache.buf[i];
    if (bcan->refcnt == 0)
    {
    /* 
      if (ref_0_count==0)
      {
        ticksmin= bcan->b_ticks;
        bcan_min=bcan;
      }
      else if (bcan->b_ticks < ticksmin)
      {
        ticksmin= bcan->b_ticks;
        bcan_min=bcan;
      }
      ref_0_count++;
      if (i < NBUF-1)
        continue;
    */
      //printf("candidte: %p, can_idex: %d, b_idex:%d can_blockno: %d \n", bcan, bcan->blockno % HASHSIZ, id, bcan->blockno);
      bcan_min=bcan;
      if (bcan_min->blockno % HASHSIZ != id)
      {
        bcan_min->valid = 0;
        bcan_min->refcnt = 1;
        bcan_min->dev = dev;
        evict(bcan);
        bcan_min->blockno = blockno;
        install(bcan_min);
      }
      else
      {
        bcan_min->valid = 0;
        bcan_min->refcnt = 1;
        bcan_min->blockno = blockno;
        if(!find(bcan_min))
          install(bcan_min);
      }
      release(&bcache.lock);
      acquiresleep(&bcan_min->lock);
      return bcan_min;
    }
  }
  panic("bget: no buffers");
}

void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  //acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    b->b_ticks = ticks;
  }
  //release(&bcache.lock);
  releasesleep(&b->lock);
}

#endif

#if !lock_lab
void
binit(void)
{
  struct buf *b;
  initlock(&bcache.lock, "bcache");
  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}



// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  release(&bcache.lock);
}

#endif

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}


void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}



5.3.4 Buffer Cache

Modify the file system to keep a cache of file blocks. When a request is made to read or write a block, check to see if it is in the cache, and if so, use the cached data without going to disk. 
"check inode cache_list "

Otherwise, fetch the block from disk into the cache, evicting an older entry if necessary. You are limited to a cache no greater than 64 sectors in size.
"eviction and allocate and update_cache_list"

You must implement a cache replacement algorithm that is at least as good as the "clock" algorithm. We encourage you to account for the generally greater value of metadata compared to data. Experiment to see what combination of accessed, dirty, and other information results in the best performance, as measured by the number of disk accesses.
1. FIFO isn't allowed?
2. accessed, dirty -> enhance 2nd chance

You can keep a cached copy of the free map permanently in memory if you like. It doesn't have to count against the cache size.
"check later"

The provided inode code uses a "bounce buffer" allocated with malloc() to translate the disk's sector-by-sector interface into the system call interface's byte-by-byte interface. You should get rid of these bounce buffers. Instead, copy data into and out of sectors in the buffer cache directly.
"New cache struct"

Your cache should be write-behind, that is, keep dirty blocks in the cache, instead of immediately writing modified data to disk. 
"Use timer_interrupt"

Write dirty blocks to disk whenever they are evicted. Because write-behind makes your file system more fragile in the face of crashes, in addition you should periodically write all dirty, cached blocks back to disk. 
"needless to say"

The cache should also be written back to disk in filesys_done(), so that halting Pintos flushes the cache.
"filesys_done is called when shutdown_halt runs, also needless to say"

If you have timer_sleep() from the first project working, write-behind is an excellent application. Otherwise, you may implement a less general facility, but make sure that it does not exhibit busy-waiting.
"I did"

You should also implement read-ahead, that is, automatically fetch the next block of a file into the cache when one block of a file is read, in case that block is about to be read. Read-ahead is only really useful when done asynchronously. That means, if a process requests disk block 1 from the file, it should block until disk block 1 is read in, but once that read is complete, control should return to the process immediately. The read-ahead request for disk block 2 should be handled asynchronously, in the background.
"after indexing maybe?"

We recommend integrating the cache into your design early. In the past, many groups have tried to tack the cache onto a design late in the design process. This is very difficult. These groups have often turned in projects that failed most or all of the tests.







void thread_fun (void * aux)
{


}




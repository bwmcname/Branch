struct Arena
{
   size_t size;
   u8 *base;
   u8 *current;
};

struct StackAllocator
{
   struct Chunk
   {
      Chunk *last;
      u8 *base;
      size_t size;
   };

   Chunk *last;
   u8 *base;
   u32 allocs;
   
   inline u8 *push(size_t size);
   void pop();
};

static inline
void InitStackAllocator(StackAllocator *allocator)
{      
   allocator->base = (u8 *)allocator + sizeof(StackAllocator);
   allocator->last = (StackAllocator::Chunk *)allocator->base;
   allocator->last->size = 0;
   allocator->last->base = (u8 *)allocator->last;
}

inline
u8 *StackAllocator::push(size_t allocation)
{
   u8 *ptr = (u8 *)(last->base + last->size);
   // align the pointer on an 8 byte boundary
   size_t align = 8 - ((size_t)ptr & 0x7);
   Chunk *newChunk = (Chunk *)(ptr + align);
   
   newChunk->base = ((u8 *)newChunk) + sizeof(Chunk);
   newChunk->size = allocation;
   newChunk->last = last;

   last = newChunk;

   ++allocs;

   return newChunk->base;
}

inline
void StackAllocator::pop()
{
   last = last->last;
   --allocs;
}


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
   
   u8 *push(size_t size);
   void pop();
};

struct VirtualCoord
{
   i32 x;
   i32 y;
};


struct LocationInfo
{
   enum
   {
      track = 0x1,
   };
   u8 flags;
   u16 ID;

   inline u8 hasTrack()
   {
      return flags & track;
   }
};

struct VirtualCoordHashTable
{
   struct Element
   {      
      VirtualCoord k;
      LocationInfo v;
      u8 flags;

      enum
      {
	 occupied = 0x1,
      };
   };

   Element *e;
   u32 capacity;
   u32 size;
   u32 misses;
   u32 accesses;
   
   void put(VirtualCoord k, u8 flags, u16 ID);
   LocationInfo get(VirtualCoord k);

private:
   inline u32 IncrementPointer(u32 ptr);
   inline void Swap(u32 a, u32 b);
};


struct Slot
{
   u32 count;
   u32 elements[3];

   enum
   {
      hasTrack = 0x1,
      hasBranch = 0x2,
   };

   inline u32 GetCombinedFlags();
   inline u32 GetTrackIndex();
};

#ifdef DEBUG
void CheckSlot(Slot s)
{
   i32 trackCount = 0;
   for(u32 i = 0; i < s.count; ++i)
   {
      if(s.elements[i] & Slot::hasTrack) ++trackCount;
   }

   assert(trackCount < 2);
}
#define DEBUG_CHECK_SLOT(s) CheckSlot(s)
#elif
#define DEBUG_CHECK_SLOT(s)
#endif


inline u32 Slot::GetCombinedFlags()
{
   DEBUG_CHECK_SLOT(*this);
   return elements[0] | elements[1] | elements[2];
}

inline u32 Slot::GetTrackIndex()
{
   DEBUG_CHECK_SLOT(*this);

   // assert(count == 1);
   for(u32 i = 0; i < count; ++i)
   {
      if(elements[i] & hasTrack) return elements[i] >> 2;
   }

   assert(false);
   return 0;
}

struct VirtualCoordHashTable
{
   struct Element
   {      
      VirtualCoord k;
      Slot v;
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
   
   VirtualCoordHashTable(u32 size, StackAllocator *allocator);
   void put(VirtualCoord k, u32 v);
   Slot get(VirtualCoord k);

private:
   inline u32 IncrementPointer(u32 ptr);
   inline void Swap(u32 a, u32 b);
};

inline
i32 Compare(VirtualCoord a, VirtualCoord b)
{
   return a.x == b.x && a.y == b.y;
}

inline
u32 HashFunction(VirtualCoord key)
{
   return key.x ^ key.y;
}


void VirtualCoordHashTable::Swap(u32 a, u32 b)
{
   Element temp = e[a];
   e[a] = e[b];
   e[b] = temp;
}

u32 VirtualCoordHashTable::IncrementPointer(u32 ptr)
{
   if(ptr == capacity - 1)
      return 0;
   else
      return ptr + 1;
}

VirtualCoordHashTable::VirtualCoordHashTable(u32 _capacity, StackAllocator *allocator)
{
   e = (Element *)allocator->push(sizeof(Element) * _capacity);
   memset(e, 0, sizeof(Element) * _capacity);
   capacity = _capacity;
   size = 0;

   misses = 0;
   accesses = 0;
}

void VirtualCoordHashTable::put(VirtualCoord k, u32 v)
{
   u32 unbounded = HashFunction(k);
   u32 slot = unbounded % capacity;
   u32 at = slot;
   ++accesses;

   while(e[at].flags & Element::occupied)
   {      
      // add the value to the table
      if(Compare(k, e[at].k))
      {
	 u32 count = e[at].v.count;
	 assert(count < 3);
	 e[at].v.elements[count] = v;
	 ++e[at].v.count;

	 if(at != slot)
	 {
	    ++misses;
	    Swap(slot, at);
	 }
	 return;
      }
      at = IncrementPointer(at);
      assert(at != slot);
   }   

   if(at != slot)
   {
      ++misses;
      // no need to swap since e[at] should be empty
      e[at] = e[slot];
   }
   
   ++size;
   Slot value = {1, {v, 0, 0}};
   e[slot] = {k, value, Element::occupied};
}

Slot VirtualCoordHashTable::get(VirtualCoord k)
{
   ++accesses;
   i32 unbounded = HashFunction(k);
   i32 slot = unbounded % capacity;
   i32 at = slot;

   while(e[at].flags & Element::occupied)
   {
      if(Compare(k, e[at].k))
      {
	 if(at != slot)
	 {
	    ++misses;
	    Swap(slot, at);
	 }
	 return e[slot].v;
      }
      at = IncrementPointer(at);
      assert(at != slot);
   }

   if(at != slot)
   {
      ++misses;
   }
   
   return {0};
}

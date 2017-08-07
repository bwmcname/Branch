
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

VirtualCoordHashTable InitVirtualCoordHashTable(u32 capacity, StackAllocator *allocator)
{
   VirtualCoordHashTable v;
   v.e = (VirtualCoordHashTable::Element *)allocator->push(sizeof(VirtualCoordHashTable::Element) * capacity);
   memset(v.e, 0, sizeof(VirtualCoordHashTable::Element) * capacity);
   v.capacity = capacity;
   v.size = 0;

   v.misses = 0;
   v.accesses = 0;

   return v;
}

void VirtualCoordHashTable::put(VirtualCoord k, u8 flags, u16 ID)
{
   u32 unbounded = HashFunction(k);
   u32 slot = unbounded % capacity;
   u32 at = slot;
   ++accesses;

   while(e[at].flags & Element::occupied)
   {
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
   LocationInfo value = {flags, ID};
   e[slot] = {k, value, Element::occupied};
}

LocationInfo VirtualCoordHashTable::get(VirtualCoord k)
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

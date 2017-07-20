
#define SWAPMAP

#ifdef SWAPMAP
template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
struct HashMap
{
   struct Element
   {      
      K k;
      V v;
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
   
   HashMap(u32 size);
   ~HashMap();
   void put(K k, V v);
   V get(K k);
   void expand();

private:
   inline u32 IncrementPointer(u32 ptr);
   inline void Swap(u32 a, u32 b);
};

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
void HashMap<K, V, HashFunction, Compare, NullValue>::Swap(u32 a, u32 b)
{
   Element temp = e[a];
   e[a] = e[b];
   e[b] = temp;
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
u32 HashMap<K, V, HashFunction, Compare, NullValue>::IncrementPointer(u32 ptr)
{
   if(ptr == capacity - 1)
      return 0;
   else
      return ptr + 1;
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
HashMap<K, V, HashFunction, Compare, NullValue>::HashMap(u32 _capacity)
{
   e = (Element *)malloc(sizeof(Element) * _capacity);
   memset(e, 0, sizeof(Element) * _capacity);
   capacity = _capacity;
   size = 0;

   misses = 0;
   accesses = 0;
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
HashMap<K, V, HashFunction, Compare, NullValue>::~HashMap()
{
   free(e);
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
void HashMap<K, V, HashFunction, Compare, NullValue>::put(K k, V v)
{
   u32 unbounded = HashFunction(k);
   u32 slot = unbounded % capacity;
   u32 at = slot;
   ++accesses;

   while(e[at].flags & Element::occupied)
   {      
      // replace the value if it is already in the table
      if(Compare(k, e[at].k))
      {
	 e[at] = {k, v, Element::occupied};

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
   e[slot] = {k, v, Element::occupied};
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
V HashMap<K, V, HashFunction, Compare, NullValue>::get(K k)
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
   
   return NullValue;
}

#else

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
struct HashMap
{
   struct Element
   {      
      K k;
      V v;
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
   
   HashMap(u32 size);
   ~HashMap();
   void put(K k, V v);
   V get(K k);
   void expand();

private:
   inline u32 IncrementPointer(u32 ptr);
};

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
u32 HashMap<K, V, HashFunction, Compare, NullValue>::IncrementPointer(u32 ptr)
{
   if(ptr == capacity - 1)
      return 0;
   else
      return ptr + 1;
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
HashMap<K, V, HashFunction, Compare, NullValue>::HashMap(u32 _capacity)
{
   e = (Element *)malloc(sizeof(Element) * _capacity);
   memset(e, 0, sizeof(Element) * _capacity);
   capacity = _capacity;
   size = 0;

   misses = 0;
   accesses = 0;
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
HashMap<K, V, HashFunction, Compare, NullValue>::~HashMap()
{
   free(e);
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
void HashMap<K, V, HashFunction, Compare, NullValue>::put(K k, V v)
{
   u32 unbounded = HashFunction(k);
   u32 slot = unbounded % capacity;
   u32 at = slot;
   ++accesses;

   while(e[at].flags & Element::occupied)
   {      
      // replace the value if it is already in the table
      if(Compare(k, e[at].k))
      {
	 e[at] = {k, v, Element::occupied};

	 if(at != slot) ++misses;
	 return;
      }
      at = IncrementPointer(at);
      assert(at != slot);
   }   

   if(at != slot) ++misses;
   ++size;
   e[at] = {k, v, Element::occupied};
}

template <typename K, typename V, u32 HashFunction(K), i32 Compare(K, K), V NullValue>
V HashMap<K, V, HashFunction, Compare, NullValue>::get(K k)
{
   ++accesses;
   i32 unbounded = HashFunction(k);
   i32 slot = unbounded % capacity;
   i32 at = slot;

   while(e[at].flags & Element::occupied)
   {
      if(Compare(k, e[at].k))
      {
	 if(at != slot) ++misses;
	 return e[at].v;
      }
      at = IncrementPointer(at);
      assert(at != slot);
   }

   if(at != slot) ++misses;
   return NullValue;
}

#endif

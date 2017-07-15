
template <typename K, typename V, i32 HashFunction(K), i32 Compare(K, K), V NullValue>
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
   i32 capacity;
   i32 size;
   
   HashMap(i32 size);
   void put(K k, V v);
   V get(K k);
   void expand();
};

template <typename K, typename V, i32 HashFunction(K), i32 Compare(K, K), V NullValue>
HashMap<K, V, HashFunction, Compare, NullValue>::HashMap(i32 _capacity)
{
   e = (Element *)malloc(sizeof(Element) * _capacity);
   memset(e, 0, sizeof(Element) * _capacity);
   capacity = _capacity;
   size = 0;
}

template <typename K, typename V, i32 HashFunction(K), i32 Compare(K, K), V NullValue>
void HashMap<K, V, HashFunction, Compare, NullValue>::put(K k, V v)
{
   i32 unbounded = HashFunction(k);
   i32 slot = unbounded % capacity;
   i32 at = slot;

   while(e[at].flags & Element::occupied)
   {
      ++at;
      assert(at != slot);

      if(Compare(k, e[at].k))
      {
	 e[at] = {k, v, Element::occupied};
	 return;
      }
   }

   ++size;
   e[at] = {k, v, Element::occupied};
}

template <typename K, typename V, i32 HashFunction(K), i32 Compare(K, K), V NullValue>
V HashMap<K, V, HashFunction, Compare, NullValue>::get(K k)
{
   i32 unbounded = HashFunction(k);
   i32 slot = unbounded % capacity;

   while(e[slot].flags & Element::occupied)
   {
      if(Compare(k, e[slot].k))
      {
	 return e[slot].v;
      }
      ++slot;
   }

   return NullValue;
}

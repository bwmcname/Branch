
inline void
Asset::SetPermanent()
{
   flags |= permanent;
}

void
AssetManager::Init(StackAllocator *_allocator)
{
   assets = (Asset *)_allocator->push(sizeof(Asset) * AssetHeader::entries);
   allocator = _allocator;

   packed = FileOpen("assets/Packed.assets");
   
   // clear to zero
   for(u32 i = 0; i < AssetHeader::entries; ++i)
   {
      assets[i] = {};
   }
}

inline void
AssetManager::SetOnGpu(u32 id)
{
   assert(id != 0);
   u32 index = id-1;

   assets[index].flags |= Asset::OnGpu;
}

Asset &
AssetManager::LoadStacked(u32 id)
{
   assert(id != 0);

   u32 index = id-1;
   if(assets[index].flags & Asset::InMem)
      return assets[index];
   
   u32 size = AssetHeader::sizeTable[index];
   assets[index].mem = allocator->push(size);
   FileReadHandle(packed, assets[index].mem, size, AssetHeader::offsetTable[index]);
   assets[index].flags |= Asset::InMem;
   assets[index].size = size;

   LOG_WRITE("Asset Loaded: %u\n", id);

   return assets[index];
}

inline void
AssetManager::PopStacked(u32 id)
{
   assert(id != 0);

   u32 index = id-1;
   assets[index].flags &= ~Asset::InMem;
   allocator->pop();

   LOG_WRITE("Asset Released: %u\n", id);
}

Asset &
AssetManager::LoadManaged(u32 id)
{
   assert(false);
   return assets[id-1];
}

inline
Asset &AssetManager::Get(u32 id)
{
   assert(id != 0);
   return assets[id-1];
}

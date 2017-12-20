
enum ShaderType
{
   Vertex,
   Fragment,
};


union AssetData
{
   struct ShaderData
   {
      ShaderType type;

      void ReleaseFromMemory();
      void ReleaseFromGpu();
   };

   ShaderData shader;
};

struct Asset
{   
   enum
   {
      InMem = 0x1,
      OnGpu = 0x2,
      permanent = 0x4,
   };
   
   u32 flags;
   u32 size;
   u8 *mem;

   inline void SetPermanent();

   AssetData data;
};

struct MapItem
{
   u32 x0, y0, x1, y1;
};

struct AssetManager
{      
   Asset *assets;
   StackAllocator *allocator;
   BranchFileHandle packed;
   Asset &LoadManaged(u32 id);
   Asset &LoadStacked(u32 id);

   // There is no guarentee that when you call this function,
   // that the memory popped will be the asset that matches this id,
   // all this function does is pops the last allocation on the stack,
   // and turns off the InMem bit on the asset
   inline void PopStacked(u32 id);
   
   void FreeManaged(u32 id);
   Asset &Get(u32 id);
   
   void Init(StackAllocator *_allocator);
   inline void SetOnGpu(u32 id);
};

// Common definitions to be used between the runtime and the asset processor


//@note: should we force pack these?
struct FontHeader
{
   u32 count;
   u32 mapWidth;
   u32 mapHeight;
};

struct CharInfo
{
   float xoffset;
   float yoffset;
   float xadvance;
   u32 width;
   u32 height;
   u32 x;
   u32 y;
   i8 id;   
};

struct FontData
{
   u32 count;
   u32 mapWidth;
   u32 mapHeight;
   CharInfo *data;
};

struct ImageHeader
{
   i32 x;
   i32 y;
   i32 channels;
};

struct Image
{
   u8 *data;
   i32 x;
   i32 y;
   i32 channels;   
};

struct VirtualCoord
{
   i32 x;
   i32 y;
};

// Higher level Game Object abstraction
struct Object
{
   v3 worldPos;
   v3 scale;
   quat orientation;   
};

#pragma pack(push, 1)
struct Branch_Image_Header
{
   u32 size;
   u32 width, height;
   u32 channels;
   // pixels tagged onto the end
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PackedFont
{
   u32 width;
   u32 height;
   u32 imageOffset;
   // followed by the array of stbtt_packedchar
};
#pragma pack(pop)

struct stbFont
{
   stbtt_packedchar *chars;
   u8 *map;
   u32 width;
   u32 height;
   u32 textureHandle;
};

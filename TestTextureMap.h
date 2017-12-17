
struct texture_bbox
{
   u32 x0, y0, x1, y1;

   inline u32 width();
   inline u32 height();
};

inline u32
texture_bbox::width()
{
   return x1 - x0;
}

inline u32
texture_bbox::height()
{
   return y1 - y0;
}

struct TestTextureMap
{
   texture_bbox playButton;
   texture_bbox 
};

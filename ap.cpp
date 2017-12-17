/* ap.cpp */
/* Quick tool I made to process assets */

#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "libs/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"

#include "BranchTypes.h"
#include "BranchMath.h"
#include "BranchCommon.h"

#define PACKED_ASSET_DIRECTORY "../ProcessedAssets/"
#define TO_PACKED_ASSET_PATH(filename) (PACKED_ASSET_DIRECTORY ## filename)

// vertex bits
#define VERTEX 0x1
#define UV     0x2
#define NORMAL 0x4

FILE *OpenForRead(char *file)
{
   FILE *result = _fsopen(file, "rb", _SH_DENYWR);

   if(result)
   {
      return result;
   }
   else
   {
      printf("unable to open: %s\n", file);
      exit(0);
   }
}

FILE *OpenForWrite(char *file)
{
   FILE *result = _fsopen(file, "wb", _SH_DENYRW);

   if(result)
   {
      return result;
   }
   else
   {
      printf("unable to open: %s\n", file);
      exit(0);
   }
}

struct index
{
   u32 mask;

   union
   {
      i32 e[3];

      struct
      {
	 i32 x, y, z;
      };
   };
};

struct MeshBuilder
{
   v3 *vertices;
   u32 vcount;
   u32 vcapacity;
   u32 vbyteCapacity;

   v2 *uvs;
   u32 uvcount;
   u32 uvcapacity;
   u32 uvbyteCapacity;

   v3 *normals;
   u32 ncount;
   u32 ncapacity;
   u32 nbyteCapacity;

   index *indices; //(vertex, uv, normal)
   u32 icount;
   u32 icapacity;
   u32 ibyteCapacity;
};

inline
MeshBuilder Init()
{
   MeshBuilder result;

   result.vertices = (v3 *)malloc(1024);
   result.vcount = 0;
   result.vcapacity = 1024 / sizeof(v3);
   result.vbyteCapacity = 1024;

   result.uvs = (v2 *)malloc(1024);
   result.uvcount = 0;
   result.uvcapacity = 1024 / sizeof(v2);
   result.uvbyteCapacity = 1024;

   result.normals = (v3 *)malloc(1024);
   result.ncount = 0;
   result.ncapacity = 1024 / sizeof(v3);
   result.nbyteCapacity = 1024;

   result.indices = (index *)malloc(1024);
   result.icount = 0;
   result.icapacity = 1024 / sizeof(index);
   result.ibyteCapacity = 1024;

   return result;
};

void AddVertex(MeshBuilder *builder, v3 vertex)
{
   if(builder->vcount == builder->vcapacity)
   {
      v3 *temp = (v3 *)malloc(builder->vbyteCapacity << 1);
      builder->vbyteCapacity <<= 1;
      builder->vcapacity = builder->vbyteCapacity / sizeof(v3);
      
      memcpy(temp, builder->vertices, builder->vcount * sizeof(v3));
      free(builder->vertices);
      builder->vertices = temp;      
   }

   builder->vertices[builder->vcount++] = vertex;
}

void AddUv(MeshBuilder *builder, v2 uv)
{
   if(builder->uvcount == builder->uvcapacity)
   {
      v2 *temp = (v2 *)malloc(builder->uvbyteCapacity << 1);
      builder->uvbyteCapacity <<= 1;
      builder->uvcapacity = builder->uvbyteCapacity / sizeof(v2);
      
      memcpy(temp, builder->uvs, builder->uvcount * sizeof(v2));
      free(builder->uvs);
      builder->uvs = temp;      
   }

   builder->uvs[builder->uvcount++] = uv;
}

void AddNormal(MeshBuilder *builder, v3 normal)
{
   
   if(builder->ncount == builder->ncapacity)
   {
      v3 *temp = (v3 *)malloc(builder->nbyteCapacity << 1);
      builder->nbyteCapacity <<= 1;
      builder->ncapacity = builder->nbyteCapacity / sizeof(v3);
      
      memcpy(temp, builder->normals, builder->ncount * sizeof(v3));
      free(builder->normals);
      builder->normals = temp;      
   }

   builder->normals[builder->ncount++] = normal;
}

void AddIndice(MeshBuilder *builder, index indice)
{
   if(builder->icount == builder->icapacity)
   {
      index *temp = (index *)malloc(builder->ibyteCapacity << 1);
      builder->ibyteCapacity <<= 1;
      builder->icapacity = builder->ibyteCapacity / sizeof(index);

      memcpy(temp, builder->indices, builder->icount * sizeof(index));
      free(builder->indices);
      builder->indices = temp;
   }

   builder->indices[builder->icount++] = indice;
}

static inline
long FileSize(FILE *file)
{
   fseek(file, 0, SEEK_END);
   long fileSize = ftell(file);
   fseek(file, 0, SEEK_SET);

   return fileSize;
}

char *LoadIntoBuffer(char *fileName, size_t *outSize)
{
   FILE *file = OpenForRead(fileName);

   if(!file)
   {
      printf("Could not open file %s\n", fileName);
      return 0;
   }

   *outSize = FileSize(file);

   char *buffer = (char *)malloc(*outSize);
   fread(buffer, *outSize, 1, file);
   fclose(file);
   return buffer;
}

MeshBuilder ParseObj(char *obj, size_t size)
{
   char *at = obj;
   char *last = obj + size;

   MeshBuilder builder = Init();

   while(at != last)
   {
      switch(*at)
      {
	 case '\n':
	 {
	    while(*at != '\n' && at != last) ++at;
	 }break;

	 case '#':
	 {
	    while(*at != '\n' && at != last) ++at;
	 }break;

	 case ' ':
	 {
	    ++at;
	 }break;

	 default:
	 {
	    char *begin = at;
	    char *end;

	    while(*at != ' ' && *at != '\n' && at != last) ++at;
	    
	    end = at;
	    size_t size = (size_t)end - (size_t)begin;

	    if(size == 1)
	    {
	       if(*begin == 'v')
	       {
		  v3 vertex;
		  for(i32 i = 0; i < 3; ++i)
		  {
		     ++at;

		     begin = at;
		     while(*at != ' ' && *at != '\n' && at != last) ++at;
		     end = at;

		     vertex.e[i] = strtof(begin, &end);
		  }

		  AddVertex(&builder, vertex);
	       }
	       else if(*begin == 'f')
	       {		  		  

		  for(i32 i = 0; i < 3; ++i)
		  {
		     index indice;
		     indice.mask = 0;
		     
		     ++at;
		     begin = at;
		     while(*at != ' ' && *at != '\n') ++at;

		     for(i32 j = 0; j < 3; ++j)
		     {
			char *indexAt = begin;
			while(*indexAt != '/') ++indexAt;

			if(indexAt == begin)
			{
			   indice.mask |= 1 << j;
			}
			else
			{
			   indice.e[j] = strtol(begin, &indexAt, 10) - 1;
			}

			begin = indexAt + 1;
		     }

		     AddIndice(&builder, indice);
		  }		  
	       }
	       else if(*begin == 'o')
	       {
		  // object name
		  // Does not suppprt (but should we?)
		  while(*at != '\n') ++at;
	       }
	       else if(*begin == 'g')
	       {
		  // group name
		  // Does not support (but should we?)
		  while(*at != '\n') ++at;
	       }
	       else if(*begin == 's')
	       {
		  // smooth shading
		  // Does not support (and we probably don't need to)
		  while(*at != '\n') ++at;
	       }
	    }
	    else if(size == 2)
	    {
	       if(*begin == 'v')
	       {
		  if(begin[1] == 't')
		  {
		     v2 uv;

		     for(i32 i = 0; i < 2; ++i)
		     {
			++at;

			begin = at;
			while(*at != ' ' && *at != '\n' && at != last) ++at;
			end = at;

			uv.e[i] = strtof(begin, &end);
		     }

		     AddUv(&builder, uv);
		  }

		  else if(begin[1] == 'n')
		  {
		     v3 normal;

		     for(i32 i = 0; i < 3; ++i)
		     {
			++at;

			begin = at;
			while(*at != ' ' && *at != '\n' && at != last) ++at;
			end = at;

			normal.e[i] = strtof(begin, &end);
		     }

		     AddNormal(&builder, normal);
		  }

		  else if(begin[1] == 'p')
		  {
		     // Does not support
		  }		  
	       }
	    }
	 }break;
      }
      ++at;
   }

   return builder;
}

void Model(char *filename)
{
   FILE *file = OpenForRead(filename);

   long fileSize = FileSize(file);
   char *buffer = (char *)malloc(fileSize);
   fread(buffer, fileSize, 1, file);
   fclose(file);

   MeshBuilder b = ParseObj(buffer, fileSize);
   size_t writeSize = b.icount * sizeof(v3) + sizeof(u32);
   void *writebuffer = malloc(writeSize);

   *(i32 *)writebuffer = b.icount;

   // make sure to be aligned on an 8 byte boundary
   v3 *begin = (v3 *)(((u8 *)writebuffer) + 8);

   for(i32 i = 0; i < b.icount; ++i)
   {
      begin[i] = b.vertices[b.indices[i].e[0]];
   }

   file = OpenForWrite(TO_PACKED_ASSET_PATH("sphere.brian"));
   fwrite(writebuffer, writeSize, 1, file);
   fclose(file);

   free(b.vertices);
   free(b.normals);
   free(b.uvs);
   free(b.indices);
   free(buffer);
   free(writebuffer);
}

static inline
long DistanceToEndOfLine(char *at, char *last)
{
   long length = 0;
   while(at < last && *at != '\n'){ ++length; ++at; }
   return length;
}

char *ProcessShader(char *buffer, long fileSize, long *outSize)
{
   char *outBuffer = (char *)malloc(fileSize);
   memcpy(outBuffer, buffer, fileSize);
   long workingSize = fileSize;

   char *at = outBuffer;
   char *last = outBuffer + workingSize;
   
   while(at < last)
   {
      switch(*at)
      {
	 case '#':
	 {
	    char *preBegin = at;
	    while(at < last && *at != ' ' && *at != '\n' && *at != '\t') ++at;
	    char *preEnd = at;
	    
	    if(at + 8 < last && strncmp(preBegin, "#include", 8) == 0)
	    {
	       while(at < last && *at != '<') ++at;	       
	       ++at;
	       
	       char *begin = at;
	       while(at < last && *at != '>') ++at;
	       char *end = at;

	       size_t fileNameSize = (size_t)end - (size_t)begin;

	       if(fileNameSize == 0)
	       {
		  printf("incorrect file name");
		  free(outBuffer);
		  return 0;
	       }
	       
	       char *fileName = (char *)malloc(fileNameSize + 1);
	       fileName[fileNameSize] = '\0';
	       memcpy(fileName, begin, fileNameSize);

	       FILE *file = OpenForRead(fileName);
	       
	       if(!file)
	       {
		  printf("\"%s\": not found.", fileName);
		  free(fileName);
		  free(outBuffer);
		  return 0;
	       }

	       long fileSize = FileSize(file);

	       long beginPart = (size_t)preBegin - (size_t)outBuffer; //keep everything before the #include <filename>
	       long cutPart = DistanceToEndOfLine(preBegin, last); // cut the #include <filename> out
	       long endPart = workingSize - beginPart - cutPart; // keep everything after the #include <filename>

	       workingSize = beginPart + endPart + fileSize;	       
	       char *temp = (char *)malloc(workingSize);	       

	       // insert file contents into new buffer
	       memcpy(temp, outBuffer, beginPart);
	       fread(temp + beginPart, fileSize, 1, file);
	       memcpy(temp + beginPart + fileSize, outBuffer + beginPart + cutPart, endPart);

	       fclose(file);
	       free(fileName);
	       free(outBuffer);
	       outBuffer = temp;

	       at = outBuffer + beginPart + 1;
	       last = &outBuffer[workingSize - 1];
	    }	    
	 }break;
      }

      ++at;
   }

   *outSize = workingSize;
   return outBuffer;
}

void Shader(char *filename)
{
   FILE *file = OpenForRead(filename);

   if(!file)
   {
      printf("Remember to close win32.exe and debuggers before processing\n");
      return;
   }

   long fileSize = FileSize(file);

   char *buffer = (char *)malloc(fileSize);
   fread(buffer, fileSize, 1, file);
   fclose(file);

   long outSize;
   char *outBuffer = ProcessShader(buffer, fileSize, &outSize);

   if(outBuffer)
   {
      char path[64];
      sprintf(path, "%s/%sp", PACKED_ASSET_DIRECTORY, filename);
      file = OpenForWrite(path);
      fwrite(outBuffer, outSize, 1, file);
      fclose(file);
      free(outBuffer);
   }
   else
   {
      printf("fatal error\n");
   }

   free(buffer);
}

void *LoadImage(char *filename, size_t *size, int *width, int *height, int *channels)
{
   unsigned char *buffer = (unsigned char *)LoadIntoBuffer(filename, size);

   return stbi_load_from_memory(buffer, (int)*size, width, height, channels, 4);
}

int WriteIntoTexture(u32 *map, u32 map_width, u32 map_height, char *filename, u32 x, u32 y)
{
   size_t fileSize;

   size_t image_size;
   int image_width;
   int image_height;
   int image_channels;
   u32 *image = (u32 *)LoadImage(filename, &image_size, &image_width, &image_height, &image_channels);

   if(!image)
   {
      printf("Could not find image %s\n", filename);
      return 0;
   }

   if(image_channels != 4)
   {
      printf("Unsupported image format of %s\n", filename);
      printf("Require channels %d, given %d\n", 4, image_channels);
      return 0;
   }

   if(image_width > map_width || image_height > map_height)
   {
      printf("image to large (or may not be an image at all)\n");
      free(image);
      return 0;
   }

   u32 *map_row = map + (map_width * y) + x;
   u32 *image_row = image;
   u32 row = 0;

   while(row <= image_height)
   {
      // write rows of image into rows of the map
      memcpy(map_row, image_row, image_width * sizeof(u32));
      map_row += map_width;
      image_row += image_width;
      ++row;
   }

   free(image);
   return 1;
}

// Right now texture maps only support 4 channel textures
int Map(u32 map_width, u32 map_height, char **images, int num_images)
{
   stbrp_context context;

   // The api says to allow num_nodes to be >= width
   int num_nodes = map_width << 1;
   stbrp_node *node = (stbrp_node *)malloc(num_nodes * sizeof(stbrp_node));

   if(!node)
   {
      printf("Pack nodes could not be allocated\n");
      return 0;
   }

   stbrp_init_target(&context, map_width, map_height, node, num_nodes);

   stbrp_rect *rects = (stbrp_rect *)malloc(sizeof(stbrp_rect) * num_images);

   if(!rects)
   {
      printf("Could not allocate rectangles for pack\n");
      free(node);
      return 0;
   }

   for(int i = 0; i < num_images; ++i)
   {
      int width, height, channels;
      stbi_info(images[i], &width, &height, &channels);

      rects[i].id = i;
      rects[i].w = width;
      rects[i].h = height;
   }

   if(!stbrp_pack_rects(&context, rects, num_images))
   {
      free(rects);
      free(node);
      
      printf("Could not pack rectangles\n");
      return 0;
   }

   // no longer need nodes
   free(node);

   Branch_Image_Header *texture_map = (Branch_Image_Header *)malloc((sizeof(u32) * map_width * map_height) + sizeof(Branch_Image_Header));
   texture_map->size = map_width * map_height * 4; // 4 channels
   texture_map->width = map_width;
   texture_map->height = map_height;
   texture_map->channels = 4; // should we support more???
   
   u32 *texture = (u32 *)(texture_map + 1);
   memset(texture, 0, sizeof(u32) * map_width * map_height);

   for(int i = 0; i < num_images; ++i)
   {
      if(!WriteIntoTexture(texture, map_width, map_height, images[rects[i].id], rects[i].x, rects[i].y))
      {
	 free(texture_map);
	 free(rects);
	 printf("Unable to insert texture %s into map\n", images[rects[i].id]);
	 return 0;
      }
   }

   free(rects);

   FILE *out = OpenForWrite(TO_PACKED_ASSET_PATH("TemporaryMap"));
   fwrite(texture_map, texture_map->size + sizeof(Branch_Image_Header), 1, out);
   fclose(out);

   stbi_write_bmp("TemporaryMap.bmp", map_width, map_height, 4, (void *)texture);

   free(texture_map);

   return 1;
}

int Font(char *fontName, u32 width, u32 height, float pointSize)
{
   FILE *font = OpenForRead(fontName);
   size_t size = FileSize(font);
   u8 *fileBuffer = (u8 *)malloc(size);
   fread(fileBuffer, size, 1, font);

   stbtt_fontinfo info;
   stbtt_packedchar *chars;

   if(!stbtt_InitFont(&info, fileBuffer, 0))
   {
      free(fileBuffer);
      return false;
   }

   u32 charOffset = sizeof(PackedFont);
   u32 imageOffset = sizeof(PackedFont) + (sizeof(stbtt_packedchar) * 256);
   u32 outFileSize = sizeof(PackedFont) + (sizeof(stbtt_packedchar) * 256) + (width * height);
   u8 *outBuffer = (u8 *)malloc(outFileSize);
   PackedFont *result = (PackedFont *)outBuffer;
   
   stbtt_pack_context pack;
   stbtt_PackBegin(&pack, outBuffer + imageOffset, width, height, width, 1, 0);
   stbtt_PackSetOversampling(&pack, 4, 4);
   stbtt_PackFontRange(&pack, fileBuffer, 0, STBTT_POINT_SIZE(pointSize), 0, 256,
		       (stbtt_packedchar *)(outBuffer + charOffset));
   stbtt_PackEnd(&pack);
   
   result->width = width;
   result->height = height;
   result->imageOffset = imageOffset;

   printf("width:      %d\n", result->width);
   printf("height:     %d\n", result->height);
   printf("point_size: %f\n", pointSize);

   FILE *outFont = OpenForWrite(TO_PACKED_ASSET_PATH("wow.font"));
   fwrite(result, outFileSize, 1, outFont);

   free(outBuffer);
   free(fileBuffer);
   
   return true;
}

/*
int Font(char *imgFileName, char *dataFileName)
{
   FILE *dataFile = OpenForRead(dataFileName);
   if(dataFileName)
   {

      while(fgetc(dataFile) != '\n');

      int count;
      fscanf(dataFile, "chars count=%i\n", &count);

      size_t fontDataSize = count * sizeof(CharInfo) + sizeof(FontHeader);
      FontHeader *header = (FontHeader *)malloc(fontDataSize);

      CharInfo *buffer = (CharInfo *)((u8 *)header + sizeof(FontHeader));

      int page;
      int chnl;

      for(int i = 0; i < count; ++i)
      {
	 fscanf(dataFile, "char id=%hhi\tx=%i\ty=%i\twidth=%i\theight=%i\t xoffset=%f\tyoffset=%f\txadvance=%f\tpage=%i\tchnl=%i\n",
		&buffer[i].id, &buffer[i].x, &buffer[i].y, &buffer[i].width, &buffer[i].height, &buffer[i].xoffset, &buffer[i].yoffset,
		&buffer[i].xadvance, &page, &chnl);
      }

      ImageHeader resultImage;
      unsigned char *image = stbi_load(imgFileName, &resultImage.x, &resultImage.y, &resultImage.channels, 0);

      header->count = count;
      header->mapWidth = resultImage.x;
      header->mapHeight = resultImage.y;

      FILE *fontFile = OpenForWrite("font_data.bf");
      fwrite(header, fontDataSize, 1, fontFile);
      fclose(fontFile);
      free(header);      

      if(image == 0)
      {
	 fclose(dataFile);
	 free(header);
	 return 0;
      }

      size_t fileSize = (resultImage.x * resultImage.y * resultImage.channels) + sizeof(ImageHeader);
      void *outImage = malloc(fileSize);
      *((ImageHeader *)outImage) = resultImage;
      memcpy((u8 *)outImage + sizeof(ImageHeader), image, resultImage.x * resultImage.y * resultImage.channels);

      FILE *imageFile = OpenForWrite("distance_field.bi");
      fwrite(outImage, fileSize, 1, imageFile);
      fclose(imageFile);
      free(outImage);
      free(image);
   }
   else
   {
      return 0;
   }

   return 1;
}

int BFont(char *filename, int point, int width, int height)
{
   FILE *dataFile = OpenForRead(filename);

   if(!dataFile)
   {
      return 0;
   }

   size_t fileSize = FileSize(dataFile);
   void *buffer = (void *)malloc(fileSize);

   fread(buffer, fileSize, 1, dataFile);

   stbtt_fontinfo font;
   if(!stbtt_InitFont(&font, (const u8 *)buffer, 0))
   {
      free(buffer);
      return 0;
   }

   i32 glyphCount = font.numGlyphs;

   u8 *pixels = (u8 *)malloc(width * height);
   stbtt_packedchar *charData = (stbtt_packedchar *)malloc(sizeof(stbtt_packedchar) * glyphCount);
   float fontSize = stbtt_ScaleForPixelHeight(&font, point);

   stbtt_pack_context spc;
   if(!stbtt_PackBegin(&spc, pixels, width, height, width, 1, 0)) return 0;
   if(!stbtt_PackFontRange(&spc, (u8 *)buffer, 0, fontSize, 0, glyphCount, charData)) return 0;
   stbtt_PackEnd(&spc);

   size_t fontDataSize = glyphCount * sizeof(CharInfo) + sizeof(FontHeader);
   FontHeader *fontDataHeader = (FontHeader *)malloc(fontDataSize);
   CharInfo *chars = (CharInfo *)(fontDataHeader + 1);

   for(i32 i = 0; i < glyphCount; ++i)
   {
      chars[i] = {
	 charData[i].xoff,
	 charData[i].yoff,
	 charData[i].xadvance,
	 (u32)charData[i].x1 - (u32)charData[i].x0,
	 (u32)charData[i].y1 - (u32)charData[i].y0,
	 (u32)charData[i].x0,
	 (u32)charData[i].y0,
	 (i8)i
      };
   }

   size_t imageSize = sizeof(ImageHeader) + width * height;
   ImageHeader *resultImage = (ImageHeader *)malloc(imageSize); // 1 channel
   resultImage->x = width;
   resultImage->y = height;
   resultImage->channels = 1;
   
   memcpy(resultImage + 1, pixels, width * height);
   free(pixels);

   FILE *output = OpenForWrite("bitmap_font.bi");
   fwrite(resultImage, imageSize, 1, output);
   fclose(output);   

   output = OpenForWrite("bitmap_font_data.bf");
   fwrite(fontDataHeader, fontDataSize, 1, output);
   fclose(output);
   fclose(dataFile);
   free(buffer);

   return 1;
}
*/

void Image(char *fileName)
{
   size_t size;
   int width, height;
   int channels;
   unsigned char *pixels;
   Branch_Image_Header header;
   if(pixels = (u8 *)LoadImage(fileName, &size, &width, &height, &channels))
   {
      header.size = size;
      header.width = width;
      header.height = height;
      header.channels = 4;

      size_t pixelsSize = (width * height * header.channels);

      Branch_Image_Header *result = (Branch_Image_Header *)malloc(sizeof(Branch_Image_Header) + pixelsSize);
      *result = header;
      memcpy(result + 1, pixels, pixelsSize);

      int nameLength = strlen(fileName);
      for(int i = 0; i < nameLength; ++i)
      {
	 if(fileName[i] == '.')
	 {
	    fileName[i] = '\0';
	    break;
	 }
      }

      char outname[32];
      sprintf(outname, "%s/%s", PACKED_ASSET_DIRECTORY, fileName);

      FILE *outFile = OpenForWrite(outname);
      fwrite(result, sizeof(Branch_Image_Header) + pixelsSize, 1, outFile);
   }
   else
   {
      printf("stb image conversion failed\n");
      exit(0);
   }
}

void Build(int count, char **arguments)
{
   // we should only call build when we are in the assets folder
   FILE *header = OpenForWrite("../AssetHeader.h");
   FILE *packed = OpenForWrite(TO_PACKED_ASSET_PATH("Packed.assets"));

   char head[] = "//BUILT BY AP.EXE, DO NOT EDIT\nstruct AssetHeader\n{\n";
   fwrite(head, strlen(head), 1, header);

   u32 *offsets = (u32 *)malloc(count * sizeof(u32));
   u32 *sizes = (u32 *)malloc(count * sizeof(u32));

   size_t currentOffset = 0;
   char *path = (char *)malloc(64);
   char *entry = (char *)malloc(128);
   for(int i = 0; i < count; ++i)
   {
      sprintf(path, "%s/%s", PACKED_ASSET_DIRECTORY, arguments[i]);
      FILE *asset = OpenForRead(path);
      size_t size = FileSize(asset);
      sizes[i] = size;

      void *buffer = malloc(size);
      fread(buffer, size, 1, asset);
      fwrite(buffer, size, 1, packed);
      fclose(asset);
      free(buffer);

      char *c = arguments[i];
      while(*c != '.' && *c != '\0') ++c;
      *c = '\0';

      sprintf(entry, "   static const u32 %s_ID = %d;\n",
	      arguments[i], i+1, arguments[i], currentOffset);
      
      fwrite(entry, strlen(entry), 1, header);

      offsets[i] = currentOffset;
      currentOffset += size;
   }

   char arrayBegin[32];
   sprintf(arrayBegin, "   static const u32 offsetTable[];\n");
   fwrite(arrayBegin, strlen(arrayBegin), 1, header);

   sprintf(arrayBegin, "   static const u32 sizeTable[];\n");
   fwrite(arrayBegin, strlen(arrayBegin), 1, header);

   sprintf(entry, "   static const u32 entries = %d;\n", count);
   fwrite(entry, strlen(entry), 1, header);

   char end[] = "};\n";
   fwrite(end, strlen(end), 1, header);

   sprintf(arrayBegin, "const u32 AssetHeader::offsetTable[] = {");
   fwrite(arrayBegin, strlen(arrayBegin), 1, header);   

   for(u32 i = 0; i < count; ++i)
   {
      char num[16];
      sprintf(num, "%d,", offsets[i]);
      fwrite(num, strlen(num), 1, header);
   }   

   char arrayEnd[] = "};\n";      
   fwrite(arrayEnd, strlen(arrayEnd), 1, header);

   sprintf(arrayBegin, "const u32 AssetHeader::sizeTable[] = {");
   fwrite(arrayBegin, strlen(arrayBegin), 1, header);

   for(u32 i = 0; i < count; ++i)
   {
      char num[16];
      sprintf(num, "%d,", sizes[i]);
      fwrite(num, strlen(num), 1, header);
   }

   fwrite(arrayEnd, strlen(arrayEnd), 1, header);

   fclose(header);
   fclose(packed);
   free(path);
   free(entry);
   free(offsets);
   printf("Assets Packed\n");
}

void main(int argc, char **argv)
{
   if(argc > 1)
   {
      char *parse = argv[1];

      int build = strcmp(parse, "model");
      if(build == 0)
      {
	 if(argc == 3)
	 {
	    Model(argv[2]);
	 }
	 else
	 {
	    printf("Incorrect argument format\n");
	 }
      }
      else
      {
	 int shader = strcmp(parse, "shader");
	 if(shader == 0)
	 {
	    if(argc == 3)
	    {
	       Shader(argv[2]);
	    }
	    else
	    {
	       printf("Incorrect argument format\n");
	    }
	 }
	 else
	 {
	    int font = strcmp(parse, "font");

	    if(font == 0)
	    {
	       if(argc == 6)
	       {
		  if(!Font(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5])))
		  {
		     printf("unable to finish font processing");
		  }
	       }
	       else
	       {
		  printf("Incorrect argument format\n");
	       }
	    }
	    else {
	       int image = strcmp(parse, "image");

	       if(image == 0)
	       {
		  if(argc == 3)
		  {
		     Image(argv[2]);
		  }
		  else
		  {
		     printf("Incorrect argument format");
		  }
	       }
	       else
	       {
		  int build = strcmp(parse, "build");

		  if(build == 0)
		  {
		     if(argc > 2)
		     {
			Build(argc - 2, &argv[2]);
		     }
		     else
		     {
			printf("No Assets specified to build");
		     }
		  }
		  else
		  {
		     int map = strcmp(parse, "map");

		     if(map == 0)
		     {
			int num_images = argc - 4;
			if(num_images < 1)
			{
			   printf("Incorrect argument format\n");
			}
			else
			{
			   int width = atoi(argv[2]);
			   int height = atoi(argv[3]);

			   Map(width, height, &argv[4], num_images);
			}
		     }
		  }
	       }      
	    }
	 }
      }
   }
   else
   {
      printf("Brian's Asset Processor\n\n");
      printf("ap shader <filename>\n");
      printf("   PreProcesses a shader\n\n");
      printf("ap model <filename>\n");
      printf("   Converts an obj model to a binary format\n");
      printf("ap image <filename>\n");
      printf("   Converts png to my format\n");
   }
}

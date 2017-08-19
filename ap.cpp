#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stb_truetype.h"
#include "branch_common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// vertex mask
#define VERTEX 1
#define UV (1 << 1)
#define NORMAL (1 << 2)

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
   FILE *file = fopen("filename", "rb");

   if(!file)
   {
      printf("Remember to close win32.exe and stop visual studio before processing\n");
      return;
   }

   long fileSize = FileSize(file);
   char *buffer = (char *)malloc(fileSize);
   fread(buffer, fileSize, 1, file);
   fclose(file);

   MeshBuilder b = ParseObj(buffer, fileSize);
   size_t writeSize = b.icount * sizeof(v3) + sizeof(u32);
   void *writebuffer = malloc(writeSize);

   *(u32 *)writebuffer = b.icount;
   v3 *begin = (v3 *)(((u32 *)writebuffer) + 1);

   for(i32 i = 0; i < b.icount; ++i)
   {
      begin[i] = b.vertices[b.indices[i].e[0]];
   }

   file = fopen("assets/sphere.brian", "wb");
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

	       FILE *file = fopen(fileName, "rb");
	       
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
   FILE *file = fopen(filename, "rb");

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
      sprintf(path, "assets/%sp", filename);
      file = fopen(path, "wb");
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


int Font(char *imgFileName, char *dataFileName)
{
   FILE *dataFile = fopen(dataFileName, "rb");
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

      FILE *fontFile = fopen("font_data.bf", "wb");
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

      FILE *imageFile = fopen("distance_field.bi", "wb");
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
   FILE *dataFile = fopen(filename, "rb");

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

   FILE *output = fopen("bitmap_font.bi", "wb");
   fwrite(resultImage, imageSize, 1, output);
   fclose(output);   

   output = fopen("bitmap_font_data.bf", "wb");
   fwrite(fontDataHeader, fontDataSize, 1, output);
   fclose(output);
   fclose(dataFile);
   free(buffer);

   return 1;
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
	       if(argc == 4)
	       {
		  if(!Font(argv[2], argv[3]))
		  {
		     printf("unable to finish font processing");
		  }
	       }
	       else
	       {
		  printf("Incorrect argument format\n");
	       }
	    }
	    else
	    {
	       int bfont = strcmp(parse, "bfont");

	       if(bfont == 0)
	       {
		  if(argc == 6)
		  {
		     if(!BFont(argv[2], atoi(argv[3]), atoi(argv[4]), atoi(argv[5])))
		     {
			printf("unable to finish font processing");
		     }
		  }
		  else
		  {
		     printf("Incorrect argument format");
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
   }
}

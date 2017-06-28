#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "branch_common.h"

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

   fseek(file, 0, SEEK_END);
   long fileSize = ftell(file);
   fseek(file, 0, SEEK_SET);

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

   printf("Done\n");
}

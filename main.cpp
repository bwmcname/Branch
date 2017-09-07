
#define SCREEN_TOP ((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_BOTTOM -((float)SCREEN_HEIGHT / (float)SCREEN_WIDTH)
#define SCREEN_RIGHT 1.0f
#define SCREEN_LEFT -1.0f

static
m4 CameraMatrix(Camera &camera)
{
   //calculate the inverse by computing the conjugate
   //the normal of a quaternion is the same as the unit for a v4
   quat inverse;
   inverse.V4 = unit(camera.orientation.V4);
   inverse.x = -inverse.x;
   inverse.y = -inverse.y;
   inverse.z = -inverse.z;
   inverse.w = -inverse.w;

   m4 rotation = M4(inverse);
   m4 translation = Translate(-camera.position.x,
			      -camera.position.y,
			      -camera.position.z);

   return rotation * translation;
}

static
void InitCamera(Camera &camera)
{
   camera.position = V3(0.0f, 0.0f, 10.0f);
   camera.orientation = Rotation(V3(1.0f, 0.0f, 0.0f), 1.0f);
   camera.view = CameraMatrix(camera);
   camera.t = 0.0f;
   camera.lerping = false;
};

static inline
tri2 Tri2(v2 a, v2 b, v2 c)
{
   tri2 result;

   result.a = a;
   result.b = b;
   result.c = c;

   return result;
}

static inline
v3 V3(v2 a, float z)
{
   return {a.x, a.y, z}; 
}

static inline
Curve InvertX(Curve c)
{
   Curve result;
   result.p1 = V2(-c.p1.x, c.p1.y);
   result.p2 = V2(-c.p2.x, c.p2.y);
   result.p3 = V2(-c.p3.x, c.p3.y);
   result.p4 = V2(-c.p4.x, c.p4.y);
   return result;
}

static __forceinline
Curve lerp(Curve a, Curve b, float t)
{
   Curve result;
   result.lerpables[0] = lerp(a.lerpables[0], b.lerpables[0], t);
   result.lerpables[1] = lerp(a.lerpables[1], b.lerpables[1], t);
   return result;
}

static __forceinline
v2 CubicBezier(v2 p1, v2 p2, v2 p3, v2 p4, float t)
{
   // uses the quicker non matrix form of the equation
   v2 result;   

   float degree1 = 1.0f - t;   
   float degree2 = degree1 * degree1; // (1 - t)^2
   float degree3 = degree2 * degree1; // (1 - t)^3
   float squared = t * t;   
   float cubed = squared * t;   

   result.x = degree3 * p1.x + 3 * t * degree2 * p2.x + 3 * squared * degree1 * p3.x + cubed * p4.x;
   result.y = degree3 * p1.y + 3 * t * degree2 * p2.y + 3 * squared * degree1 * p3.y + cubed * p4.y;

   return result;
}

static __forceinline
v2 CubicBezier(Curve c, float t)
{
   return CubicBezier(c.p1, c.p2, c.p3, c.p4, t);
}

static inline
Track CreateTrack(v3 position, v3 scale, Curve *bezier)
{
   Object obj;
   obj.worldPos = position;
   obj.scale = scale;
   obj.orientation = Quat(0.0f, 0.0f, 0.0f, 0.0f);

   Track result;
   result.renderable = obj;
   result.bezier = bezier;
   result.flags = 0;

   return result;
}

template <typename T>
inline i32 CircularQueue<T>::IncrementIndex(i32 index)
{
   if(index == max) return 0;
   else return index + 1;
}

template <typename T>
void CircularQueue<T>::Push(T e)
{
   assert(size <= max);

   elements[end] = e;
   ++size;
   IncrementEnd();
}

template <typename T>
T CircularQueue<T>::Pop()
{
   assert(size > 0);

   T pop = elements[begin];
   --size;
   IncrementBegin();

   return pop;
}

template <typename T>
void CircularQueue<T>::IncrementBegin()
{
   if(begin == max - 1)
   {
      begin = 0;
   }
   else
   {
      ++begin;
   }
}

template <typename T>
void CircularQueue<T>::IncrementEnd()
{
   if(end == max - 1)
   {
      end = 0;
   }
   else
   {
      ++end;
   }
}

template <typename T>
void CircularQueue<T>::ClearToZero()
{
   size = 0;
   begin = 0;
   end = 0;
   memset(elements, 0, max * sizeof(T));
}

static inline
v2 VirtualToReal(i32 x, i32 y)
{
   return V2((float)x * TRACK_SEGMENT_SIZE, (float)y * TRACK_SEGMENT_SIZE);
}

// returns a linear bezier curve
static inline
Curve LinearCurve(i32 x1, i32 y1,
		  i32 x2, i32 y2)
{
   v2 begin = VirtualToReal(x1, y1);
   v2 end = VirtualToReal(x2, y2);

   v2 direction = end - begin;
   
   // right now, just a straight line
   Curve result;
   result.p1 = V2(0.0f, 0.0f);
   result.p2 = 0.333333f * direction;
   result.p3 = 0.666666f * direction;
   result.p4 = direction;

   return result;
}

static inline
Curve BreakCurve()
{

   float length = 0.7f * TRACK_SEGMENT_SIZE;

   Curve result;
   result.p1 = {0.0f, 0.0f};
   result.p2 = 0.333333f * V2(0.0f, length);
   result.p3 = 0.666666f * V2(0.0f, length);
   result.p4 = {1.0f, length};

   return result;
}

static inline
Curve BranchCurve(i32 x1, i32 y1,
		  i32 x2, i32 y2)
{
   v2 begin = VirtualToReal(x1, y1);
   v2 end = VirtualToReal(x2, y2);

   v2 direction = end - begin;

   Curve result;
   result.p1 = V2(0.0f, 0.0f);
   result.p2 = V2(0.0f, direction.y * 0.666667f);
   result.p3 = V2(direction.x, direction.y * 0.333333f);
   result.p4 = direction;

   return result;
}

inline void
Attribute::removeEdge(u16 edgeID)
{
   if(hasLeft())
   {
      if(leftEdge() == edgeID)
      {
	 flags &= ~hasLeftEdge;
	 return;
      }
   }

   if(hasRight())
   {
      if(leftEdge() == edgeID)
      {
	 flags &= ~hasRightEdge;
	 return;
      }
   }
}

inline void
Attribute::addAncestor(u16 ancestorID)
{
   assert(ancestorCount < 3);
   ancestors[ancestorCount++] = ancestorID;   
}

inline void
Attribute::removeAncestor(u16 ancestorID)
{
   assert(ancestorCount > 0);

   u16 i;
   for(i = 0; i < ancestorCount; ++i)
   {
      if(ancestors[i] == ancestorID) break;
   }

   if(i == (ancestorCount - 1))
   {
      --ancestorCount;
   }
   else
   {
      ancestors[i] = ancestors[ancestorCount-1];
      --ancestorCount;
   }
}

inline Attribute &
NewTrackGraph::GetTrack(u16 id)
{
   return adjList[IDtable[id]];
}

inline void
NewTrackGraph::SetID(u16 virt, u16 actual)
{
   IDtable[virt] = actual;
   reverseIDtable[actual] = virt;
}

inline u16
NewTrackGraph::GetActualID(u16 virt)
{
   return IDtable[virt];
}

inline u16
NewTrackGraph::GetVirtualID(u16 actual)
{
   return reverseIDtable[actual];
}

inline u16
NewTrackGraph::HasLinearAncestor(u16 actual)
{
   for(u16 i = 0; i < adjList[actual].ancestorCount; ++i)
   {
      u16 ancestorActual = GetActualID(adjList[actual].ancestors[i]);
      if(adjList[ancestorActual].flags & Attribute::linear) return true;
   }

   return false;
}

inline void
NewTrackGraph::Move(u16 src, u16 dst)
{
   elements[dst] = elements[src];
   adjList[dst] = adjList[src];
   u16 virt = GetVirtualID(src);
   SetID(virt, dst);
}

inline void
NewTrackGraph::Swap2(u16 a, u16 b)
{
   // Tracks
   {
      Track temp = elements[a];
      elements[a] = elements[b];
      elements[b] = temp;
   }

   // adjacency information
   {
      Attribute temp = adjList[a];
      adjList[a] = adjList[b];
      adjList[b] = temp;
   }

   // id's
   {
      u16 temp = IDtable[a];
      IDtable[a] = IDtable[b];
      IDtable[b] = temp;
   }
}

void
NewTrackGraph::RemoveTrackActual(u16 id)
{
   if(adjList[id].hasLeft())
   {
      u16 idEdge = IDtable[adjList[id].leftEdge()];

      adjList[idEdge].removeAncestor(id);
   }

   if(adjList[id].hasRight())
   {
      u16 idEdge = IDtable[adjList[id].rightEdge()];

      adjList[idEdge].removeAncestor(id);
   }

   for(u16 i = 0; i < adjList[id].ancestorCount; ++i)
   {
      u16 idAncestor = IDtable[adjList[id].ancestors[i]];

      adjList[idAncestor].removeEdge(id);
   }   
}

template <typename T>
CircularQueue<T> InitCircularQueue(u32 size, StackAllocator *allocator)
{
   CircularQueue<T> q;
   q.max = size;
   q.begin = 0;
   q.end = 0;
   q.size = 0;
   q.elements = (T *)allocator->push(sizeof(T) * size);

   return q;
}

void StartTracks(NewTrackGraph &g)
{
   g.IDtable[0] = 0;
   g.elements[0] = CreateTrack(V3(0.0f, 0.0f, 0.0f), V3(1.0f, 1.0f, 1.0f), &GlobalLinearCurve);
   g.elements[0].flags = Track::left;

   g.orders.Push({0, NewTrackOrder::left | NewTrackOrder::dontBranch | NewTrackOrder::dontBreak, 0, 1});
   g.IDtable[0] = 0;
   g.reverseIDtable[0] = 0;

   // make sure the tracks start out straight
   for(u16 i = 1; i < 50; ++i)
   {
      g.orders.Push({i, NewTrackOrder::left | NewTrackOrder::dontBranch | NewTrackOrder::dontBreak, 0, 1 + i});
      g.IDtable[i] = i;
      g.reverseIDtable[i] = i;
   }   

   g.flags = NewTrackGraph::left;

   g.switchDelta = 0.0f;
   g.beginLerp = {};
   g.endLerp = {};

   for(u16 i = 50; i < g.capacity; ++i)
   {
      g.availableIDs.Push(i);
      g.IDtable[i] = i;
      g.reverseIDtable[i] = i;
   }
}

NewTrackGraph InitNewTrackGraph(StackAllocator *allocator)
{
   NewTrackGraph g;
   g.adjList = (Attribute *)allocator->push(sizeof(Attribute) * 1024);

   for(u32 i = 0; i < g.capacity; ++i)
   {
      g.adjList[i] = {0, Attribute::unused, 0, 0, {}, {}};
   }

   g.availableIDs = InitCircularQueue<u16>(1024, allocator); //@ could be smaller?
   g.orders = InitCircularQueue<NewTrackOrder>(1024, allocator); //@ could be smaller
   g.newBranches = InitCircularQueue<u16>(256, allocator);
   g.taken = InitVirtualCoordHashTable(1024, allocator);
   g.IDtable = (u16 *)allocator->push(sizeof(u16) * 1024);
   g.reverseIDtable = (u16 *)allocator->push(sizeof(u16) * 1024);
   g.elements = (Track *)allocator->push(sizeof(Track) * 1024);

   StartTracks(g);

   return g;
}

void ResetGraph(NewTrackGraph &g)
{
   for(u32 i = 0; i < g.capacity; ++i)
   {
      g.adjList[i] = {0, Attribute::unused, 0, 0, {}, {}};
   }

   g.availableIDs.ClearToZero();
   g.newBranches.ClearToZero();
   g.orders.ClearToZero();
   g.taken.ClearToZero();

   g.switchDelta = 0.0f;
   g.flags = 0;

   StartTracks(g);
}

static
i32 OtherSideOfBranchHasBreak(NewTrackGraph &graph, u16 ancestor, u16 thisSide)
{
   u16 actualAncestor = graph.GetActualID(ancestor);

   if(thisSide & NewTrackOrder::left)
   {
      if(graph.adjList[actualAncestor].hasRight())
      {
	 u16 right = graph.GetActualID(graph.adjList[actualAncestor].rightEdge());
	 return (graph.adjList[right].flags & Attribute::breaks);
      }
      return false;
   }
   else
   {
      if(graph.adjList[actualAncestor].hasLeft())
      {
	 u16 left = graph.GetActualID(graph.adjList[actualAncestor].leftEdge());
	 return (graph.adjList[left].flags & Attribute::breaks);
      }
      return false;
   }
}

void FillGraph(NewTrackGraph &graph)
{
   while(graph.availableIDs.size > 0 && graph.orders.size > 0)
   {
      NewTrackOrder item = graph.orders.Pop();

      LocationInfo info = graph.taken.get({item.x, item.y});

      u16 edgeID;
      u16 flags = 0;

      if(info.hasTrack())
      {
	 edgeID = info.ID;	 
      }
      else
      {	
	 edgeID = graph.availableIDs.Pop();
	 assert(graph.adjList[graph.IDtable[edgeID]].flags & Attribute::unused);
	 
	 v2 position = VirtualToReal(item.x, item.y);
	 long roll = rand();	 

	 if(roll % 2 == 0 &&
	    !(item.flags & NewTrackOrder::dontBreak) &&
	    !graph.taken.get({item.x, item.y-1}).hasTrack() &&
	    !OtherSideOfBranchHasBreak(graph, item.ancestorID, item.flags) &&
	    graph.adjList[graph.GetActualID(edgeID)].ancestorCount == 0)
	 {
	    u16 breakActual = graph.GetActualID(edgeID);
	    graph.elements[breakActual] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f),
						      &GlobalBreakCurve);
	    flags |= Attribute::breaks;
	    graph.taken.put({item.x, item.y}, LocationInfo::track, edgeID);
	 }
	 else if(roll % 5 == 0 && !(item.flags & NewTrackOrder::dontBranch) &&
		 !graph.taken.get({item.x+1, item.y+1}).hasBranch() &&
		 !graph.taken.get({item.x-1, item.y+1}).hasBranch() &&
		 !graph.taken.get({item.x+1, item.y-1}).hasBranch() &&
		 !graph.taken.get({item.x-1, item.y-1}).hasBranch())
	 {
	    u16 branchActual = graph.GetActualID(edgeID);
	    graph.elements[branchActual] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f),
								&GlobalBranchCurve);

	    graph.elements[branchActual].flags |= Track::branch;
	    flags |= Attribute::branch;
	    
	    graph.orders.Push({edgeID, NewTrackOrder::left | NewTrackOrder::dontBranch, item.x - 1, item.y + 1});
	    graph.orders.Push({edgeID, NewTrackOrder::right | NewTrackOrder::dontBranch, item.x + 1, item.y + 1});

	    graph.taken.put({item.x, item.y}, LocationInfo::track | LocationInfo::branch, edgeID);
	    graph.newBranches.Push(edgeID);
	 }
	 // if(item.flags & NewTrackOrder::speedup)
	 // {
	 //    graph.elements[graph.GetActualID(edgeID)] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f),
	 // 							    &GlobalLinearCurve);

	 //    graph.orders.Push({edgeID, NewTrackOrder::left | NewTrackOrder::dontBreak, item.x, item.y+1});
	 //    flags |= Attribute::speedup;
	 //    graph.taken.put({item.x, item.y}, LocationInfo::track, edgeID);
	 // }
 	 else // push linear track (and speedups)
	 {
	    graph.elements[graph.GetActualID(edgeID)] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f),
								    &GlobalLinearCurve);

	    graph.orders.Push({edgeID, NewTrackOrder::left | NewTrackOrder::dontBreak, item.x, item.y+1});
	    flags |= Attribute::linear;
	    graph.taken.put({item.x, item.y}, LocationInfo::track, edgeID);
	 }	 
      }

      u16 ancestorID = item.ancestorID;
      u16 actualAncestor = graph.GetActualID(ancestorID);
      u16 actualEdge = graph.GetActualID(edgeID);

      assert(actualEdge != actualAncestor);

      if(item.flags & NewTrackOrder::left)
      {
	 graph.adjList[actualAncestor].flags |= Attribute::hasLeftEdge;
      }
      else
      {
	 graph.adjList[actualAncestor].flags |= Attribute::hasRightEdge;
      }

      graph.adjList[actualAncestor].edges[item.Side()] = edgeID;
      ++graph.adjList[actualAncestor].edgeCount;
      
      graph.adjList[actualEdge].addAncestor(ancestorID);
      graph.adjList[actualEdge].flags |= flags;
      graph.adjList[actualEdge].flags &= ~Attribute::unused;
      DEBUG_DO(graph.VerifyGraph());
      DEBUG_DO(graph.VerifyIDTables());
   }

   // check all new branches, if any of them have linear tracks on both sides, make one side speedups.
   // only do 1/2, so that tracks can form around the latest pushed branches
   i32 length = graph.newBranches.size >> 1;
   for(i32 i = 0; i < length; ++i)
   {
      u16 virt = graph.newBranches.Pop();
      u16 actual = graph.GetActualID(virt);

      if(graph.adjList[actual].hasLeft() && graph.adjList[actual].hasRight())
      {
	 u16 leftActual = graph.GetActualID(graph.adjList[actual].leftEdge());
	 u16 rightActual = graph.GetActualID(graph.adjList[actual].rightEdge());

	 if(graph.adjList[leftActual].flags & Attribute::linear &&
	    graph.adjList[rightActual].flags & Attribute::linear)
	 {
	    i32 side = rand() & 1;
	    u16 speedupActual;

	    if(side == 0)
	    {
	       speedupActual = leftActual;
	    }
	    else
	    {
	       speedupActual = rightActual;
	    }	    
	    
	    do
	    {
	       if(graph.adjList[speedupActual].ancestorCount > 1) break;

	       // change the track from a linear track to a speeduptrack
	       graph.adjList[speedupActual].flags &= ~Attribute::linear;
	       graph.adjList[speedupActual].flags |= Attribute::speedup;
	       side = 0; // make side to left since all linear tracks only have a left child
	       speedupActual = graph.GetActualID(graph.adjList[speedupActual].edges[0]);
	    } while(graph.adjList[speedupActual].flags & Attribute::linear);
	 }	 
      }
      else
      {
	 // children of branch haven't been generated yet, take care of later
	 graph.newBranches.Push(virt);
      }      
   }
}

void NewSetReachable(NewTrackGraph &graph, StackAllocator &allocator, u16 start)
{
   u16 *stack = (u16 *)allocator.push(1024);
   u32 top = 1;

   stack[0] = graph.GetActualID(start);

   while(top > 0)
   {
      u16 index = stack[--top];

      if(!(graph.adjList[index].flags & Attribute::reachable))
      {
	 graph.adjList[index].flags |= Attribute::reachable;

	 if(graph.adjList[index].hasLeft())
	 {
	    stack[top++] = graph.GetActualID(graph.adjList[index].leftEdge());
	 }

	 if(graph.adjList[index].hasRight())
	 {
	    stack[top++] = graph.GetActualID(graph.adjList[index].rightEdge());
	 }
      }
   }

   allocator.pop();
}

#define NotReachableVisible(flags) (((flags) & Attribute::invisible) && !((flags) & Attribute::reachable))
#define NotReachable(flags) (!((flags) & Attribute::reachable))
#define NotVisible(flags) ((flags) & Attribute::invisible)
				

void NewSortTracks(NewTrackGraph &graph, StackAllocator &allocator, v3 cameraPos)
{
   u16 *removed = (u16 *)allocator.push(1024 * sizeof(u16)); //@ can these be smaller?
   u16 *unreachable = (u16 *)allocator.push(1024 * sizeof(u16));
   u16 removedTop = 0;
   u16 unreachableTop = 0;
   
   for(u16 i = 0; i < (u16)graph.capacity; ++i)
   {      
      u8 notReachable = NotReachable(graph.adjList[i].flags);
      u8 notVisible = NotVisible(graph.adjList[i].flags);
      if(notReachable)
      {
	 u16 virt = graph.GetVirtualID(i);
	 if(notVisible)
	 { 
	    removed[removedTop++] = virt;
	    graph.RemoveTrackActual(i);
	    if(graph.availableIDs.size != graph.availableIDs.max)
	    {
	       graph.availableIDs.Push(virt);
	    }
	    graph.adjList[i] = {};
	    graph.elements[i] = {};
	    graph.adjList[i].flags = Attribute::unused;
	 }
	 else
	 {
	    unreachable[unreachableTop++] = virt;
	 }
      }
   }

   if(unreachableTop > 0)
   {
      u32 size = graph.orders.size;

      for(u32 i = 0; i < size; ++i)
      {
	 i32 good = 1;
	 NewTrackOrder item = graph.orders.Pop();
	 for(u16 j = 0; j < unreachableTop; ++j)
	 {
	    if(unreachable[j] == item.ancestorID)
	    {
	       good = 0;
	       break;
	    }
	 }

	 if(good) graph.orders.Push(item);
      }
   }

   // now remove all orders with removed ancestors
   i32 good = 1;
   if(removedTop > 0)
   {
      u32 size = graph.orders.size;
      for(u32 i = 0; i < size; ++i)
      {
	 NewTrackOrder item = graph.orders.Pop();
	 for(u16 j = 0; j < removedTop; ++j)
	 {
	    if(removed[j] == item.ancestorID)
	    {
	       good = 0;
	       break;
	    }
	 }

	 if(good) graph.orders.Push(item);
      }

      // now remove all removed items in hashtable
      for(u16 i = 0; i < graph.capacity; ++i)
      {
	 if(graph.taken.e[i].flags & VirtualCoordHashTable::Element::occupied)
	 {
	    for(u16 j = 0; j < removedTop; ++j)
	    {
	       if(graph.taken.e[i].v.ID == removed[j])
	       {
		  graph.taken.e[i].flags = 0;
		  graph.taken.e[i].v = {0};
		  graph.taken.e[i].k = {0};	       
	       }
	    }
	 }
      }
   }

   for(u16 i = 1; i < graph.capacity; ++i)
   {
      Track track = graph.elements[i];
      Attribute attribute = graph.adjList[i];
      u16 virt = graph.GetVirtualID(i);

      u16 j = i - 1;
      float distance;
      if(attribute.flags & Attribute::unused)
      {
	 distance = INFINITY;
      }
      else
      {
	 distance = track.renderable.worldPos.y - cameraPos.y;
      }

      while(j >= 0 && (graph.elements[j].renderable.worldPos.y - cameraPos.y) > distance)
      {
	 graph.Move(j, j+1);
	 --j;
      }

      graph.elements[j+1] = track;
      graph.adjList[j+1] = attribute;      
      graph.SetID(virt, j+1);
   }

   allocator.pop();
   allocator.pop();
}

void NewUpdateTrackGraph(NewTrackGraph &graph, StackAllocator &allocator, Player &player, Camera &camera)
{
   for(u16 i = 0; i < graph.capacity; ++i)
   {
      graph.adjList[i].flags &= ~Attribute::reachable;
      graph.adjList[i].flags &= ~Attribute::invisible;
   }

   NewSetReachable(graph, allocator, player.trackIndex);

   float cutoff = player.renderable.worldPos.y - (TRACK_SEGMENT_SIZE * 2.0f);

   for(u16 i = 0; i < graph.capacity; ++i)
   {
      if(!(graph.adjList[i].flags & Attribute::unused))
      {
	 if(graph.elements[i].renderable.worldPos.y < cutoff)
	 {	    
	    graph.adjList[i].flags |= Attribute::invisible;
	 }
      }
   }

   NewSortTracks(graph, allocator, camera.position);

   FillGraph(graph);   
}

// the globalName "LoadImage" is already taken
static 
Image LoadImageFile(char *filename, StackAllocator *allocator)
{
   size_t fileSize = FileSize(filename);
   // @leak
   u8 *buffer = allocator->push(fileSize);

   FileRead(filename, buffer, fileSize);

   ImageHeader *header = (ImageHeader *)buffer;
   Image result;

   result.x = header->x;
   result.y = header->y;
   result.channels = header->channels;
   result.data = buffer + sizeof(header);

   return result;
}

static
FontData LoadFontFile(char *filename, StackAllocator *allocator)
{
   size_t fileSize = FileSize(filename);

   u8 *buffer = allocator->push(fileSize);
   FileRead(filename, buffer, fileSize);

   FontHeader *header = (FontHeader *)buffer;
   FontData result;

   result.count = header->count;
   result.mapWidth = header->mapWidth;
   result.mapHeight = header->mapHeight;
   result.data = (CharInfo *)(buffer + sizeof(FontHeader));

   return result;
}

static inline
Object SpherePrimitive(v3 position, v3 scale, quat orientation)
{
   Object result;
   result.worldPos = position;
   result.scale = scale;
   result.orientation = orientation;
   return result;
}

static __forceinline
v3 GetPositionOnTrack(Track &track, float t)
{   
   v2 displacement = CubicBezier(*track.bezier, t);
   return V3(displacement.x + track.renderable.worldPos.x,
	     displacement.y + track.renderable.worldPos.y,
	     track.renderable.worldPos.z);
}

inline
void ResetPlayer(Player &player)
{
   player.trackIndex = 0;
   player.t = 0.0f;
   player.velocity = 0.15f;
   player.forceDirection = 0;
}

void UpdatePlayer(Player &player, NewTrackGraph &tracks, GameState &state)
{
   // current track physical ID
   u16 actualID = tracks.GetActualID(player.trackIndex);
   Track *currentTrack = &tracks.elements[actualID];

   static bool paused = false;
   
   if(state.input.UnEscaped())
   {
      paused = !paused;
   }

   if(!paused)
   {
      player.t += player.velocity * delta;
   
      // don't drag if on speedup
      if(!(tracks.adjList[actualID].flags & Attribute::speedup))
      {
	 player.velocity = max(player.velocity - (0.00005f * delta), 0.15f); // drag
      }
   }
   // if the player has just left the current track
   if(player.t > 1.0f)
   {
      player.t -= 1.0f;      

      if((tracks.flags & NewTrackGraph::left) || (currentTrack->flags & Track::branch) == 0 ||
	 player.forceDirection & Player::Force_Left)
      {
	 u16 trackActual = tracks.GetActualID(player.trackIndex);
	 if(tracks.adjList[trackActual].hasLeft())
	 {
	    u16 newIndex = tracks.adjList[trackActual].leftEdge();
	    actualID = tracks.GetActualID(newIndex);
	    currentTrack = &tracks.elements[actualID];
	    player.trackIndex = newIndex;

	    if(tracks.adjList[actualID].flags & Attribute::speedup)
	    {
	       player.velocity = min(player.velocity + 0.01f, 0.25f);
	    }
	    else if(tracks.adjList[actualID].flags & Attribute::branch)
	    {
	       state.camera.BeginTrackSwitchEase();
	    }
	 }
	 else
	 {
	    //@TEMPORARY
	    state.state = GameState::RESET;
	 }
      }
      else
      {
	 u16 trackActual = tracks.GetActualID(player.trackIndex);
	 if(tracks.adjList[trackActual].hasRight())
	 {
	    u16 newIndex = tracks.adjList[tracks.GetActualID(player.trackIndex)].rightEdge();
	    actualID = tracks.GetActualID(newIndex);
	    currentTrack = &tracks.elements[actualID];
	    player.trackIndex = newIndex;	    

	    if(tracks.adjList[actualID].flags & Attribute::speedup)
	    {
	       player.velocity = min(player.velocity + 0.01f, 0.25f);
	    }
	    else if(tracks.adjList[actualID].flags & Attribute::branch)
	    {
	       state.camera.BeginTrackSwitchEase();
	    }
	 }
	 else
	 {
	    //@TEMPORARY
	    state.state = GameState::RESET;
	 }
      }

      player.forceDirection = 0;
   }   

   player.renderable.worldPos = GetPositionOnTrack(*currentTrack, player.t);   
}

inline void
Camera::BeginTrackSwitchEase()
{
   lerping = true;
   t = 0.0f;
}

void UpdateCamera(Camera &camera, Player &player, NewTrackGraph &graph)
{
   v3 playerPosition = player.renderable.worldPos;

   if(camera.lerping)
   {
      camera.t = min(camera.t + (delta * 0.02f), 1.0f);
      camera.position.x = lerp(camera.position.x, player.renderable.worldPos.x, camera.t);

      if(camera.t >= 1.0f)
      {
	 camera.lerping = false;
      }
   }
   else
   {      
      camera.position.x = playerPosition.x;
   }

   camera.position.y = playerPosition.y - 10.0f;
   camera.view = CameraMatrix(camera);
}

static __forceinline
v2 Tangent(Curve c, float t)
{
   v2 result;

   float minust = 1.0f - t;

   result.x = 3 * (minust * minust) * (c.p2.x - c.p1.x) + 6.0f * minust * t * (c.p3.x - c.p2.x) + 3.0f * t * t * (c.p4.x - c.p3.x);
   result.y = 3 * (minust * minust) * (c.p2.y - c.p1.y) + 6.0f * minust * t * (c.p3.y - c.p2.y) + 3.0f * t * t * (c.p4.y - c.p3.y);

   return unit(result);
}

static
void GenerateTrackSegmentVertices(MeshObject &meshBuffers, Curve bezier)
{
   // 10 segments, 8 tris per segment --- 80 tries   
   tri *tris = (tri *)meshBuffers.mesh.vertices;

   float t = 0.0f;
   v2 sample = CubicBezier(bezier, t);

   v2 direction = Tangent(bezier, t);
   v2 perpindicular = V2(-direction.y, direction.x) * 0.5f;         

   v3 top1 = V3(sample.x, sample.y, 0.5f);
   v3 right1 = V3(sample.x + perpindicular.x, sample.y, 0.0f);
   v3 bottom1 = V3(sample.x, sample.y, -0.5f);
   v3 left1 = V3(sample.x - perpindicular.x, sample.y, 0.0f);   
   
   for(i32 i = 0; i < 10; ++i)
   {
      t += 0.1f;
      sample = CubicBezier(bezier, t);

      direction = Tangent(bezier, t);
      
      perpindicular = V2(-direction.y, direction.x) * 0.5f;      

      v3 top2 = V3(sample.x, sample.y, 0.5f);
      v3 right2 = V3(sample.x + perpindicular.x, sample.y + perpindicular.y, 0.0f);
      v3 bottom2 = V3(sample.x, sample.y, -0.5f);
      v3 left2 = V3(sample.x - perpindicular.x, sample.y - perpindicular.y, 0.0f);
      
      i32 j = i * 8;

      // top left side
      tris[j] = Tri(top1, left1, left2);
      tris[j+1] = Tri(top1, left2, top2);

      // top right side
      tris[j+2] = Tri(top1, top2, right1);
      tris[j+3] = Tri(top2, right2, right1);

      // bottom right side
      tris[j+4] = Tri(bottom1, right1, bottom2);
      tris[j+5] = Tri(bottom2, right1, right2);

      // bottom left side
      tris[j+6] = Tri(bottom1, left2, left1);
      tris[j+7] = Tri(bottom1, bottom2, left2);

      top1 = top2;
      right1 = right2;
      left1 = left2;
      bottom1 = bottom2;
   }

   v3 *normals = meshBuffers.mesh.normals;
   Normals((float *)tris, (v3 *)normals, 80 * 3); // 3 vertices per tri
   
   glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.handles.vbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, (sizeof(tri) * 80), (void *)tris);
   glBindBuffer(GL_ARRAY_BUFFER, meshBuffers.handles.nbo);
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tri) * 80, (void *)normals);
   glBindBuffer(GL_ARRAY_BUFFER, 0);   
}

static inline
void GenerateTrackSegmentVertices(Track &track, MeshObject meshBuffers)
{
   GenerateTrackSegmentVertices(meshBuffers, *track.bezier);   
}

GLuint UploadDistanceTexture(Image &image)
{
   GLuint result;
   glGenTextures(1, &result);
   glBindTexture(GL_TEXTURE_2D, result);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.x, image.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

GLuint UploadTexture(Image &image, i32 channels = 4)
{
   GLuint result;
   glGenTextures(1, &result);
   glBindTexture(GL_TEXTURE_2D, result);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   GLuint type;
   switch(channels)
   {
      case 1:
      {
	 type = GL_RED;
      }break;
      case 4:
      {
	 type = GL_RGBA;
      }break;
      default:
      {
	 assert(!"unsupported channel format");
	 type = 0; // shut the compiler up
      }
   }

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.x, image.y, 0, type, GL_UNSIGNED_BYTE, image.data);
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

static
void InitTextBuffers()
{
   glGenVertexArrays(1, &textVao);
   glBindVertexArray(textVao);
   glGenBuffers(1, &textUVVbo);
   glBindBuffer(GL_ARRAY_BUFFER, textUVVbo);
   glEnableVertexAttribArray(UV_LOCATION);
   glVertexAttribPointer(UV_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * 12), 0, GL_STATIC_DRAW);

   glGenBuffers(1, &textVbo);
   glBindBuffer(GL_ARRAY_BUFFER, textVbo);
   glEnableVertexAttribArray(VERTEX_LOCATION);
   glVertexAttribPointer(VERTEX_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * 12), 0, GL_STATIC_DRAW);
   glBindVertexArray(0);
}

static
stbFont InitFont_stb(char *fontFile, u32 width, u32 height, StackAllocator *allocator)
{
   stbFont result;

   result.width = width;
   result.height = height;

   size_t fileSize = FileSize(fontFile);

   result.rawFile = allocator->push(fileSize);
   FileRead(fontFile, result.rawFile, fileSize);
   
   if(!stbtt_InitFont(&result.info, result.rawFile, 0))
   {
      assert(false);
   }

   stbtt_pack_context pack;
   result.chars = (stbtt_packedchar *)allocator->push(sizeof(stbtt_packedchar) * 256);
   result.map = allocator->push(width * height);

   // @could we do this in the asset processor? and then use the stb_api to pull the quad from the generated texture?
   stbtt_PackBegin(&pack, result.map, width, height, width, 1, 0); // @should supply our own allocator instead of defaulting to malloc
   stbtt_PackSetOversampling(&pack, 4, 4);

   stbtt_PackFontRange(&pack, result.rawFile, 0, STBTT_POINT_SIZE(16.0f), 0, 256, result.chars);
   stbtt_PackEnd(&pack);

   glGenTextures(1, &result.textureHandle);
   glBindTexture(GL_TEXTURE_2D, result.textureHandle);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, result.map);
   glBindTexture(GL_TEXTURE_2D, 0);

   return result;
}

inline
Player InitPlayer()
{
   Player result;
   result.renderable = SpherePrimitive(V3(0.0f, -2.0f, 5.0f),
				       V3(1.0f, 1.0f, 1.0f),
				       Quat(1.0f, 0.0f, 0.0f, 0.0f));

   result.mesh = Sphere;   
   result.velocity = 0.15f;
   result.t = 0.0f;   
   result.trackIndex = 0;

   result.forceDirection = 0;

   return result;
}

static inline
Branch_Image_Header *LoadImageFromAsset(Asset &asset)
{
   return (Branch_Image_Header *)asset.mem;
}

Branch_Image_Header *LoadImageFromFileName(char *name, StackAllocator *allocator)
{
   size_t fileSize = FileSize(name);
   Branch_Image_Header *buffer = (Branch_Image_Header *)allocator->push(fileSize);
   WinReadFile(name, (u8 *)buffer, fileSize);
   return buffer;
}

GLuint LoadImageIntoTexture(Branch_Image_Header *header)
{
   GLuint result;
   glGenTextures(1, &result);
   glBindTexture(GL_TEXTURE_2D, result);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, header->width, header->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void *)(header + 1));
   glBindTexture(GL_TEXTURE_2D, 0);
   return result;
}

void GameInit(GameState &state)
{
   state.mainArena.base = AllocateSystemMemory(GIGABYTES(2), &state.mainArena.size);
   state.mainArena.current = state.mainArena.base;
   InitStackAllocator((StackAllocator *)state.mainArena.base);
   StackAllocator *stack = (StackAllocator *)state.mainArena.base;

   state.assetManager.Init(stack);

      // @leak
   LinearTrack = AllocateMeshObject(80 * 3, stack);
   BranchTrack = AllocateMeshObject(80 * 3, stack);
   BreakTrack = AllocateMeshObject(80 * 3, stack);

   state.renderer = InitRenderState(stack, state.assetManager);   

   state.state = GameState::START;

   state.tracks = InitNewTrackGraph(stack);

   assert(state.mainArena.base);

   srand((u32)__rdtsc());

   glEnable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   InitCamera(state.camera);   

   Asset &defaultVert = state.assetManager.LoadStacked(AssetHeader::default_vert_ID);
   Asset &defaultFrag = state.assetManager.LoadStacked(AssetHeader::default_frag_ID);
   DefaultShader = CreateProgramFromAssets(defaultVert, defaultFrag);

   state.assetManager.PopStacked(AssetHeader::default_frag_ID);
   state.assetManager.PopStacked(AssetHeader::default_vert_ID);

   Asset &defaultInstancedVirt = state.assetManager.LoadStacked(AssetHeader::Default_Instance_vert_ID);
   Asset &defaultInstancedFrag = state.assetManager.LoadStacked(AssetHeader::Default_Instance_frag_ID);
   DefaultInstanced = CreateProgramFromAssets(defaultInstancedVirt, defaultInstancedFrag);

   state.assetManager.PopStacked(AssetHeader::Default_Instance_frag_ID);
   state.assetManager.PopStacked(AssetHeader::Default_Instance_vert_ID);

   state.fontProgram = CreateTextProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::text_vert_ID),
						   state.assetManager.LoadStacked(AssetHeader::text_frag_ID));

   state.bitmapFontProgram = CreateTextProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::bitmap_font_vert_ID),
							 state.assetManager.LoadStacked(AssetHeader::bitmap_font_frag_ID));
   
   BreakBlockProgram = CreateProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::BreakerBlock_vert_ID),
					       state.assetManager.LoadStacked(AssetHeader::BreakerBlock_frag_ID));

   ButtonProgram = CreateProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::Button_vert_ID),
					   state.assetManager.LoadStacked(AssetHeader::Button_frag_ID));

   Branch_Image_Header *buttonHeader = LoadImageFromAsset(state.assetManager.LoadStacked(AssetHeader::button_ID));
   state.buttonTex = LoadImageIntoTexture(buttonHeader);

   state.backgroundProgram = CreateSimpleProgramFromAssets(state.assetManager.LoadStacked(AssetHeader::Background_vert_ID),
							   state.assetManager.LoadStacked(AssetHeader::Background_frag_ID));

   stack->pop();
   stack->pop();

   state.tempFontField = LoadImageFile("distance_field.bi", stack);   
   state.tempFontData = LoadFontFile("font_data.bf", stack);
   state.bitmapFont = InitFont_stb("c:/Windows/Fonts/arial.ttf", 1024, 1024,stack);
   
   state.fontTextureHandle = UploadTexture(state.tempFontField);
   state.keyState = up;
   Sphere = InitMeshObject("assets\\sphere.brian", stack);

   GlobalLinearCurve = LinearCurve(0, 0, 0, 1);
   GlobalBranchCurve = BranchCurve(0, 0,
				   -1, 1);
   GlobalBreakCurve = BreakCurve();

   GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve);
   GenerateTrackSegmentVertices(LinearTrack, GlobalLinearCurve);
   GenerateTrackSegmentVertices(BreakTrack, GlobalBreakCurve);

   RectangleUVBuffer = UploadVertices(RectangleUVs, 6, 2);
   RectangleVertBuffer = UploadVertices(RectangleVerts, 6, 2);
   ScreenVertBuffer = UploadVertices(ScreenVerts, 6, 2);

   InitTextBuffers();
   
   state.lightPos = state.camera.position;

   state.sphereGuy = InitPlayer();

   FillGraph(state.tracks);
}

template <typename int_type> static __forceinline
int_type IntToString(char *dest, int_type num)
{
   static char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

   if(num == 0)
   {
      dest[0] = '0';
      dest[1] = '\0';
      return 1;
   }
   
   int_type count = 0;
   for(char *c = dest; num; ++c)
   {
      int_type digit = num % 10;
      dest[count] = table[digit];
      ++count;
      num /= 10;
   }

   for(int_type i = 0; i < (count / 2); ++i)
   {
      SWAP(char, dest[i], dest[count - i - 1]);
   }

   dest[count] = '\0';

   return count;
}

inline
v2 ScreenToClip(v2i screen)
{
   float x = (float)screen.x;
   float y = (float)screen.y;

   v2 result;
   result.x = (2.0f * x / (float)SCREEN_WIDTH) - 1.0f;
   result.y = -((2.0f * y / (float)SCREEN_HEIGHT) - 1.0f);

   return result;
}

void ProcessInput(GameState &state)
{
   if(state.input.Touched())
   {      

      if(state.state == GameState::LOOP)
      {
	 if(state.sphereGuy.OnSwitch(state.tracks) && state.sphereGuy.t > 0.5f)
	 {
	    if(state.tracks.flags & NewTrackGraph::left)
	    {
	       state.sphereGuy.forceDirection = Player::Force_Left;
	    }
	    else
	    {
	       state.sphereGuy.forceDirection = Player::Force_Right;
	    }
	 }

	 {
	    // toggle direction
	    state.tracks.flags ^= NewTrackGraph::left;

	    // if already lerping
	    if(state.tracks.flags & NewTrackGraph::switching)
	    {
	       state.tracks.beginLerp = state.tracks.endLerp;
	       state.tracks.endLerp = InvertX(state.tracks.beginLerp);
	       state.tracks.switchDelta = 1.0f - state.tracks.switchDelta;	 
	    }
	    else
	    {
	       state.tracks.flags |= NewTrackGraph::switching;

	       state.tracks.beginLerp = GlobalBranchCurve;
	       state.tracks.endLerp = InvertX(GlobalBranchCurve);
	       state.tracks.switchDelta = 0.0f;
	    }
	 }
      }  
   }
}

void GameLoop(GameState &state)
{
   BeginFrame(state);

   ProcessInput(state);

   switch(state.state)
   { 
      case GameState::LOOP:
      {	 
	 if(state.tracks.flags & NewTrackGraph::switching)
	 {
	    state.tracks.switchDelta = min(state.tracks.switchDelta + 0.1f * delta, 1.0f);

	    GlobalBranchCurve = lerp(state.tracks.beginLerp, state.tracks.endLerp, state.tracks.switchDelta);
	    GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve);

	    if(state.tracks.switchDelta == 1.0f)
	    {
	       state.tracks.flags &= ~NewTrackGraph::switching;
	    }
	 }	 

	 NewUpdateTrackGraph(state.tracks, *((StackAllocator *)state.mainArena.base), state.sphereGuy, state.camera);
	 UpdatePlayer(state.sphereGuy, state.tracks, state);
	 UpdateCamera(state.camera, state.sphereGuy, state.tracks);	 
	 state.lightPos = state.sphereGuy.renderable.worldPos;

	 glUseProgram(DefaultShader.programHandle); 
	 RenderObject(state.sphereGuy.renderable, state.sphereGuy.mesh, &DefaultShader, state.camera.view, state.lightPos, V3(1.0f, 0.0f, 0.0f));
	 glUseProgram(0);

	 RenderTracks(state, (StackAllocator *)state.mainArena.base);
	 
#ifdef TIMERS
	 static char framerate[8];
	 static float time = 120.0f;
	 time += delta;
	 static i32 count = 0;
	 if(time > 30.0f)
	 {
	    time = 0.0f;
	    count = IntToString(framerate, (i32)((1.0f / delta) * 60.0f));
	 }
	 // RenderText_stb(framerate, count, -0.8f, 0.8f, state.bitmapFont, state.bitmapFontProgram);
	 state.renderer.commands.PushRenderText(framerate, count, V2(-0.8f, 0.8f), V2(0.0f, 0.0f), V3(1.0f, 0.0f, 0.0f), ((StackAllocator *)state.mainArena.base));	 
	 static char renderTimeSting[19];
	 size_t renderTimeCount = IntToString(renderTimeSting, state.TrackRenderTime);
	 // RenderText_stb(renderTimeSting, (u32)renderTimeCount, -0.8f, 0.75f, state.bitmapFont, state.bitmapFontProgram);
	 state.renderer.commands.PushRenderText(renderTimeSting, (u32)renderTimeCount, V2(-0.8f, 0.75f), V2(0.0f, 0.0f), V3(1.0f, 0.0f, 0.0f), ((StackAllocator *)state.mainArena.base));

#ifdef DEBUG
#pragma warning(push, 0)
	 v2 clip = ScreenToClip(state.input.TouchPoint());
	 static char PointString[32];
	 sprintf_s(PointString, 32, "Mouse: (%f, %f)", clip.x, clip.y);
	 state.renderer.commands.PushRenderText(PointString, (u32)strlen(PointString), V2(-0.8f, 0.7f), V2(0.0f, 0.0f), V3(1.0f, 0.0f, 0.0f), ((StackAllocator *)state.mainArena.base));
#pragma warning(pop)
#endif
#endif	 
      }break;

      case GameState::RESET:
      {	 
	 state.state = GameState::START;

	 ResetPlayer(state.sphereGuy);
	 state.camera.position.y = state.sphereGuy.renderable.worldPos.y - 10.0f;
	 state.camera.position.x = 0.0f;

	 ResetGraph(state.tracks);
	 FillGraph(state.tracks);
      }break;

      case GameState::START:
      {
	 static float position = 0.0f;
	 position += delta * 0.01f;	 

	 if(ButtonUpdate(V2(0.0f, 0.0f), V2(0.2f, 0.1f), state.input) == Clicked)
	 {
	    state.state = GameState::LOOP;	    
	    state.sphereGuy.trackIndex = 0;
	    state.sphereGuy.t = 0.0f;

	    GlobalBranchCurve = BranchCurve(0, 0,
					    -1, 1);

	    GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve);
	 }

	 RenderTracks(state, (StackAllocator *)state.mainArena.base);
	 
	 state.renderer.commands.PushDrawButton(V2(0.0f, 0.0f), V2(0.2f, 0.1f), state.buttonTex, ((StackAllocator *)state.mainArena.base));
      }break;      
      
      case GameState::INITGAME:
      {
      }break;
      
      default:
      {
	 assert(!"invalid state");
      }
   }
   
   state.renderer.commands.ExecuteCommands(state.camera, state.lightPos, state.bitmapFont, state.bitmapFontProgram, state.renderer, ((StackAllocator *)state.mainArena.base));   
   state.renderer.commands.Clean(((StackAllocator *)state.mainArena.base));
}
   
void GameEnd(GameState &state)
{
   FreeSystemMemory(state.mainArena.base);

   glDeleteBuffers(1, &textUVVbo);
   glDeleteBuffers(1, &textVbo);
   glDeleteBuffers(1, &RectangleUVBuffer);
   glDeleteBuffers(1, &RectangleVertBuffer);
   glDeleteBuffers(1, &ScreenVertBuffer);
   glDeleteBuffers(1, &Sphere.handles.vbo);
   glDeleteBuffers(1, &Sphere.handles.nbo);
   glDeleteBuffers(1, &LinearTrack.handles.vbo);
   glDeleteBuffers(1, &BranchTrack.handles.nbo);
   glDeleteBuffers(1, &LinearTrack.handles.nbo);
   glDeleteBuffers(1, &BranchTrack.handles.vbo);

   glDeleteTextures(1, &state.fontTextureHandle);
}

#ifdef DEBUG

void
NewTrackGraph::VerifyIDTables()
{
   for(u16 i = 0; i < 1024; ++i)
   {
      assert(IDtable[reverseIDtable[i]] == i);
   }
}

void
NewTrackGraph::VerifyGraph()
{
   for(u32 i = 0; i < capacity; ++i)
   {
      if(!(adjList[i].flags & Attribute::unused))
      {
	 if(adjList[i].flags & Attribute::branch)
	 {
	    if(adjList[i].flags & Attribute::reachable)
	    {
	       if(!adjList[i].hasLeft())
	       {	      
		  i32 good = 0;
		  for(i32 j = orders.begin; j != orders.end; j = orders.IncrementIndex(j))
		  {
		     if(IDtable[orders.elements[j].ancestorID] == i &&
			orders.elements[j].flags & NewTrackOrder::left)
		     {
			good = 1;
			break;
		     }
		  }

		  assert(good);
	       }
	    
	       if(!adjList[i].hasRight())
	       {	       
		  i32 good = 0;
		  for(i32 j = orders.begin; j != orders.end; j = orders.IncrementIndex(j))
		  {
		     if(IDtable[orders.elements[j].ancestorID] == i &&
			(orders.elements[j].flags & NewTrackOrder::right))
		     {
			good = 1;
			break;
		     }
		  }

		  assert(good);
	       }
	    }
	 }
	 else if(adjList[i].flags & Attribute::breaks)
	 {
	    assert(!(adjList[i].flags & (Attribute::hasLeftEdge | Attribute::hasRightEdge)));

	    // There really isn't a good way to test if there is a branch behind a break,
	    // since tracks are removed once they become invisible
	    /*
	    i32 good = 0;
	    for(u32 j = 0; j < adjList[i].ancestorCount; ++j)
	    {
	       u16 ancestor = IDtable[adjList[i].ancestors[j]];
	       if(adjList[ancestor].flags & Attribute::branch)
	       {
		  good = 1;

		  u16 leftEdge = IDtable[adjList[ancestor].leftEdge()];
		  u16 rightEdge = IDtable[adjList[ancestor].rightEdge()];

		  if(adjList[ancestor].hasLeft())
		  {
		     if(leftEdge == i)
		     {
			if(adjList[ancestor].hasRight())
			{
			   assert(!(adjList[rightEdge].flags & Attribute::breaks));
			}
		     }	    		     
		  }
		  else if(adjList[ancestor].hasRight())
		  {
		     if(rightEdge == i)
		     {
			if(adjList[ancestor].hasLeft())
			{
			   assert(!(adjList[leftEdge].flags & Attribute::breaks));
			}
		     }
		  }
		  break;
	       }
	    }

	    assert(good);
	    */
	 }
	 else if(adjList[i].flags & Attribute::linear ||
		 adjList[i].flags & Attribute::speedup)
	 {
	    if(!adjList[i].hasLeft() && (adjList[i].flags & Attribute::reachable))
	    {
	       i32 good = 0;
	       for(i32 j = orders.begin; j != orders.end; j = orders.IncrementIndex(j))
	       {		  
		  if(IDtable[orders.elements[j].ancestorID] == i)
		  {
		     good = 1;
		     break;
		  }
	       }

	       assert(good);
	    }
	 }
	 else
	 {
	    assert(0);
	 }
      }
   }
}
#endif
#pragma warning(push, 0) // to shut up the compiler about sprintf_s

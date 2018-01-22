/* main.cpp */
/* Most gameplay code is implemented here
 * Track generation
 * Track sorting
 * Track mesh generation
 * State loading
 * Camera update
 * Player update
 *
 * This source file implements two important functions
 * GameInit
 * GameLoop
 * 
 * Both of these functions must be called by the platform layer.
 */

// Save the state of the application
// needed for android.

RebuildState *SaveState(GameState *state)
{
   RebuildState *saving = (RebuildState *)malloc(sizeof(RebuildState));

   saving->camera = state->camera;
   saving->player = state->sphereGuy;
   memcpy(saving->trackAttributes, state->tracks.adjList, sizeof(Attribute) * TRACK_COUNT);
   memcpy(saving->tracks, state->tracks.elements, sizeof(Track) * TRACK_COUNT);

   saving->availableIDsBegin = state->tracks.availableIDs.begin;
   saving->availableIDsEnd = state->tracks.availableIDs.end;
   saving->availableIDsSize = state->tracks.availableIDs.size;
   memcpy(saving->availableIDs, state->tracks.availableIDs.elements, sizeof(u16) * TRACK_COUNT);

   saving->ordersBegin = state->tracks.orders.begin;
   saving->ordersEnd = state->tracks.orders.end;
   saving->ordersSize = state->tracks.orders.size;
   memcpy(saving->orders, state->tracks.orders.elements, sizeof(NewTrackOrder) * TRACK_COUNT);

   memcpy(saving->takenElements, state->tracks.taken.e, sizeof(Element) * TRACK_COUNT);
   saving->takenSize = state->tracks.taken.size;

   memcpy(saving->IDtable, state->tracks.IDtable, sizeof(u16) * TRACK_COUNT);
   memcpy(saving->reverseIDtable, state->tracks.reverseIDtable, sizeof(u16) * TRACK_COUNT);

   saving->switchDelta = state->tracks.switchDelta;
   saving->beginLerp = state->tracks.beginLerp;
   saving->endLerp = state->tracks.endLerp;

   saving->trackGraphFlags = state->tracks.flags;
   
   return saving;
}

// Restore state of application
void ReloadState(RebuildState *saved, GameState &result)
{
   result.camera = saved->camera;
   result.sphereGuy = saved->player;
   memcpy(result.tracks.adjList, saved->trackAttributes, sizeof(Attribute) * TRACK_COUNT);
   memcpy(result.tracks.elements, saved->tracks, sizeof(Track) * TRACK_COUNT);

   result.tracks.availableIDs.begin = saved->availableIDsBegin;
   result.tracks.availableIDs.end = saved->availableIDsEnd;
   result.tracks.availableIDs.size = saved->availableIDsSize;
   result.tracks.availableIDs.max = TRACK_COUNT;
   memcpy(result.tracks.availableIDs.elements, saved->availableIDs, sizeof(u16) * TRACK_COUNT);

   result.tracks.orders.begin = saved->ordersBegin;
   result.tracks.orders.end = saved->ordersEnd;
   result.tracks.orders.size = saved->ordersSize;
   result.tracks.orders.max = TRACK_COUNT;
   memcpy(result.tracks.orders.elements, saved->orders, sizeof(NewTrackOrder) * TRACK_COUNT);

   // result.tracks.newBranches.begin = saved->newBranchesBegin;
   // result.tracks.newBranches.end = saved->newBranchesEnd;
   // result.tracks.newBranches.size = saved->newBranchesSize;
   // result.tracks.newBranches.max = 256;
   // memcpy(result.tracks.newBranches.elements, saved->newBranches, sizeof(u16) * 256);

   memcpy(result.tracks.taken.e, saved->takenElements, sizeof(Element) * TRACK_COUNT);
   result.tracks.taken.size = saved->takenSize;

   memcpy(result.tracks.IDtable, saved->IDtable, sizeof(u16) * TRACK_COUNT);
   memcpy(result.tracks.reverseIDtable, saved->reverseIDtable, sizeof(u16) * TRACK_COUNT);

   result.tracks.switchDelta = saved->switchDelta;
   result.tracks.beginLerp = saved->beginLerp;
   result.tracks.endLerp = saved->endLerp;

   result.renderer.commands.count = 0;
}

void
Camera::UpdateView()
{
   //calculate the inverse by computing the conjugate
   //the normal of a quaternion is the same as the unit for a v4
   quat inverse;
   inverse.V4 = unit(orientation.V4);
   inverse.x = -inverse.x;
   inverse.y = -inverse.y;
   inverse.z = -inverse.z;
   inverse.w = -inverse.w;

   m4 rotation = M4(inverse);
   m4 translation = Translate(-position.x,
			      -position.y,
			      -position.z);

   view = rotation * translation;

   forward = V3(view.e2[2][0], view.e2[2][1], -view.e2[2][2]);
}

static
void InitCamera(Camera &camera)
{
   camera.distanceFromPlayer = -14.0f;
   camera.position = V3(0.0f, camera.distanceFromPlayer, 12.0f);
   camera.orientation = Rotation(V3(1.0f, 0.0f, 0.0f), 1.1f);
   camera.UpdateView();
};

static B_INLINE
tri2 Tri2(v2 a, v2 b, v2 c)
{
   tri2 result;

   result.a = a;
   result.b = b;
   result.c = c;

   return result;
}

static B_INLINE
v3 V3(v2 a, float z)
{
   return {a.x, a.y, z}; 
}

static B_INLINE
Curve InvertX(Curve c)
{
   Curve result;
   result.p1 = V2(-c.p1.x, c.p1.y);
   result.p2 = V2(-c.p2.x, c.p2.y);
   result.p3 = V2(-c.p3.x, c.p3.y);
   result.p4 = V2(-c.p4.x, c.p4.y);
   return result;
}

static B_INLINE
Curve lerp(Curve a, Curve b, float t)
{
   Curve result;
   result.lerpables[0] = lerp(a.lerpables[0], b.lerpables[0], t);
   result.lerpables[1] = lerp(a.lerpables[1], b.lerpables[1], t);
   return result;
}

static B_INLINE
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

static B_INLINE
v2 CubicBezier(Curve c, float t)
{
   return CubicBezier(c.p1, c.p2, c.p3, c.p4, t);
}

static B_INLINE
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
B_INLINE i32 CircularQueue<T>::IncrementIndex(i32 index)
{
   if(index == max) return 0;
   else return index + 1;
}

template <typename T>
void CircularQueue<T>::Push(T e)
{
   B_ASSERT(size <= max);

   elements[end] = e;
   ++size;
   IncrementEnd();
}

template <typename T>
T CircularQueue<T>::Pop()
{
   B_ASSERT(size > 0);

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

static B_INLINE
v2 VirtualToReal(i32 x, i32 y)
{
   return V2((float)x * TRACK_SEGMENT_SIZE, (float)y * TRACK_SEGMENT_SIZE);
}

// returns a linear bezier curve
static B_INLINE
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

static B_INLINE
Curve BreakCurve()
{

   float length = 0.5f * TRACK_SEGMENT_SIZE;

   Curve result;
   result.p1 = {0.0f, 0.0f};
   result.p2 = {0.0f, 0.333333f * length};
   result.p3 = {0.0f, 0.666666f * length};
   result.p4 = {0.0f, length};

   return result;
}

static B_INLINE
Curve BranchCurve(i32 x1, i32 y1,
		  i32 x2, i32 y2)
{
   v2 begin = VirtualToReal(x1, y1);
   v2 end = VirtualToReal(x2, y2);

   v2 direction = end - begin;

   Curve result;
   result.p1 = V2(0.0f, 0.0f);
   // result.p2 = V2(0.0f, direction.y * 0.666667f);
   // result.p3 = V2(direction.x, direction.y * 0.333333f);
   result.p2 = V2(0.0f, direction.y * 0.44f);
   result.p3 = V2(direction.x, direction.y * 0.59f); 
   result.p4 = direction;

   return result;
}

B_INLINE void
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

B_INLINE void
Attribute::addEdge(u16 edgeID)
{
   edges[edgeCount++] = edgeID;
}

B_INLINE void
Attribute::addAncestor(u16 ancestorID)
{
   B_ASSERT(ancestorCount < 3);
   ancestors[ancestorCount++] = ancestorID;   
}

B_INLINE void
Attribute::removeAncestor(u16 ancestorID)
{
   B_ASSERT(ancestorCount > 0);

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

B_INLINE Attribute &
NewTrackGraph::GetTrack(u16 id)
{
   return adjList[IDtable[id]];
}

B_INLINE void
NewTrackGraph::SetID(u16 virt, u16 actual)
{
   IDtable[virt] = actual;
   reverseIDtable[actual] = virt;
}

B_INLINE u16
NewTrackGraph::GetActualID(u16 virt)
{
   return IDtable[virt];
}

B_INLINE u16
NewTrackGraph::GetVirtualID(u16 actual)
{
   return reverseIDtable[actual];
}

B_INLINE u16
NewTrackGraph::HasLinearAncestor(u16 actual)
{
   for(u16 i = 0; i < adjList[actual].ancestorCount; ++i)
   {
      u16 ancestorActual = GetActualID(adjList[actual].ancestors[i]);
      if(adjList[ancestorActual].flags & Attribute::linear) return true;
   }

   return false;
}

B_INLINE void
NewTrackGraph::Move(u16 src, u16 dst)
{
   elements[dst] = elements[src];
   adjList[dst] = adjList[src];
   u16 virt = GetVirtualID(src);
   SetID(virt, dst);
}

B_INLINE void
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
   // fill in initial tracks
   i32 count = 10;
   for(u16 i = 0; i < (u16)count; ++i)
   {
      g.IDtable[i] = i;
      g.reverseIDtable[i] = i;
   }

   g.taken.put({0, 0}, LocationInfo::track, 0);
   g.elements[0] = CreateTrack(V3(0.0f, 0.0f, 0.0f),
			       V3(1.0f, 1.0f, 1.0f), &GlobalLinearCurve);
   g.adjList[0].flags = Attribute::linear | Attribute::hasLeftEdge;
   g.adjList[0].edgeCount = 0;
   g.adjList[0].ancestorCount = 0;
   g.adjList[0].addEdge(1);

   for(i32 i = 1; i < count - 1; ++i)
   {
      g.taken.put({0, i}, LocationInfo::track, (u16)i);
      g.elements[i] = CreateTrack(V3(0.0f, i * TRACK_SEGMENT_SIZE, 0.0f),
				  V3(1.0f, 1.0f, 1.0f), &GlobalLinearCurve);
      g.adjList[i].flags = Attribute::linear | Attribute::hasLeftEdge;
      g.adjList[i].edgeCount = 0;
      g.adjList[i].ancestorCount = 0;
      g.adjList[i].addEdge(i+1);
      g.adjList[i].addAncestor(i-1);
   }

   g.taken.put({0, count-1}, LocationInfo::track, (count-1));
   g.elements[count-1] = CreateTrack(V3(0.0f, (count-1) * TRACK_SEGMENT_SIZE, 0.0f),
			       V3(1.0f, 1.0f, 1.0f), &GlobalLinearCurve);
   g.adjList[count-1].flags = Attribute::linear;
   g.adjList[count-1].edgeCount = 0;
   g.adjList[count-1].ancestorCount = 0;
   g.adjList[count-1].addAncestor(count-2);
   
   g.flags |= NewTrackGraph::left;
   
   for(u16 i = (u16)count; i < TRACK_COUNT; ++i)
   {
      g.availableIDs.Push(i);
      g.IDtable[i] = i;
      g.reverseIDtable[i] = i;
   }

   g.orders.Push({9, NewTrackOrder::left | NewTrackOrder::dontBranch | NewTrackOrder::dontBreak, 0, count});

   g.switchDelta = 0.0f;
   g.beginLerp = {};
   g.endLerp = {};
}

void AllocateTrackGraphBuffers(NewTrackGraph &g, StackAllocator *allocator)
{
   g.adjList = (Attribute *)allocator->push(sizeof(Attribute) * TRACK_COUNT);
   g.availableIDs = InitCircularQueue<u16>(TRACK_COUNT, allocator); //@ could be smaller?
   g.orders = InitCircularQueue<NewTrackOrder>(TRACK_COUNT, allocator); //@ could be smaller
   // g.newBranches = InitCircularQueue<u16>(256, allocator);
   g.taken = InitVirtualCoordHashTable(TRACK_COUNT, allocator);
   g.IDtable = (u16 *)allocator->push(sizeof(u16) * TRACK_COUNT);
   g.reverseIDtable = (u16 *)allocator->push(sizeof(u16) * TRACK_COUNT);
   g.elements = (Track *)allocator->push(sizeof(Track) * TRACK_COUNT);
}

NewTrackGraph InitNewTrackGraph(StackAllocator *allocator)
{
   NewTrackGraph g;

   AllocateTrackGraphBuffers(g, allocator);

   for(u32 i = 0; i < TRACK_COUNT; ++i)
   {
      g.adjList[i] = {0, Attribute::unused, 0, 0, {}, {}};
   }

   g.flags = 0;

   StartTracks(g);

   return g;
}

void ResetGraph(NewTrackGraph &g)
{
   for(u32 i = 0; i < TRACK_COUNT; ++i)
   {
      g.adjList[i] = {0, Attribute::unused, 0, 0, {}, {}};
   }

   g.availableIDs.ClearToZero();
   // g.newBranches.ClearToZero();
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

// Generate Tracks
void FillGraph(NewTrackGraph &graph)
{
   // In the track orders, find if there are multiple orders for the same coordinate.
   // This helps us get rid of the problem where a linear track can lead up
   // to a break.
   i32 sizeOuter = graph.orders.size;
   for(i32 i = 0; i < sizeOuter; ++i)
   {
      NewTrackOrder order = graph.orders.Pop();
      i32 sizeInner = graph.orders.size;
      for(i32 j = 0; j < sizeInner; ++j)
      {
	 NewTrackOrder toCompare = graph.orders.Pop();

	 if(toCompare.x == order.x && toCompare.y == order.y)
	 {
	    // make sure orders share the same flags
	    order.flags |= (toCompare.flags & (NewTrackOrder::dontBranch | NewTrackOrder::dontBreak));
	    toCompare.flags |= (order.flags & (NewTrackOrder::dontBranch | NewTrackOrder::dontBreak));

	    // We don't want to remove orders from the same location,
	    // so the FillGraph() can still assign ancestors and edges
	    // appropriately.
	 }

	 graph.orders.Push(toCompare);
      }

      graph.orders.Push(order);
   }

   while(graph.availableIDs.size > 0 &&
	 graph.orders.size > 0)
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
	 B_ASSERT(graph.adjList[graph.IDtable[edgeID]].flags & Attribute::unused);
	 
	 v2 position = VirtualToReal(item.x, item.y);
	 long roll = rand();	 

	 if(!(roll & 0x1) &&
	    !(item.flags & NewTrackOrder::dontBreak) &&
	    !graph.taken.get({item.x, item.y-1}).hasTrack() &&
	    !OtherSideOfBranchHasBreak(graph, item.ancestorID, item.flags) &&
	    graph.adjList[graph.GetActualID(edgeID)].ancestorCount == 0)
	 {
	    // Create break tracks
	    u16 breakActual = graph.GetActualID(edgeID);
	    graph.elements[breakActual] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f),
						      &GlobalBreakCurve);
	    flags |= Attribute::breaks;
	    graph.taken.put({item.x, item.y}, LocationInfo::track | LocationInfo::breaks, edgeID);
	 }
	 else if(roll % 5 == 0 && !(item.flags & NewTrackOrder::dontBranch) &&
		 !graph.taken.get({item.x+1, item.y+1}).hasBranch() &&
		 !graph.taken.get({item.x-1, item.y+1}).hasBranch() &&
		 !graph.taken.get({item.x+1, item.y-1}).hasBranch() &&
		 !graph.taken.get({item.x-1, item.y-1}).hasBranch() &&
		 !graph.taken.get({item.x-1, item.y-2}).hasBranch() &&
		 !graph.taken.get({item.x+1, item.y-2}).hasBranch() &&
		 !graph.taken.get({item.x+1, item.y+2}).hasBranch() &&
		 !graph.taken.get({item.x-1, item.y+2}).hasBranch())
	 {
	    // Create branching track
	    u16 branchActual = graph.GetActualID(edgeID);
	    graph.elements[branchActual] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f),
								&GlobalBranchCurve);

	    graph.elements[branchActual].flags |= Track::branch;
	    flags |= Attribute::branch;
	    
	    graph.orders.Push({edgeID, NewTrackOrder::left | NewTrackOrder::dontBranch, item.x - 1, item.y + 1});
	    graph.orders.Push({edgeID, NewTrackOrder::right | NewTrackOrder::dontBranch, item.x + 1, item.y + 1});

	    graph.taken.put({item.x, item.y}, LocationInfo::track | LocationInfo::branch, edgeID);
	    // graph.newBranches.Push(edgeID);
	 }
 	 else
	 {
	    // Create Linear Tracks
	    graph.elements[graph.GetActualID(edgeID)] = CreateTrack(V3(position.x, position.y, 0.0f), V3(1.0f, 1.0f, 1.0f),
								    &GlobalLinearCurve);
	    	    
	    graph.orders.Push({edgeID, NewTrackOrder::left | NewTrackOrder::dontBreak, item.x, item.y+1});
	    LocationInfo &behind = graph.taken.get({item.x, item.y});
	    if(behind.hasSpeedup())
	    {
	       graph.taken.put({item.x, item.y}, LocationInfo::speedup, edgeID);
	       flags |= Attribute::speedup;	       
	    }
	    else
	    {
	       graph.taken.put({item.x, item.y}, LocationInfo::track, edgeID);
	       flags |= Attribute::linear;
	    }
	 }	 
      }

      u16 ancestorID = item.ancestorID;
      u16 actualAncestor = graph.GetActualID(ancestorID);
      u16 actualEdge = graph.GetActualID(edgeID);

      B_ASSERT(actualEdge != actualAncestor);

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
      graph.adjList[actualEdge].id = edgeID;
      DEBUG_DO(graph.VerifyGraph());
      DEBUG_DO(graph.VerifyIDTables());
   }
}

void NewSetReachable(NewTrackGraph &graph, StackAllocator &allocator, u16 start)
{
   u16 *stack = (u16 *)allocator.push(TRACK_COUNT);
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
   u16 *removed = (u16 *)allocator.push(TRACK_COUNT * sizeof(u16)); //@ can these be smaller?
   u16 *unreachable = (u16 *)allocator.push(TRACK_COUNT * sizeof(u16));
   u16 removedTop = 0;
   u16 unreachableTop = 0;
   
   for(u16 i = 0; i < (u16)TRACK_COUNT; ++i)
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
	    graph.elements[i] = {};
	    graph.adjList[i] = {};
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
      for(u16 i = 0; i < TRACK_COUNT; ++i)
      {
	 if(graph.taken.e[i].flags & Element::occupied)
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

   // sort tracks by distance
   for(u16 i = 1; i < TRACK_COUNT; ++i)
   {
      Track track1 = graph.elements[i];
      Attribute attribute1 = graph.adjList[i];      
      u16 virt = graph.GetVirtualID(i);
      float distance1 = track1.renderable.worldPos.y - cameraPos.y;

      i16 j = i - 1;

      if(!(attribute1.flags & Attribute::unused))
      {
	 Track track2 = graph.elements[j];
	 Attribute attribute2 = graph.adjList[j];
	 float distance2 = track2.renderable.worldPos.y - cameraPos.y;
	 while(!(attribute2.flags & Attribute::unused) && distance2 > distance1)
	 {
	    graph.Move(j, j+1);
	    --j;

	    if(j >= 0)
	    {
	       track2 = graph.elements[j];
	       attribute2 = graph.adjList[j];
	       distance2 = track2.renderable.worldPos.y - cameraPos.y;
	    }
	    else
	    {
	       break;
	    }
	 }
	 
      }

      graph.elements[j+1] = track1;
      graph.adjList[j+1] = attribute1;      
      graph.SetID(virt, j+1);
   }
   
   allocator.pop();
   allocator.pop();
}

void NewUpdateTrackGraph(NewTrackGraph &graph, StackAllocator &allocator, Player &player, Camera &camera)
{
   for(u16 i = 0; i < TRACK_COUNT; ++i)
   {
      graph.adjList[i].flags &= ~Attribute::reachable;
      graph.adjList[i].flags &= ~Attribute::invisible;
   }

   NewSetReachable(graph, allocator, player.trackIndex);

   float cutoff = player.renderable.worldPos.y - (TRACK_SEGMENT_SIZE * 3.0f);

   for(u16 i = 0; i < TRACK_COUNT; ++i)
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

static B_INLINE
v3 GetPositionOnTrack(Track &track, float t)
{   
   v2 displacement = CubicBezier(*track.bezier, t);
   return V3(displacement.x + track.renderable.worldPos.x,
	     displacement.y + track.renderable.worldPos.y,
	     track.renderable.worldPos.z);
}

B_INLINE
void ResetPlayer(Player &player)
{
   player.trackIndex = 0;
   player.t = 0.0f;
   player.velocity = 0.3f;
   player.forceDirection = 0;
   player.tracksTraversedSequence = 0;
   player.timesAccelerated = 0;
   player.flags = 0;
}

inline void
Player::StartShrink()
{
   flags |= shrinking;
   shrink.t = 0.0f;
}

void OnLoss(GameState &state)
{
   u32 distance = state.sphereGuy.renderable.worldPos.y / 10.0f;
   if(distance > state.maxDistance)
   {
      state.maxDistance = distance;
      if(!SaveGame(state, (StackAllocator *)state.mainArena.base))
      {
	 B_ASSERT(0);
      }
   }

   state.sphereGuy.StartShrink();

   #ifdef ANDROID_BUILD
   LoadAndShowAds(state.android);
   #endif
}

void UpdatePlayer(Player &player, NewTrackGraph &tracks, GameState &state)
{
   // current track physical ID
   u16 actualID = tracks.GetActualID(player.trackIndex);
   Track *currentTrack = &tracks.elements[actualID];
   
   player.animation.t += delta;
   float scale = ((sinf(player.animation.t * 0.3f) + 1.0f) * 0.2f) + 0.7f;
   player.renderable.scale = V3(scale, scale, scale);
   

   if(!state.paused)
   {
      player.t += player.velocity * delta;
   }
   // if the player has just left the current track
   if(player.t > 1.0f)
   {
      ++player.tracksTraversedSequence;

      if(player.tracksTraversedSequence >= 300)
      {
	 if(player.timesAccelerated < 2)
	 {
	    player.velocity += 0.004f;
	    ++player.timesAccelerated;
	 }

	 player.tracksTraversedSequence = 0;
	 BeginChangeWorldColor(state);
      }

      player.t -= 1.0f;

      if(((tracks.flags & NewTrackGraph::left) && !(tracks.adjList[actualID].flags & Attribute::lockedRight)) ||
	 (currentTrack->flags & Track::branch) == 0 ||
	 tracks.adjList[actualID].flags & Attribute::lockedLeft)
      {
	 u16 trackActual = tracks.GetActualID(player.trackIndex);
	 if(tracks.adjList[trackActual].hasLeft())
	 {
	    u16 newIndex = tracks.adjList[trackActual].leftEdge();
	    actualID = tracks.GetActualID(newIndex);
	    currentTrack = &tracks.elements[actualID];
	    player.trackIndex = newIndex;
	 }
	 else
	 {
	    player.t = 0.9f;
	    state.state = GameState::RESET;
	    OnLoss(state);
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
	 }
	 else
	 {
	    player.t = 0.9f;
	    state.state = GameState::RESET;
	    OnLoss(state);
	 }
      }

      player.forceDirection = 0;
   }
   
   player.renderable.worldPos = GetPositionOnTrack(*currentTrack, player.t);
}

inline float
ShrinkInterpolation(float a, float b, float t)
{
   return a + ((t * t * t) * (b - a));
}

inline
void UpdatePlayerShrink(Player &player)
{
   if(player.flags & Player::shrinking)
   {
      player.shrink.t += delta * 0.01f;

      if(player.shrink.t >= 1.0f)
      {
	 player.shrink.t = 1.0f;
	 player.flags &= ~Player::shrinking;
      }

      float shrunk = ShrinkInterpolation(1.0f, 0.0f, player.shrink.t);
      player.renderable.scale = V3(shrunk, shrunk, shrunk);
   }
}

void UpdateCamera(Camera &camera, Player &player, NewTrackGraph &graph)
{
   v3 playerPosition = player.renderable.worldPos;
   camera.position.x = lerp(camera.position.x, playerPosition.x, delta * 0.1f);
   camera.position.y = playerPosition.y + camera.distanceFromPlayer;
   camera.UpdateView();
}

static B_INLINE
v2 Tangent(Curve c, float t)
{
   v2 result;

   float minust = 1.0f - t;

   result.x = 3 * (minust * minust) * (c.p2.x - c.p1.x) + 6.0f * minust * t * (c.p3.x - c.p2.x) + 3.0f * t * t * (c.p4.x - c.p3.x);
   result.y = 3 * (minust * minust) * (c.p2.y - c.p1.y) + 6.0f * minust * t * (c.p3.y - c.p2.y) + 3.0f * t * t * (c.p4.y - c.p3.y);

   return unit(result);
}

B_INLINE
Player InitPlayer()
{
   Player result;
   result.renderable = {V3(0.0f, -2.0f, 5.0f),
			V3(1.0f, 1.0f, 1.0f),
			Quat(1.0f, 0.0f, 0.0f, 0.0f)};

   result.mesh = Sphere;
   result.velocity = 0.3f;
   result.t = 0.0f;   
   result.trackIndex = 0;
   result.tracksTraversedSequence = 0;
   result.timesAccelerated = 0;
   result.flags = 0;

   result.forceDirection = 0;

   result.animation.t = 0.0f;

   return result;
}

static B_INLINE
Branch_Image_Header *LoadImageFromAsset(Asset &asset)
{
   return (Branch_Image_Header *)asset.mem;
}

void SetTrackMeshesForRebuild(Track *tracks, u32 count)
{
   for(u32 i = 0; i < count; ++i)
   {
      if(tracks[i].flags & Track::branch)
      {
	 tracks[i].bezier = &GlobalBranchCurve;
      }
      else if(tracks[i].flags & Track::breaks)
      {
	 tracks[i].bezier = &GlobalBreakCurve;
      }
      else // linear
      {
	 tracks[i].bezier = &GlobalLinearCurve;
      }
   }
}

static B_INLINE
bool VerifySaveFile(u8 *fileBuffer, u32 size)
{
   if(size >= 6)
   {
      bool result =
	 fileBuffer[0] == 'B' ||
	 fileBuffer[1] == 'R' ||
	 fileBuffer[2] == 'A' ||
	 fileBuffer[3] == 'N' ||
	 fileBuffer[4] == 'C' ||
	 fileBuffer[5] == 'H';

      return result;
   }

   return false;
}

static B_INLINE
u32 GetMaxDistanceFromFile(u8 *fileBuffer)
{
   return *((u32 *)(fileBuffer + 6));
}

#ifdef DEBUG

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#pragma warning(pop)

WorldPallette QuickLoadPalette(char *name)
{
   int x, y, channels;
   u8 *data = stbi_load(name, &x, &y, &channels, 3);

   WorldPallette result;
   float *floatArr = (float *)&result;

   // 18 floats in 6 color
   for(i32 i = 0; i < 18; ++i)
   {
      floatArr[i] = (float)data[i] / 255.0f;
   }

   stbi_image_free((void *)data);

   return result;
}
#endif

// Pulled out of GameInit because Android doesn't give you the dimensions of the screen until
// NATIVEWINDOWCREATED
void CreateProjection()
{
   Projection = Projection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 100.0f, 60.0f);
   InfiniteProjection = InfiniteProjection3D(SCREEN_WIDTH, SCREEN_HEIGHT, 0.01f, 80.0f);
}

void GameInit(GameState &state, RebuildState *rebuild, size_t rebuildSize)
{
   INIT_LOG();

   state.mainArena.base = AllocateSystemMemory(MEGABYTES(512), &state.mainArena.size);
   LOG_WRITE("memory: %p", state.mainArena.base);
   state.mainArena.current = state.mainArena.base;
   InitStackAllocator((StackAllocator *)state.mainArena.base);
   StackAllocator *stack = (StackAllocator *)state.mainArena.base;
   
   state.assetManager.Init(stack);

   #if !defined(DEBUG) || defined(ANDROID_BUILD)
   colorTable[0] = {V3(1.0f, 0.0f, 0.0f), V3(0.0f, 0.0f, 0.0f),
		    V3(0.0f, 0.0f, 0.0f), V3(1.0f, 0.0f, 0.0f),
		    V3(1.0f, 0.0f, 0.0f), V3(0.0f, 1.0f, 0.0f)};
   colorTable[1] = {V3(0.0f, 0.0f, 1.0f), V3(1.0f, 1.0f, 1.0f),
		    V3(0.0f, 0.0f, 0.0f), V3(1.0f, 0.0f, 0.0f),
		    V3(0.839f, 0.149f, 1.0f), V3(0.0f, 0.0f, 0.0f)};
   colorTable[2] = {V3(0.239f, 0.522f, 0.012f), V3(0.0f, 0.0f, 0.0f),
		    V3(0.0f, 0.0f, 0.0f), V3(1.0f, 0.0f, 0.0f),
		    V3(0.867f, 0.706f, 0.0f), V3(1.0f, 1.0f, 1.0f)};
   colorTable[3] = {V3(0.447f, 0.129f, 0.435f), V3(0.169f, 0.667f, 0.227f),
		    V3(0.0f, 0.0f, 0.0f), V3(1.0f, 0.0f, 0.0f),
		    V3(1.0f, 1.0f, 0.0f), V3(0.0f, 1.0f, 0.0f)};
   colorTable[4] = {V3(0.384f, 0.0f, 0.482f), V3(0.851f, 0.627f, 0.906f),
		    V3(0.0f, 0.0f, 0.0f), V3(1.0f, 0.0f, 0.518f),
		    V3(1.0f, 0.0f, 0.518f), V3(0.0f, 0.0f, 0.0f)};
#else
   colorTable[0] = colorTable[1] = colorTable[2] = colorTable[3] = colorTable[4] = QuickLoadPalette("temppalette1.png");
#endif

   CreateProjection();
   state.renderer = InitRenderState(stack, state.assetManager);
   
   B_ASSERT(state.mainArena.base);
   srand((u32)bclock());   

   state.keyState = up;
   
   // Generate Track Beziers
   GlobalLinearCurve = LinearCurve(0, 0, 0, 1);
   GlobalBreakCurve = BreakCurve();
   GlobalLeftCurve = LEFT_CURVE;
   GlobalRightCurve = RIGHT_CURVE;
   GlobalBranchCurve = LEFT_CURVE;
   state.state = GameState::START;   

   LOG_WRITE("%zu : %zu", sizeof(RebuildState), rebuildSize);

   AllocateTrackGraphBuffers(state.tracks, stack);
   state.tracks = InitNewTrackGraph(stack);
   InitCamera(state.camera);
   state.sphereGuy = InitPlayer();
   FillGraph(state.tracks);

   LinearTrack = AllocateMeshObject(80 * 3, stack);
   BranchTrack = AllocateMeshObject(80 * 3, stack);
   BreakTrack = AllocateMeshObject(80 * 3, stack);
   LeftBranchTrack = AllocateMeshObject(80 * 3, stack);
   RightBranchTrack = AllocateMeshObject(80 * 3, stack);  

   state.paused = false;
   
   u32 fileSize;
   u8 *fileBuffer = GetSaveFileBuffer(state, stack, &fileSize);
   if(fileBuffer)
   {
      if(VerifySaveFile(fileBuffer, fileSize))
      {
	 state.maxDistance = GetMaxDistanceFromFile(fileBuffer);
      }
      else
      {
	 state.maxDistance = 0;
      }

      // release filebuffer;
      stack->pop();
   }
   else
   {
      state.maxDistance = 0;
   }
}

template <typename int_type> static B_INLINE
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

B_INLINE
v2 ScreenToClip(v2i screen)
{
   float x = (float)screen.x;
   float y = (float)screen.y;

   v2 result;
   result.x = (2.0f * x / (float)SCREEN_WIDTH) - 1.0f;
   result.y = -((2.0f * y / (float)SCREEN_HEIGHT) - 1.0f);

   return result;
}

void TracksProcessInput(GameState &state)
{
   if(state.input.Touched())
   {
      u16 actual = state.tracks.GetActualID(state.sphereGuy.trackIndex);
      if(state.sphereGuy.OnSwitch(state.tracks) &&
	 !(state.tracks.adjList[actual].flags & Attribute::lockedMask))
      {	 
	 Track &on = state.tracks.elements[actual];	 

	 if(state.tracks.flags & NewTrackGraph::left)
	 {
	    on.bezier = &GlobalLeftCurve;
	    LockedBranchTrack = &LeftBranchTrack;
	    state.tracks.adjList[actual].flags |= Attribute::lockedLeft;
	 }
	 else
	 {
	    on.bezier = &GlobalRightCurve;
	    LockedBranchTrack = &RightBranchTrack;
	    state.tracks.adjList[actual].flags |= Attribute::lockedRight;
	 }
      }
      
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

static inline
float GetXtoYRatio(MapItem item)
{
   float width = item.x1 - item.x0;
   float height = item.y1 - item.y0;

   return width / height;
}

#ifdef DEBUG

u16 FindBreakBeforeBranch(NewTrackGraph &g, u16 id)
{
   u16 last = id;
   for(;;)
   {
      u16 actual = g.GetActualID(last);
      Attribute &attr = g.adjList[actual];

      if(attr.flags & Attribute::branch) return 0;
      if(attr.flags & Attribute::breaks) return 1;

      B_ASSERT(attr.hasLeft());

      last = attr.leftEdge();
   }
}

void AutoPlay(GameState &state)
{
   NewTrackGraph &g = state.tracks;
   Player &p = state.sphereGuy;

   u32 trackIndex = g.IDtable[p.trackIndex];
   Attribute &attribute = g.adjList[trackIndex];

   if(attribute.flags & Attribute::branch) return;

   u32 nextTrackIndex = g.IDtable[attribute.leftEdge()];
   Attribute &nextAttribute = g.adjList[nextTrackIndex];

   if(!(nextAttribute.flags & Attribute::branch)) return;

   u32 nextNextVirtualIndex;
   if(g.flags & NewTrackGraph::left)
   {
      nextNextVirtualIndex = nextAttribute.leftEdge();
   }
   else
   {
      nextNextVirtualIndex = nextAttribute.rightEdge();
   }

   Attribute &nextNextAttribute = g.adjList[g.IDtable[nextNextVirtualIndex]];

   if(FindBreakBeforeBranch(state.tracks, nextNextAttribute.id))
   {
      state.input.SetTouched();
   }
}
#endif

#if defined(DEBUG) && defined(WIN32_BUILD)
void ControlCamera(GameState &state)
{
   v3 forward = V3(0.0f, 1.0f, 0.0f);
   v3 right = V3(1.0f, 0.0f, 0.0f);
   if(state.input.w())
   {
      state.camera.position = state.camera.position + (delta * forward);
   }

   if(state.input.a())
   {
      state.camera.position = state.camera.position - (delta * right);
   }

   if(state.input.s())
   {
      state.camera.position = state.camera.position - (delta * forward);
   }

   if(state.input.d())
   {
      state.camera.position = state.camera.position + (delta * right);
   }
}
#define DEBUG_CONTROL_CAMERA(state) ControlCamera(state)

#else
#define DEBUG_CONTROL_CAMERA(state)
#endif

static B_INLINE
void PushMaxDistanceDisplay(GameState &state, v2 location)
{
   static char best[32];
   sprintf(best, "Best %i", state.maxDistance);
   size_t length = strlen(best);
   state.renderer.commands.PushRenderText(best, (u32)length, location, V2(0.5f, 0.5f), state.renderer.currentColors.textc, ((StackAllocator *)state.mainArena.base));
}

static B_INLINE
void PushCurrentDistanceDisplay(GameState &state, v2 location)
{
   static char print_string[32];
   i32 print_string_count = 0;
   print_string_count = IntToString((char *)print_string, (u32)state.sphereGuy.renderable.worldPos.y / 10);
   state.renderer.commands.PushRenderText(print_string, print_string_count, location, V2(0.0f, 0.0f), state.renderer.currentColors.textc, ((StackAllocator *)state.mainArena.base));   
}

#ifdef DEBUG
static B_INLINE
void PushFramerate(GameState &state, v2 location)
{
   static char framerate[8];
   static int frame_id = 0;
   static i32 framerate_string_count = 0;

   if(frame_id == 0)
   {
      framerate_string_count = IntToString(framerate, state.framerate);
      frame_id = 60;
   }

   state.renderer.commands.PushRenderText(framerate, framerate_string_count, location, V2(0.0f, 0.0f), state.renderer.currentColors.textc, ((StackAllocator *)state.mainArena.base));

   --frame_id;
}

static B_INLINE
void PushCycles(GameState &state, v2 location, u64 count)
{
   static char cycles[16];
   i32 cycle_string_count = 0;
   cycle_string_count = IntToString(cycles, count);
   state.renderer.commands.PushRenderText(cycles, cycle_string_count, location, V2(0.0f, 0.0f), state.renderer.currentColors.textc, ((StackAllocator *)state.mainArena.base));  
}

#define DebugPushFramerate(state, location) PushFramerate(state, location)
#define DebugPushCycles(state, location, count) PushCycles(state, location, count)
#else
#define DebugPushFramerate(state, location)
#define DebugPushCycles(state, location, count)
#endif

void UpdateTracks(GameState &state)
{
   if(state.tracks.flags & NewTrackGraph::switching)
   {
      state.tracks.switchDelta = min(state.tracks.switchDelta + 0.1f * delta, 1.0f);

      GlobalBranchCurve = lerp(state.tracks.beginLerp, state.tracks.endLerp, smoothstep(0.0f, 1.0f, state.tracks.switchDelta));
      GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve, (StackAllocator *)state.mainArena.base);

      if(state.tracks.switchDelta == 1.0f)
      {
	 state.tracks.flags &= ~NewTrackGraph::switching;
      }
   }	 

   NewUpdateTrackGraph(state.tracks, *((StackAllocator *)state.mainArena.base), state.sphereGuy, state.camera);	    
}

void GameLoop(GameState &state)
{
   static u64 cycles = 0;
   static u64 frames = 0;
   BEGIN_TIME();
   BeginFrame(state);

   switch(state.state)
   { 
      case GameState::LOOP:
      {
	 v2 button_pos = V2(0.9f, 0.1f + USABLE_SCREEN_BOTTOM(state));
	 v2 button_scale = V2(GetXtoYRatio(GUIMap::pause_box) * 0.16f, 0.16f);	 

	 if(ButtonUpdate(button_pos, button_scale, state.input) == Clicked)
	 {
	    state.state = GameState::PAUSE;
	 }
	 else
	 {
	    #ifdef DEBUG
	    // AutoPlay(state);
	    #endif
	    TracksProcessInput(state);

	    UpdateTracks(state);
	    UpdatePlayer(state.sphereGuy, state.tracks, state);
	    UpdateCamera(state.camera, state.sphereGuy, state.tracks);
	 }

	 PushRenderTracks(state, (StackAllocator *)state.mainArena.base);
	 DebugPushFramerate(state, V2(-0.8f, 0.6f));
	 PushCurrentDistanceDisplay(state, V2(0.0f, 0.75f));
	 state.renderer.commands.PushDrawGUI(button_pos, button_scale, state.glState.guiTextureMap, state.glState.pauseButtonVbo, ((StackAllocator *)state.mainArena.base));
	 state.lightPos = state.sphereGuy.renderable.worldPos + V3(0.0f, 0.0f, 5.0f);

	 // @TODO This should be inserted into the render queue instead of being drawn immediately.
	 glUseProgram(DefaultShader.programHandle);
	 RenderObject(state.sphereGuy.renderable, state.sphereGuy.mesh, &DefaultShader, state.camera.view, state.lightPos, state.renderer.currentColors.playerc);

	 static int frame_id = 0;
	 static u64 value = 0;
	 if(frames > 0 && frame_id == 0)
	 {
	    value = cycles / frames;
	    frame_id = 60;
	 }
	 DebugPushCycles(state, V2(-0.6f, 0.8f), value);
	 --frame_id;

      }break;

      case GameState::PAUSE:
      {
	 DEBUG_CONTROL_CAMERA(state);

	 glUseProgram(DefaultShader.programHandle);
	 RenderObject(state.sphereGuy.renderable, state.sphereGuy.mesh, &DefaultShader, state.camera.view, state.lightPos, state.renderer.currentColors.playerc);
	 PushRenderTracks(state, (StackAllocator *)state.mainArena.base);

	 v2 button_pos = V2(0.0f, 0.0f);
	 v2 button_scale = V2(GetXtoYRatio(GUIMap::play_box) * 0.16f, 0.16f);
	 state.renderer.commands.PushDrawGUI(V2(0.0f, 0.0f), button_scale, state.glState.guiTextureMap, state.glState.playButtonVbo, ((StackAllocator *)state.mainArena.base));

	 if(ButtonUpdate(button_pos, button_scale, state.input) == Clicked)
	 {
	    state.state = GameState::LOOP;
	 }

	 PushMaxDistanceDisplay(state, V2(0.0f, -0.2f));
      }break;

      case GameState::RESET:
      {	 
	 DEBUG_CONTROL_CAMERA(state);

	 UpdatePlayerShrink(state.sphereGuy);
	 glUseProgram(DefaultShader.programHandle);
	 RenderObject(state.sphereGuy.renderable, state.sphereGuy.mesh, &DefaultShader, state.camera.view, state.lightPos, state.renderer.currentColors.playerc);

	 // We want to keep updating the tracks when we lose so that
	 // If we hit a break and the tracks were in the middle of switching
	 // it will finish that switch.
	 UpdateTracks(state);

	 v2 button_area = V2(2.0f, 2.0f);
	 v2 button_scale = V2(GetXtoYRatio(GUIMap::GoSign_box) * 0.16f, 0.16f);
	 if(ButtonUpdate(V2(0.0f, 0.0f), button_area, state.input) == Clicked)
	 {
	    ResetPlayer(state.sphereGuy);
	    ResetRenderer(state.renderer);
	    InitCamera(state.camera);

	    ResetGraph(state.tracks);
	    FillGraph(state.tracks);

	    state.state = GameState::START;
	    
	    // Reset track
	    state.sphereGuy.trackIndex = 0;
	    state.sphereGuy.t = 0.0f;

	    GlobalBranchCurve = BranchCurve(0, 0,
					    -1, 1);

	    GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve, (StackAllocator *)state.mainArena.base);
	 }

	 PushRenderTracks(state, (StackAllocator *)state.mainArena.base);
	 state.renderer.commands.PushDrawGUI(V2(0.0f, 0.0f), button_scale, state.glState.guiTextureMap, state.glState.startButtonVbo, ((StackAllocator *)state.mainArena.base));
	 PushMaxDistanceDisplay(state, V2(0.0f, -0.2f));
	 PushCurrentDistanceDisplay(state, V2(0.0f, 0.75f));
	 
      }break;

      case GameState::START:
      {
	 // Start menu of the game
	 static float position = 0.0f;
	 position += delta * 0.01f;

	 v2 button_area = V2(2.0f, 2.0f);
	 v2 button_scale = V2(GetXtoYRatio(GUIMap::GoSign_box) * 0.16f, 0.16f);
	 if(ButtonUpdate(V2(0.0f, 0.0f), button_area, state.input) == Clicked)
	 {
	    state.state = GameState::LOOP;	    
	    state.sphereGuy.trackIndex = 0;
	    state.sphereGuy.t = 0.0f;

	    GlobalBranchCurve = BranchCurve(0, 0,
					    -1, 1);

	    GenerateTrackSegmentVertices(BranchTrack, GlobalBranchCurve, (StackAllocator *)state.mainArena.base);
	    
	 }

	 PushRenderTracks(state, (StackAllocator *)state.mainArena.base);
	 
	 state.renderer.commands.PushDrawGUI(V2(0.0f, 0.0f), button_scale, state.glState.guiTextureMap, state.glState.startButtonVbo, ((StackAllocator *)state.mainArena.base));
	 PushMaxDistanceDisplay(state, V2(0.0f, -0.2f));
	 // DebugPushFramerate(state, V2(-0.8f, 0.8f));
      }break;
      
      default:
      {
	 B_ASSERT(!"invalid state");
      }
   }

   state.renderer.commands.ExecuteCommands(state.camera, state.lightPos, state.glState.bitmapFont, state.bitmapFontProgram, state.renderer, (StackAllocator *)state.mainArena.base, state.glState);
   state.renderer.commands.Clean(((StackAllocator *)state.mainArena.base));

   END_TIME();
   if(state.state == GameState::LOOP)
   {
      u64 passed;
      READ_TIME(passed);
      cycles += passed;
      ++frames;
   }
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
}

#ifdef DEBUG

// only good on creation
// ID's after sorting do not necassarily = i
void
NewTrackGraph::VerifyIDTables()
{
   for(u16 i = 0; i < TRACK_COUNT; ++i)
   {
      B_ASSERT(IDtable[reverseIDtable[i]] == i);
   }
}

void
NewTrackGraph::VerifyGraph()
{
   for(u32 i = 0; i < TRACK_COUNT; ++i)
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

		  B_ASSERT(good);
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

		  B_ASSERT(good);
	       }
	    }
	 }
	 else if(adjList[i].flags & Attribute::breaks)
	 {
	    B_ASSERT(!(adjList[i].flags & (Attribute::hasLeftEdge | Attribute::hasRightEdge)));

	    // There really isn't a good way to test if there is a branch behind a break,
	    // since tracks are removed once they become invisible
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

	       if(!good)
	       {
		  // B_ASSERT(good);
	       }
	    }
	 }
	 else
	 {
	    B_ASSERT(0);
	 }
      }
   }
}
#endif

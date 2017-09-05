
enum ButtonUpdateResult
{
   Clicked,
   Hovered,
   Up,
   None,
};

__forceinline
bool TestPoint(v2 boxPosition, v2 boxScale, v2 point)
{
   return point.x < (boxPosition.x + boxScale.x) && point.x > (boxPosition.x - boxScale.x) &&
      point.y < (boxPosition.y + boxScale.y) && point.y > (boxPosition.y - boxScale.y);
}

ButtonUpdateResult ButtonUpdate(v2 position, v2 scale, Platform_Input_State state)
{
   if(state.Touched())
   {
      if(TestPoint(position, scale, ScreenToClip(state.TouchPoint())))
      {
	 return Clicked;
      }
      else
      {
	 return None;
      }
   }
   if(state.UnTouched())
   {
      if(TestPoint(position, scale, ScreenToClip(state.TouchPoint())))
      {
	 return Up;
      }
      else
      {
	 return None;
      }
   }
   else if(TestPoint(position, scale, ScreenToClip(state.TouchPoint())))
   {
      return Hovered;
   }
   else
   {
      return None;
   }
}

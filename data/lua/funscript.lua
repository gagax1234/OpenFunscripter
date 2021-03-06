-- ============================
-- == DO NOT EDIT THIS FILE ==
-- ============================

-- This file contains a really basic api to modify funscripts.

Action = {at = 0, pos = 0, selected = false, tag = 0}

-- internal use only
function Action:new(o)
   o = o or {}
   setmetatable(o, self)
   self.__index = self
   self.__tostring = function(self) return string.format("at:%d pos:%d", self.at, self.pos) end
   self.at = at or 0
   self.pos = pos or 0
   self.tag = tag or 0 -- an arbitrary tag that scripts can set which will persist till OFS gets closesd. 
                       -- it's one byte so only values from 0 to 255 are valid
   self.selected = false
   return o
end

Funscript = { title = "", path = "" }

-- tostring function for funscript
function FunscriptToString(self)
   local result = "actions: "
   for i,v in ipairs(self.actions) do
      result = result..tostring(v)
      result = result..";\n"
   end
   return result
end

-- can be used to temporarily store generated actions
-- before adding them to CurrentScript
function Funscript:new(o)
   o = o or {}
   setmetatable(o, self)
   self.__index = self
   self.__tostring = FunscriptToString
   
   o.title = "" -- this is a readonly property filled in by OFS
   o.path = "" -- this is a readonly property. contains the file path
   o.actions = {}
   return o
end

-- removes all actions
function Funscript:Clear()
   self.actions = {}
end

-- adds an action & makes sure it's in temporal order
function Funscript:AddAction(at, pos, selected)
   local newAction = Action:new()
   newAction.at = at
   newAction.pos = pos
   newAction.selected = selected or false

   -- find position to insert
   local insertPosition = #self.actions + 1
   for i, v in ipairs(self.actions) do
      if at == v.at then
         print("ERROR: can't add action with the same timestamp: " .. at)
         return
      elseif v.at > at then
         insertPosition = i
         break
      end
   end

   -- insert into the array/table
   if insertPosition >= 1 then
      table.insert(self.actions, insertPosition, newAction)
   else
      print("ERROR: failed to add action at: " .. at)
   end
end


-- adds action & ignores ordering
-- OFS will order them correctly after the script ran
-- using this function when order doesn't matter greatly improves performance
function Funscript:AddActionUnordered(at, pos, selected, tag)
   local newAction = Action:new()
   newAction.at = at
   newAction.pos = pos
   newAction.selected = selected or false
   newAction.tag = tag or 0

   table.insert(self.actions, newAction)
end


-- removes an action at a given index
function Funscript:RemoveAction(idx) 
   -- this doesn't work while iterating the array/table
   table.remove(self.actions, idx)
end

-- removes all selected actions
function Funscript:RemoveSelected()
   local toBeRemoved = {}
   for i, v in ipairs(self.actions) do
      if v.selected then
         table.insert(toBeRemoved, i)
      end
   end
   table.sort(toBeRemoved)
   for i = #toBeRemoved, 1, -1 do
      table.remove(self.actions, toBeRemoved[i])
   end
end

-- clears the selection
function Funscript:DeselectAll()
   for i, v in ipairs(self.actions) do
      v.selected = false
   end
end


-- returns the closest action or nil after the given time in milliseconds
function Funscript:GetClosestActionAfter(time_ms)
   for i,v in ipairs(self.actions) do
      if v.at > time_ms then
         return v
      end
   end
   return nil
end

-- returns the closest action or nil before the given time in milliseconds
function Funscript:GetClosestActionBefore(time_ms)
   for i, v in ipairs(self.actions) do
      if v.at > time_ms then
         return self.actions[i-1]
      end
   end
   return nil
end

-- context variables
CurrentScript = Funscript:new() -- the currently active funscript.

CurrentScriptIdx = 0 -- the index of the currently active funscript. only relevant when multiple scripts are loaded
LoadedScripts = {}  -- contains an array of all loaded scripts. should MUST NOT be reordered OFS expects them to come out in the order they came in

CurrentTimeMs = 0 -- holds the current player time in ms. can also set the current position
FrameTimeMs = 0  -- holds the time of a single frame in ms 60 fps => 1/60 seconds
TotalTimeMs = 0 -- hold the total duration of the video in ms

Clipboard = Funscript:new() -- contains the currently copied actions. this is readonly

VideoFilePath = "" -- the path of the currently loaded video
VideoFileDirectory = "" -- the directory of the currently loaded video


-- native functions
-- right now there's two functions which you can call from lua which call back into OFS
-- SetProgress(float) -- to set the progress for long processes with a value from 0.0 to 1.0
-- SetSettings(table) -- call SetSettings with a lua table to export variables which can be edited in OFS. check out the examples for concrete usage

-- utility functions which can be called
-- round a number
function round(x)
   return x >= 0 and math.floor(x+0.5) or math.ceil(x-0.5)
end

-- clamp a value
function clamp(val, min_value, max_value)
   return math.min(math.max(val, min_value), max_value)
end

-- linear interpolation
function lerp(start_val, end_val, t) 
   return start_val * (1-t) + end_val * t 
end
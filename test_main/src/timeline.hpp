#pragma once

#include <higanbana/core/datastructures/vector.hpp>

namespace app
{

struct Event {
  uint64_t start;
  uint64_t duration;
  // some thing is played while this is on...
  // some value is edited per frame
  
};

class Track {
  higanbana::vector<Event> m_events; 
};
class Timeline {
  uint64_t m_currentTime = 0;
  higanbana::vector<Track> m_tracks;


};
}
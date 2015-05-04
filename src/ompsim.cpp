// OMPSIM.cpp. Discrete event simmulator.
//

#include "stdafx.h"
#include "OMPSIM.h"

#include <stdint.h>
#include <vector>
#include <queue>
#include <functional>
#include <memory>

// Represents time in microseconds. That is about 292K years.
class SimTime {
  int64_t st_;
  static SimTime* g_master;

public:
  static void init_master() {
    if (g_master)
      delete g_master;
    g_master = new SimTime();
  }

  static const void advance(int64_t u_seconds) {
    g_master->st_ += u_seconds;
  }

  static const SimTime& now() { return *g_master; }

  SimTime() : st_(0) {}
  explicit SimTime(int seconds) : st_(100 * seconds) {}
  explicit SimTime(int64_t u_seconds) :st_(u_seconds) {}
  bool operator==(const SimTime& other) const { return st_ == other.st_; }
  bool operator<(const SimTime& other) const { return st_ < other.st_; }
  bool operator>(const SimTime& other) const { return st_ > other.st_; }
};

SimTime* SimTime::g_master = nullptr;

class Scheduler;

class Actor {
public:
  virtual ~Actor() {}
  virtual void on_event(Scheduler* sched, SimTime btime, uint32_t id) = 0;
};

class SimEvent {
  // Activation time and birth time.
  SimTime atime_;
  SimTime btime_;
  Actor* actor_;
  uint32_t id_;

public:
  SimEvent(SimTime activation_time, Actor* actor, uint32_t id) 
      : atime_(activation_time), btime_(SimTime::now()), 
        actor_(actor), id_(id) {}

  virtual bool run(Scheduler* sched) {
    if (actor_) {
      actor_->on_event(sched, btime_, id_);
    } else {
      // $$ loop over all actors??
    }
    return true;
  }

  bool operator>(const SimEvent& other) const { return atime_ > other.atime_; }
};

class Scheduler {
  std::priority_queue<SimEvent, std::vector<SimEvent>, std::greater<SimEvent>> pqueue_;

public:
  bool empty() const { return pqueue_.empty(); }

  void run_top() { 
    auto& ev = pqueue_.top();
    ev.run(this);
    pqueue_.pop();
  }
};

class Simulation {
  Scheduler sched_;

public:
  Simulation()  {
    SimTime::init_master();
  }

  virtual int prime(Scheduler* sched) = 0;

  int run(int max_seconds) {
    // Prime the simulation.
    int rv = prime(&sched_);
    if (rv != 0) {
      fatal_error("priming sim failed", rv);
      return rv;
    }

    // Simulate until no more events pending or end-of-time.
    SimTime max_time(max_seconds);
    while ((SimTime::now() < max_time) && !sched_.empty()) {
      sched_.run_top();
    }

    return 0;
  }

private:
  void fatal_error(const char* what, int code) {
    __debugbreak();
  }

};

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace OMP {

class Person : public Actor {
  SimTime age_;

public:
  Person(Scheduler* sched) {

  }

protected:
  void on_event(Scheduler* sched, SimTime btime, uint32_t id) override {

  }
};

class OMPSim : public Simulation {
  std::vector<std::unique_ptr<Person>> people_;

  int prime(Scheduler* sched) override {

    return 0;
  }
};


}  // namepsace OMP.

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE,
                    wchar_t* cmd_line,
                    int cmd_show) {
  MSG msg;

  while (::GetMessage(&msg, NULL, 0, 0)) {
    ::DispatchMessage(&msg);
  }

  return (int) msg.wParam;
}


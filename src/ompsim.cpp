// OMPSIM.cpp. Discrete event simmulator.
//

#include "stdafx.h"
#include "OMPSIM.h"

#include <stdint.h>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <random>

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
  explicit SimTime(int64_t u_seconds) :st_(u_seconds) {}
  bool operator==(const SimTime& other) const { return st_ == other.st_; }
  bool operator<(const SimTime& other) const { return st_ < other.st_; }
  bool operator>(const SimTime& other) const { return st_ > other.st_; }

  SimTime& operator+=(int64_t u_seconds) {
    st_ += u_seconds;
    return *this;
  }

  friend SimTime operator+(const SimTime& time, int64_t u_seconds) {
    return SimTime(time) += u_seconds;
  }
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
  std::random_device randdev_;
  std::mt19937 randgen_;

public:
  Scheduler() : randgen_(randdev_()) { }

  bool empty() const { return pqueue_.empty(); }

  void run_top() { 
    auto& ev = pqueue_.top();
    ev.run(this);
    pqueue_.pop();
  }

  void add_event(int delta_seconds, Actor* actor, uint32_t id) {
    auto activation_time = SimTime::now() + (100 * uint64_t(delta_seconds));
    pqueue_.emplace(activation_time, actor, id);
  }

  SimTime random_uniform_seconds(int min_sec, int max_sec) {
    std::uniform_int_distribution<uint64_t> dis(min_sec, max_sec);
    return SimTime(100 * dis(randgen_));
  }

  int random_uniform_number(int min, int max) {
    std::uniform_int_distribution<> dis(min, max);
    return dis(randgen_);
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
  int energy_;   // calories.
  int weight_;   // lbs.

public:
  enum States {
    dead,
    sleeping,
    awake,
    eating,
    moving,
    working,
    relaxing,
  };

  Person() = delete;

  Person(Scheduler* sched)
      : age_(0),
        energy_(0),
        weight_(0) {

    int khour = 60 * 60;
    int kyear = 365 * 24 * khour;
    age_ = sched->random_uniform_seconds(kyear * 20, kyear * 90);
    weight_ = sched->random_uniform_number(90, 220);
    energy_ = 2 * (weight_ * 13);

    auto wakeup_time = sched->random_uniform_number(4, 8) * khour;
    sched->add_event(wakeup_time, this, awake);
  }

protected:
  void on_event(Scheduler* sched, SimTime btime, uint32_t id) override {

  }
};

class OMPSim : public Simulation {
  std::vector<std::unique_ptr<Person>> people_;

  int prime(Scheduler* sched) override {
    for (int ix = 0; ix != 100; ++ix) {
      people_.push_back(std::make_unique<Person>(sched));
    }
    return 0;
  }
};


}  // namepsace OMP.

DWORD WINAPI SpawnSimulation(void* ctx) {
  long mode = 1;
  if (ctx) {
    OMP::OMPSim simulation;
    auto rv = simulation.run(10000);
    return rv;
  } else {
    auto th = ::CreateThread(NULL, 0, &SpawnSimulation, &mode, 0, nullptr);
    return th? 1 : 0;
  }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE,
                    wchar_t* cmd_line,
                    int cmd_show) {
  if (!SpawnSimulation(nullptr))
    return -1;

  MSG msg;

  while (::GetMessage(&msg, NULL, 0, 0)) {
    ::DispatchMessage(&msg);
  }

  return (int) msg.wParam;
}


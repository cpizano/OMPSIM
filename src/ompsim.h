#pragma once

#include <stdint.h>
#include <vector>
#include <queue>
#include <functional>
#include <random>
#include <string>

#include "omp_utils.h"

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

  static const void set(const SimTime& new_time) {
    g_master->st_ = new_time.st_;
  }

  static const SimTime& now() { return *g_master; }

  SimTime() : st_(0) {}
  explicit SimTime(int64_t u_seconds) :st_(u_seconds) {}
  int64_t as_seconds() const { return st_ / 100; }

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

  friend int64_t operator-(const SimTime& time1, const SimTime& time2) {
    return time1.st_ - time2.st_;
  }

  std::string format() const {

    auto fus  = st_ % 100;
    auto sec = st_ / 100;
    auto min = sec / 60;
    if (min < 60) {
      return StringFmt("%dm:%d.%ds", min, sec % 60, fus);
    }

    auto hrs = min / 60;
    auto dys = hrs / 24;
    if (dys < 30) {
      return StringFmt("%dd %dh:%dm:%ds", dys, hrs % 24, min % 60, sec % 60);
    }

    return "??";
  }
};

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

  SimTime when() const { return atime_; }

  bool operator>(const SimEvent& other) const { return atime_ > other.atime_; }
};

class Scheduler {
  std::priority_queue<SimEvent, std::vector<SimEvent>, std::greater<SimEvent>> pqueue_;
  std::random_device randdev_;
  std::mt19937 randgen_;

public:
  Scheduler() : randgen_(randdev_()) { }

  bool empty() const { return pqueue_.empty(); }

  bool run_top() { 
    auto& ev = pqueue_.top();
    SimTime::set(ev.when());
    ev.run(this);
    pqueue_.pop();
    return !pqueue_.empty();
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

  int hour_of_day() const {
    return int(SimTime::now().as_seconds() / 3600 ) % 24;
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
    SimTime max_time(max_seconds * 100);
    while (SimTime::now() < max_time) {
      if (!sched_.run_top())
        break;
    }

    return 0;
  }

private:
  void fatal_error(const char* what, int code) {
    __debugbreak();
  }

};

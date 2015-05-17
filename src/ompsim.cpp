// OMPSIM.cpp. Discrete event simmulator.
//

#include "stdafx.h"

#include <memory>
#include <forward_list>

#include "ompsim.h"
#include "resource.h"

OMP_SIM_GLOBALS

omp::RandomUniform random;

class Bureaucrat {
public:
  enum States {
    none = __LINE__,
    go_work,
    lunch_break,
    go_home,
    last_id
  };

  static void become(omp::Scheduler* sched, omp::Actor* actor, int travel_time_am_min) {
    auto go_time = omp::to_minutes_hhmm(8, 0) - travel_time_am_min;
    sched->time_table().add_event(actor, go_time, go_work);
    auto ln_time = omp::to_minutes_hhmm(12, 0) - travel_time_am_min;
    sched->time_table().add_event(actor, ln_time, lunch_break);
    auto ret_time = omp::to_minutes_hhmm(17, 0);
    sched->time_table().add_event(actor, ret_time, go_home);
  }
};

class Person : public omp::Actor {
  int age_;      // days.
  int energy_;   // calories.
  int weight_;   // lbs.
  int prev_;     // previous event.

public:
  enum States {
    none = __LINE__,
    sleeping,
    awake,
    breakfasting,
    working,
    relaxing,
    last_id,
  };

  Person() = delete;

  Person(omp::Scheduler* sched)
      : age_(0),
        energy_(0),
        weight_(0),
        prev_(none) {
    age_ = omp::to_days_years(random.gen_int(20, 90));
    weight_ = random.gen_int(90, 220);
    energy_ = 2 * (weight_ * 13);
    // born a a bureaucrat.
    Bureaucrat::become(sched, this, 10);
    // first event.
    auto wakeup_time = omp::to_seconds_hours(random.gen_int(4, 7));
    sched->add_event(wakeup_time, this, awake);
  }

protected:
  void on_event(omp::Scheduler* sched, omp::SimTime btime, uint32_t id) override {
    auto t0 = btime.format();
    auto t1 = omp::SimTime::now().format();
    auto t2 = sched->hour_of_day();

    if (id == awake) {
      // cleanup then eat.
      energy_ -= 15 * int((omp::SimTime::now() - btime) / (10 * 3600));
      auto awake_time = omp::to_seconds_mins(random.gen_int(30, 60));
      sched->add_event(awake_time, this, breakfasting);
    } else if (id == breakfasting) {
      // breakfast just increases energy.
      energy_ += random.gen_int(300, 800);
    } else if (id == Bureaucrat::go_work) {
      // conmute then work.
      auto move_time = omp::to_seconds_mins(random.gen_int(15, 80));
      if (prev_ == sleeping) {
        // this event can arrive before we normally wake up! so we
        // skip breakfast start conmuting a bit late.
        move_time += omp::to_seconds_mins(15);
        energy_ -= 100;
      }
      sched->add_event(move_time, this, working);
    } else if (id == working) {
      // work, possibly until lunch break.
      energy_ -= 15 * int((omp::SimTime::now() - btime) / (10 * 3600));
    } else if (id == Bureaucrat::lunch_break) {
      // lunch, then back to work.
      energy_ += random.gen_int(500, 1000);
      auto lunch_time = omp::to_seconds_mins(random.gen_int(20, 40));
      sched->add_event(lunch_time, this, working);
    } else if (id == Bureaucrat::go_home) {
      auto move_time = omp::to_seconds_mins(random.gen_int(15, 80));
      sched->add_event(move_time, this, relaxing);
    } else if (id == relaxing) {
      // watch some tv then go to sleep.
      auto relax_time = omp::to_seconds_mins(random.gen_int(60, 90));
      sched->add_event(relax_time, this, sleeping);
    } else if (id == sleeping) {
      // dream, then perhaps wake up. it is possible to sleep more than
      // allowable for a breakfast.
      auto sleep_time = omp::to_seconds_hours(random.gen_int(5, 9));
      sched->add_event(awake, this, sleeping);
    } else {
      __debugbreak();
    }
    
    prev_ = id;
  }
};

class PeopleSim : public omp::Simulation {
  std::vector<std::unique_ptr<Person>> people_;

public:
  // timetable : 10 mins, stats: 5 mins.
  PeopleSim() : omp::Simulation(10, 5) {}

private:
  int prime(omp::Scheduler* sched) override {
    for (int ix = 0; ix != 10; ++ix) {
      people_.push_back(std::make_unique<Person>(sched));
    }
    return 0;
  }
};

const DWORD kDoneMSG = 6666;

DWORD WINAPI SpawnSimulation(void* ctx) {
  static DWORD tid = ::GetCurrentThreadId();

  if (ctx) {
    PeopleSim simulation;
    auto rv = simulation.run(3600 * 30);
    ::PostThreadMessage(tid, kDoneMSG, 0, 0);
    return rv;
  } else {
    auto th = ::CreateThread(NULL, 0, &SpawnSimulation, &tid, 0, nullptr);
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
    if (msg.message == kDoneMSG) {
      ::PostQuitMessage(0);
    }
    ::DispatchMessage(&msg);
  }

  return (int) msg.wParam;
}


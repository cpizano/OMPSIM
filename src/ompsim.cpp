// OMPSIM.cpp. Discrete event simmulator.
//

#include "stdafx.h"

#include <memory>

#include "ompsim.h"
#include "resource.h"


SimTime* SimTime::g_master = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace OMP {
const int kmin  = 60;
const int khour = 60 * kmin;
const int kdays = 365;

class Person : public Actor {
  int age_;      // days.
  int energy_;   // calories.
  int weight_;   // lbs.

  int prev_;     // previous event.

public:
  enum States {
    none,
    dead,
    sleeping,
    awake,
    eating,
    going_work,
    going_home,
    working,
    relaxing,
  };

  Person() = delete;

  Person(Scheduler* sched)
      : age_(0),
        energy_(0),
        weight_(0),
        prev_(none) {
    age_ = sched->random_uniform_number(20, 90) * kdays;
    weight_ = sched->random_uniform_number(90, 220);
    energy_ = 2 * (weight_ * 13);

    auto wakeup_time = sched->random_uniform_number(4, 8) * khour;
    sched->add_event(wakeup_time, this, awake);
  }

protected:
  void on_event(Scheduler* sched, SimTime btime, uint32_t id) override {
    auto t0 = btime.format();
    auto t1 = SimTime::now().format();
    auto t2 = sched->hour_of_day();

    if (id == awake) {
      energy_ -= 15 * int((SimTime::now() - btime) / (10 * khour));
      auto awake_time = sched->random_uniform_number(30, 60) * kmin;
      sched->add_event(awake_time, this, eating);
    } else if (id == eating) {
      auto eating_time = sched->random_uniform_number(10, 40) * kmin;
      energy_ += sched->random_uniform_number(300, 800);
      if (prev_ = awake) {
        sched->add_event(eating_time, this, going_work);
      } else {
        sched->add_event(eating_time, this, working);
      }
    } else if (id == going_work) {
      auto move_time = sched->random_uniform_number(15, 80) * kmin;
      sched->add_event(move_time, this, working);
    } else if (id == working) {
      energy_ -= 15 * int((SimTime::now() - btime) / (10 * khour));
      auto work_time = sched->random_uniform_number(2, 4) * khour;
      if (sched->hour_of_day() < 18) {
        sched->add_event(work_time, this, eating);
      } else {
        sched->add_event(work_time, this, going_home);
      }
    } else if (id == going_home) {
      auto move_time = sched->random_uniform_number(15, 80) * kmin;
      sched->add_event(move_time, this, relaxing);
    } else if (id == relaxing) {
      auto relax_time = sched->random_uniform_number(60, 90) * kmin;
      sched->add_event(relax_time, this, sleeping);
    } else if (id == sleeping) {
      auto sleep_time = sched->random_uniform_number(5, 9) * khour;
      sched->add_event(awake, this, sleeping);
    } else {
      __debugbreak();
    }
    
    prev_ = id;
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

const DWORD kDoneMSG = 6666;

DWORD WINAPI SpawnSimulation(void* ctx) {
  static DWORD tid = ::GetCurrentThreadId();

  if (ctx) {
    OMP::OMPSim simulation;
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


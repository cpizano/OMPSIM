// OMPSIM.cpp. Discrete event simmulator.
//

#include "stdafx.h"

#include <memory>

#include "ompsim.h"
#include "resource.h"


SimTime* SimTime::g_master = nullptr;

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
    auto t0 = btime.format();
    auto t1 = SimTime::now().format();

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
    auto rv = simulation.run(3600 * 10);
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


// This file is part of arduino-fsm.
//
// arduino-fsm is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// arduino-fsm is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with arduino-fsm.  If not, see <http://www.gnu.org/licenses/>.

#ifndef FSM_H
#define FSM_H


#if defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

struct State
{
  virtual void on_enter() = 0;
  virtual void on_exit() = 0;
};

struct CBState : public State
{
  CBState(void (*callback_enter)(), void (*callback_exit)());

  virtual void on_enter();
  virtual void on_exit();

  void (*callback_enter)();
  void (*callback_exit)();
};

template <class TClass>
struct TState : public State
{
  typedef void (TClass::*Method)();

  TState(TClass *instance, Method method_enter, Method method_exit);

  virtual void on_enter();
  virtual void on_exit();

  TClass *instance;
  Method method_enter;
  Method method_exit;
};

template <class TClass>
TState<TClass>::TState(TClass *instance, Method method_enter, Method method_exit)
: instance(instance),
  method_enter(method_enter),
  method_exit(method_exit)
{
}

template <class TClass>
void TState<TClass>::on_enter()
{
  if (instance != NULL && method_enter != NULL)
  {
    (instance->*method_enter)();
  }
}

template <class TClass>
void TState<TClass>::on_exit()
{
  if (instance != NULL && method_exit != NULL)
  {
    (instance->*method_exit)();
  }
}

struct Transition
{
  Transition();
  virtual ~Transition();
  State* state_from;
  State* state_to;
  int event;
  virtual void on_transition() = 0;
  bool to_delete;

  State* make_transition();
};

struct CBTransition : Transition
{
  virtual void on_transition();
  void (*callback_transition)();
};

template <class TClass>
struct TTransition : Transition
{
  typedef void (TClass::*Method)();
  virtual void on_transition();
  TClass *instance;
  Method method;
};

template <class TClass>
void TTransition<TClass>::on_transition()
{
  if (method != NULL && instance != NULL)
  {
    (instance->*method)();
  }
}

class Fsm
{
private:
  struct TimedTransition
  {
    Transition* transition;
    unsigned long start;
    unsigned long interval;
  };

  static Transition* create_transition(State* state_from, State* state_to,
    int event, void (*on_transition)());

public:
  Fsm(State* initial_state);
  ~Fsm();

  void add_transition(State* state_from, State* state_to, int event,
                      void (*on_transition)());

  void add_transition(Transition *transition);

  void add_timed_transition(State* state_from, State* state_to,
                            unsigned long interval, void (*on_transition)());

  void add_timed_transition(unsigned long interval, Transition *transition);

  void trigger(int event);
  void check_timer();

private:
  State* m_current_state;
  Transition** m_transitions;
  int m_num_transitions;

  TimedTransition* m_timed_transitions;
  int m_num_timed_transitions;
};


#endif

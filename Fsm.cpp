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

#include "Fsm.h"


CBState::CBState(void (*callback_enter)(), void (*callback_exit)())
: callback_enter(callback_enter),
  callback_exit(callback_exit)
{
}

void CBState::on_enter()
{
  if (callback_enter != NULL)
  {
    callback_enter();
  }
}

void CBState::on_exit()
{
  if (callback_exit != NULL)
  {
    callback_exit();
  }
}

Fsm::Fsm(State* initial_state)
: m_current_state(initial_state),
  m_transitions(NULL),
  m_num_transitions(0)
{
}


Fsm::~Fsm()
{
  for (int i = 0; i < m_num_transitions; ++i)
  {
    if (m_transitions[i]->to_delete)
    {
      delete m_transitions[i];
    }
  }
  free(m_transitions);
  free(m_timed_transitions);
  m_transitions = NULL;
  m_timed_transitions = NULL;
}

void Fsm::add_transition(State* state_from, State* state_to, int event,
                         void (*on_transition)())
{
  if (state_from == NULL || state_to == NULL)
    return;

  Transition *transition = Fsm::create_transition(state_from, state_to, event,
                                               on_transition);
  transition->to_delete = true;
  add_transition(transition);
}

void Fsm::add_transition(Transition *transition)
{
  if (transition == NULL)
    return;

  m_transitions = (Transition**) realloc(m_transitions, (m_num_transitions + 1)
                                                       * sizeof(Transition*));
  m_transitions[m_num_transitions] = transition;
  m_num_transitions++;
}

void Fsm::add_timed_transition(State* state_from, State* state_to,
                               unsigned long interval, void (*on_transition)())
{
 if (state_from == NULL || state_to == NULL)
   return;

 Transition *transition = Fsm::create_transition(state_from, state_to, 0,
                                                on_transition);
 transition->to_delete = true;
 add_timed_transition(interval, transition);
}

void Fsm::add_timed_transition(unsigned long interval, Transition *transition)
{
  if (transition == NULL)
    return;

  TimedTransition timed_transition;
  timed_transition.transition = transition;
  timed_transition.start = 0;
  timed_transition.interval = interval;

  m_timed_transitions = (TimedTransition*) realloc(
      m_timed_transitions, (m_num_timed_transitions + 1) * sizeof(TimedTransition));
  m_timed_transitions[m_num_timed_transitions] = timed_transition;
  m_num_timed_transitions++;
}


Transition* Fsm::create_transition(State* state_from, State* state_to,
                                       int event, void (*callback_transition)())
{
  CBTransition *t = new CBTransition();
  t->state_from = state_from;
  t->state_to = state_to;
  t->event = event;
  t->callback_transition = callback_transition;

  return t;
}

void Fsm::trigger(int event)
{
  // Find the transition with the current state and given event.
  for (int i = 0; i < m_num_transitions; ++i)
  {
    if (m_transitions[i]->state_from == m_current_state &&
        m_transitions[i]->event == event)
    {
      m_current_state = m_transitions[i]->make_transition();
      return;
    }
  }
}


void Fsm::check_timer()
{
  for (int i = 0; i < m_num_timed_transitions; ++i)
  {
    TimedTransition* transition = &m_timed_transitions[i];
    if (transition->transition->state_from == m_current_state)
    {
      if (transition->start == 0)
      {
        transition->start = millis();
      }
      else
      {
        unsigned long now = millis();
        Serial.println(now);
        if (now - transition->start >= transition->interval)
        {
          m_current_state = transition->transition->make_transition();
          transition->start = 0;
        }
      }
    }
  }
}

Transition::Transition()
{
  to_delete = false;
}

Transition::~Transition()
{
  // Nothing to do...
}

void CBTransition::on_transition()
{
  if (callback_transition != NULL)
  {
    callback_transition();
  }
}

State* Transition::make_transition()
{
  // Execute the handlers in the correct order.
  bool changed = (state_from != state_to);
  if (changed)
  {
    state_from->on_exit();
  }
  on_transition();
  if (changed)
  {
    state_to->on_enter();
  }

  return state_to;
}

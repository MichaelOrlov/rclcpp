// Copyright 2019 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdexcept>
#include <string>

#include "rcl/event.h"

#include "rclcpp/event_handler.hpp"
#include "rclcpp/exceptions/exceptions.hpp"

namespace rclcpp
{

UnsupportedEventTypeException::UnsupportedEventTypeException(
  rcl_ret_t ret,
  const rcl_error_state_t * error_state,
  const std::string & prefix)
: UnsupportedEventTypeException(exceptions::RCLErrorBase(ret, error_state), prefix)
{}

UnsupportedEventTypeException::UnsupportedEventTypeException(
  const exceptions::RCLErrorBase & base_exc,
  const std::string & prefix)
: exceptions::RCLErrorBase(base_exc),
  std::runtime_error(prefix + (prefix.empty() ? "" : ": ") + base_exc.formatted_message)
{}

EventHandlerBase::~EventHandlerBase()
{
  // Since the rmw event listener holds a reference to
  // this callback, we need to clear it on destruction of this class.
  // This clearing is not needed for other rclcpp entities like pub/subs, since
  // they do own the underlying rmw entities, which are destroyed
  // on their rclcpp destructors, thus no risk of dangling pointers.
  clear_on_ready_callback();

  if (rcl_event_fini(&event_handle_) != RCL_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(
      "rclcpp",
      "Error in destruction of rcl event handle: %s", rcl_get_error_string().str);
    rcl_reset_error();
  }
}

/// Get the number of ready events.
size_t
EventHandlerBase::get_number_of_ready_events()
{
  return 1;
}

/// Add the Waitable to a wait set.
void
EventHandlerBase::add_to_wait_set(rcl_wait_set_t * wait_set)
{
  rcl_ret_t ret = rcl_wait_set_add_event(wait_set, &event_handle_, &wait_set_event_index_);
  if (RCL_RET_OK != ret) {
    exceptions::throw_from_rcl_error(ret, "Couldn't add event to wait set");
  }
}

/// Check if the Waitable is ready.
bool
EventHandlerBase::is_ready(rcl_wait_set_t * wait_set)
{
  return wait_set->events[wait_set_event_index_] == &event_handle_;
}

void
EventHandlerBase::set_on_new_event_callback(
  rcl_event_callback_t callback,
  const void * user_data)
{
  rcl_ret_t ret = rcl_event_set_callback(
    &event_handle_,
    callback,
    user_data);

  if (RCL_RET_OK != ret) {
    using rclcpp::exceptions::throw_from_rcl_error;
    throw_from_rcl_error(ret, "failed to set the on new message callback for Event");
  }
}

}  // namespace rclcpp

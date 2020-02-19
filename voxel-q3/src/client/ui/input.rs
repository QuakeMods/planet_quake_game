/*
    Copyright 2013 Jesse 'Jeaye' Wilkerson
    See licensing in LICENSE file, or at:
        http://www.opensource.org/licenses/BSD-3-Clause

    File: client/ui/input.rs
    Author: Jesse 'Jeaye' Wilkerson
    Description:
      The stack-based input state.
*/

use glfw;

pub trait Input_Listener
{
  /*  Returns true when the event has been captured. If the event is not
      captured, it's set to the next lower state. Rinse and repeat. */
  fn key_action(&mut self, key: glfw::Key, action: glfw::Action, mods: glfw::Modifiers) -> bool;
  fn key_char(&mut self, ch: char) -> bool;
  fn mouse_action(&mut self, button: i32, action: i32, mods: i32) -> bool;
  fn mouse_moved(&mut self, x: f32, y: f32) -> bool;
}


/*
    Copyright 2013 Jesse 'Jeaye' Wilkerson
    See licensing in LICENSE file, or at:
        http://www.opensource.org/licenses/BSD-3-Clause

    File: shared/console/console.rs
    Author: Jesse 'Jeaye' Wilkerson
    Description:
      Manages the model portion
      of the in-game console.
*/

use std::local_data;
use std::hashmap::HashMap;
use log::Log;

#[macro_escape]
#[path = "../log/macros.rs"]
mod macros;

/* Takes: property name; Returns: property value. */
pub trait Accessor
{
  fn access(&self, name: &str) -> ~str;
}
/* Takes: property name, new value; Returns: Some(failure message). */
pub trait Mutator
{
  fn mutate(&mut self, name: &str, val: &str) -> Option<~str>;
}
/* Takes: function name, params; Returns: sucess/failure, message. */
pub trait Functor
{
  fn call(&mut self, name: &str, params: &str) -> (bool, ~str);
}

static tls_key: local_data::Key<@mut Console> = &local_data::Key;

struct Registry
{
  /*  
      Maps of property names to callbacks (get and set).
      Properties are invoked with a prebuilt 'set' or 'get'
      function. 
      
      The only argument to the accessor is the property name
      and it returns the value.

      The first argument of the mutator is the
      property name and the second is everything else 
      contained in the command. The mutator returns a string
      containing an error message if something didn't work well.

      Ex: set map.wireframe on
      Ex: get map.wireframe
  */
  accessors: HashMap<~str, @Accessor>,
  mutators: HashMap<~str, @mut Mutator>,

  /*
      A map of arbitrary functions to callbacks.

      The first argument is the function name and the second
      is whatever else is supplied in the command. The return
      value is a string containing an error message if something
      didn't go well.

      Ex: record my.avi
      Ex: callvote kick annoying_dude
  */
  functions: HashMap<~str, @mut Functor>,
}

pub struct Console
{
  body: ~str,
  prefix: ~str,
  input: ~str,

  registry: Registry,
}

impl Console
{
  pub fn new() -> @mut Console
  {
    let c = @mut Console
    {
      body: format!("Welcome to Q³\nVersion: {}.{}", env!("VERSION"), env!("COMMIT")),
      prefix: ~"> ",
      input: ~"", 

      registry: Registry
      {
        accessors: HashMap::<~str, @Accessor>::new(),
        mutators: HashMap::<~str, @mut Mutator>::new(),
        functions: HashMap::<~str, @mut Functor>::new(),
      },
    };

    /* Store the console in task-local storage. (singleton) */
    local_data::set(tls_key, c);

    /* The 'get' and 'set' functions are built in to the console. */
    c.registry.functions.insert(~"get", c as @mut Functor);
    c.registry.functions.insert(~"set", c as @mut Functor);

    /* Default properties. */
    c.add_accessor("q3.version", c as @Accessor);

    c
  }

  /* Accesses the singleton console from task-local storage. */
  pub fn get() -> @mut Console
  {
    local_data::get(tls_key, 
    |opt|
    {
      match opt
      {
        Some(x) => *x,
        None => log_fail!("Singleton not available")
      }
    })
  }

  pub fn add_function(&mut self, name: ~str, func: @mut Functor)
  {
    log_debug!("Adding function: {}", name);
    self.registry.functions.insert(name, func);
  }
  pub fn add_accessor(&mut self, name: &str, accessor: @Accessor)
  {
    log_debug!("Adding read access to property: {}", name);
    self.registry.accessors.insert(name.to_owned(), accessor);
  }
  pub fn add_mutator(&mut self, name: &str, mutator: @mut Mutator)
  {
    log_debug!("Adding write access to property: {}", name);
    self.registry.mutators.insert(name.to_owned(), mutator);
  }
  pub fn add_log(&mut self, text: &str)
  { self.body.push_str("\n" + text); }
  pub fn add_error_log(&mut self, text: &str)
  { self.body.push_str("\n\\2Error: \\1" + text); }

  pub fn run_function(mut input_func: ~str) -> (bool, ~str)
  {
    /* Extract the function name. */
    let mut func = ~"";
    input_func = input_func.trim().to_owned();
    for x in input_func.split_iter(' ')
    { func = x.to_owned(); break; };

    /* Remove the function from the string. */
    for _ in func.iter()
    { input_func.shift_char(); }
    if input_func.len() > 0
    { input_func.shift_char(); }

    /* Look for the function in the cached map. */
    let fun = Console::get().registry.functions.find(&func);
    match fun
    {
      Some(ref f) =>
      {
        let input = input_func.clone();
        (*f).call(func, input)
      }
      None => { (false, format!("\\\\2Error: \\\\1Invalid function '{}'", func)) }
    }
  }
}

impl Functor for Console
{
  fn call(&mut self, name: &str, params: &str) -> (bool, ~str)
  {
    match name
    {
      "get" =>
      {
        let mut msg;
        let mut success = false;

        /* Check if this property exists. */
        match self.registry.accessors.find(&params.to_owned())
        {
          Some(func) =>
          { msg = format!("{} = {}", params, (*func).access(params)); success = true; }
          None =>
          { msg = format!("\\\\2Error: \\\\1Invalid property '{}'", params); }
        }

        (success, msg)
      }
      "set" =>
      {
        let mut msg;
        let mut success = false;
        let mut property = ~"";
        let mut params = params.to_owned();
        for x in params.split_iter(' ')
        { property = x.to_owned(); break; }

        /* We require a property and a value.
         * Remove the property from the string.
         */
        for _ in property.iter()
        { params.shift_char(); }
        if params.len() > 0
        { params.shift_char(); }

        /* Check if this property exists. */
        match self.registry.mutators.find(&property)
        {
          Some(func) =>
          {
            /* Pass the args to the property mutator. */
            match (*func).mutate(property, params)
            {
              /* Check if the mutator liked the args. */
              Some(err) => { success = false; msg = ~"\\\\2Error: \\\\1" + err; }
              None => { success = true; msg = format!("\\\\1{} = {}", property, params); }
            }
          }
          None => { msg = format!("\\\\2Error: \\\\1The property '{}' does not exist.", property); }
        }

        (success, msg)
      }
      _ => { (false, ~"\\\\2Error: \\\\1Invalid function") }
    }
  }
}

impl Accessor for Console
{
  fn access(&self, name: &str) -> ~str
  {
    match name
    {
      "q3.version" =>
      { format!("{}.{}", env!("VERSION"), env!("COMMIT")) },

      _ => ~"ERROR",
    }
  }
}


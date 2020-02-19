/*
    Copyright 2013 Jesse 'Jeaye' Wilkerson
    See licensing in LICENSE file, or at:
        http://www.opensource.org/licenses/BSD-3-Clause

    File: shared/console/mod.rs
    Author: Jesse 'Jeaye' Wilkerson
    Description:
      An aggregator of model-side console items.
*/

#[link(name = "console", vers = "0.2")];
#[crate_type = "lib"];

#[feature(globs)];
#[feature(macro_rules)];
#[feature(managed_boxes)];

extern mod log;

pub use self::console::*;
pub use self::util::Util;

pub mod console;
pub mod util;


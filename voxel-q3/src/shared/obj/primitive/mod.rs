/*
    Copyright 2013 Jesse 'Jeaye' Wilkerson
    See licensing in LICENSE file, or at:
        http://www.opensource.org/licenses/BSD-3-Clause

    File: shared/obj/primitive/mod.rs
    Author: Jesse 'Jeaye' Wilkerson
    Description:
      An aggregator of primitive geometric items.
*/

pub use self::vertex::{ Vertex_P, Vertex_PC, Vertex_PN, Vertex_PCN };
pub use self::triangle::{ Triangle, Triangle_Index };

pub mod vertex;
pub mod triangle;


/*
    Copyright 2013 Jesse 'Jeaye' Wilkerson
    See licensing in LICENSE file, or at:
        http://www.opensource.org/licenses/BSD-3-Clause

    File: client/state/game/game.rs
    Author: Jesse 'Jeaye' Wilkerson
    Description:
      The main game state, shared between
      client and server; it loads and
      voxelizes a BSP map, handles
      players, and manages the game from
      a user's perspective. No rendering
      is included.
*/

use extra;
use obj;
use super::State;
use log::Log;

#[macro_escape]
#[path = "../../../shared/log/macros.rs"]
mod macros;

pub struct Game
{
  bsp_map: obj::BSP_Map,
  voxel_map: @mut obj::Voxel_Map,
}

impl Game
{
  pub fn new(map_name: &str) -> Result<@mut Game, ~str>
  {
    let bmap = obj::BSP_Map::new(~"data/maps/" + map_name + ".bsp");
    if bmap.is_err()
    { return Err(bmap.unwrap_err()); }
    let bmap = bmap.unwrap();

    let start_time = extra::time::precise_time_s();
    let vmap = obj::Voxel_Map::new(bmap.tris, 300);
    let time = extra::time::precise_time_s() - start_time;
    log_info!("Voxelization took {} seconds", time);
    if vmap.is_err()
    { return Err(vmap.unwrap_err()); }
    let vmap = vmap.unwrap();

    let game = @mut Game
    {
      voxel_map: vmap,
      bsp_map: bmap,
    };

    Ok(game)
  }
}

impl State for Game
{
  fn load(&mut self)
  { log_debug!("Loading game state."); }
  fn unload(&mut self)
  { log_debug!("Unloading game state."); }

  fn get_key(&self) -> &str
  { &"game" }

  fn update(&mut self, _delta: f32) -> bool /* dt is in terms of seconds. */
  { false }
  fn render(&mut self) -> bool
  { false }
}


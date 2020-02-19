/*
    Copyright 2013 Jesse 'Jeaye' Wilkerson
    See licensing in LICENSE file, or at:
        http://www.opensource.org/licenses/BSD-3-Clause

    File: shared/obj/voxel/map.rs
    Author: Jesse 'Jeaye' Wilkerson
    Description:
      A voxelization of arbitrary triangles
      into OpenGL-ready cubes.
*/

use std::{ vec, cmp };
use extra;
use math;
use primitive::Triangle;
use super::{ Vertex, Visible };
use log::Log;

#[macro_escape]
#[path = "../../log/macros.rs"]
mod macros;

pub struct Map
{
  resolution: u32,
  voxel_size: f32,

  states: Option<~[u32]>,
  voxels: ~[Vertex],
  error: ~str,
}

impl Map
{
  pub fn new(tris: &[Triangle], res: u32) -> Result<@mut Map, ~str>
  {
    let map = @mut Map
    {
      resolution: res,
      voxel_size: 0.0,

      states: None,
      voxels: ~[],
      error: ~"",
    };

    if !map.voxelize(tris)
    { return Err(map.error.clone()); }

    Ok(map)
  }

  fn voxelize(&mut self, tris: &[Triangle]) -> bool
  {
    /* Require at least one triangle. */
    if !(tris.len() >= 1)
    { self.error = ~"Invalid triangle count"; return false; }

    log_info!("Voxelizing in {}x{}x{} grid",
              self.resolution,
              self.resolution,
              self.resolution);
    log_debug!("Incoming triangles: {}", tris.len());

    /* Bounding box of vert dimensions. */
    let mut min = math::Vec3f::new( tris[0].verts[0].position.x,
                                    tris[0].verts[0].position.y, 
                                    tris[0].verts[0].position.z);
    let mut max = math::Vec3f::new( tris[0].verts[0].position.x,
                                    tris[0].verts[0].position.y,
                                    tris[0].verts[0].position.z);
    for curr in tris.iter()
    {
      for vert in curr.verts.iter()
      {
        min.x = cmp::min(min.x, vert.position.x);
        min.y = cmp::min(min.y, vert.position.y);
        min.z = cmp::min(min.z, vert.position.z);

        max.x = cmp::max(max.x, vert.position.x);
        max.y = cmp::max(max.y, vert.position.y);
        max.z = cmp::max(max.z, vert.position.z);
      }
    }
    log_debug!("Min: {} Max: {}", min.to_str(), max.to_str());
    let center = math::Vec3f::new(max.x - ((max.x - min.x) / 2.0),
                                  max.y - ((max.y - min.y) / 2.0),
                                  max.z - ((max.z - min.z) / 2.0));
    log_debug!("Center of mesh is {}", center.to_str());

    /* Calculate, given resolution (how many states across), the dimensions of a voxel. */
    self.voxel_size = cmp::max( max.x - min.x,
                                cmp::max(max.y - min.y, max.z - min.z)) / (self.resolution as f32);
    log_debug!("Voxel size is {}", self.voxel_size);

    /* World space mid point of the grid. */
    let mid_offset = (((self.resolution as f32) / 2.0) * self.voxel_size); 
    log_debug!("Midpoint offset is {}", mid_offset);

    /* Create 3D array of states. */
    self.states = Some(vec::with_capacity(((self.resolution + 1) as f32).pow(&3.0) as uint));
    self.voxels = vec::with_capacity(self.states.get_mut_ref().len() / 2); /* Half is just a (generous) guess. */
    for _z in range(0, self.resolution as uint) 
    { for _y in range(0, self.resolution as uint)
      { for _x in range(0, self.resolution as uint)
        {
          self.states.get_mut_ref().push(0); /* Invisible. */
        }
      }
    }

    /* Voxels are temporarily stored in a set, which will prevent the
     * voxelization algorithm from adding any duplicated. We trade a bit
     * of CPU to save some memory here. */
    let mut voxels = extra::treemap::TreeSet::new();
    for tri in tris.iter()
    {
      /* Calculate bounding box of the triangle. */
      min = math::Vec3f::new(tri.verts[0].position.x, tri.verts[0].position.y, tri.verts[0].position.z);
      max = math::Vec3f::new(tri.verts[0].position.x, tri.verts[0].position.y, tri.verts[0].position.z);
      for vert in tri.verts.iter()
      {
        /* Adjust by half of a voxel to account for voxel centering. */
        min.x = cmp::min(min.x, vert.position.x - (self.voxel_size / 2.0));
        min.y = cmp::min(min.y, vert.position.y - (self.voxel_size / 2.0));
        min.z = cmp::min(min.z, vert.position.z - (self.voxel_size / 2.0));

        max.x = cmp::max(max.x, vert.position.x - (self.voxel_size / 2.0));
        max.y = cmp::max(max.y, vert.position.y - (self.voxel_size / 2.0));
        max.z = cmp::max(max.z, vert.position.z - (self.voxel_size / 2.0));
      }

      /* The dimensions (in voxels) of the triangle's bounding box. */
      let mut vox_amount = math::Vec3i::new((((max.x - min.x) / self.voxel_size)).ceil() as i32,
                                            (((max.y - min.y) / self.voxel_size)).ceil() as i32,
                                            (((max.z - min.z) / self.voxel_size)).ceil() as i32);
      if vox_amount.x < 1
      { vox_amount.x = 1; }
      if vox_amount.y < 1
      { vox_amount.y = 1; }
      if vox_amount.z < 1
      { vox_amount.z = 1; }
      //log_debug!("[Per voxel] Checking {} surrounding states with SAT", vox_amount.to_str());

      /* Get the starting indices of the triangle's bounding box. */
      let start_voxels = math::Vec3i::new( ((min.x - -mid_offset) / self.voxel_size) as i32, 
                                      ((min.y - -mid_offset) / self.voxel_size) as i32,
                                      ((min.z - -mid_offset) / self.voxel_size) as i32);

      /* Test intersection with each accepted voxel. */
      for z in range(start_voxels.z, start_voxels.z + vox_amount.z)
      { for y in range(start_voxels.y, start_voxels.y + vox_amount.y)
        { for x in range(start_voxels.x, start_voxels.x + vox_amount.x)
          {
            /* Check for intersection. */
            let c = math::Vec3f::new( ((x as f32 - (self.resolution as f32 / 2.0)) * self.voxel_size) + (self.voxel_size / 2.0), 
                                ((y as f32 - (self.resolution as f32 / 2.0)) * self.voxel_size) + (self.voxel_size / 2.0),
                                ((z as f32 - (self.resolution as f32 / 2.0)) * self.voxel_size) + (self.voxel_size / 2.0));
            if tri_cube_intersect(c, self.voxel_size, tri)
            {
              /* Calculate the average color from all three verts. */
              let av_color = math::Vec3f::new
              (
                ((tri.verts[0].color.x + tri.verts[1].color.x + tri.verts[2].color.x) / 3.0) as f32 / 255.0,
                ((tri.verts[0].color.y + tri.verts[1].color.y + tri.verts[2].color.y) / 3.0) as f32 / 255.0,
                ((tri.verts[0].color.z + tri.verts[1].color.z + tri.verts[2].color.z) / 3.0) as f32 / 255.0
              );

              /* Enable some debug rendering of invalid voxels. */
              let col = if x >= self.resolution as i32 || y >= self.resolution as i32 || z >= self.resolution as i32
              { math::Vec3f::new(1.0, 0.0, 0.0) }
              else if x < 0 || y < 0 || z < 0
              { math::Vec3f::new(1.0, 0.0, 0.0) }
              else
              { av_color };

              /* We have intersection; add a reference to this voxel to the index map. */
              voxels.insert(
              Vertex
              {
                position: math::Vec3f::new( x as f32 - (self.resolution / 2) as f32, 
                                            y as f32 - (self.resolution / 2) as f32,
                                            z as f32 - (self.resolution / 2) as f32), 
                color: col
              });
            }
          }
        }
      }
    }

    /* Now that all of the voxels have been put in the set, we
     * can pull them out and store them in a contiguous structure.
     * Otherwise, giving the voxels to OpenGL would be difficult. */
    for vox in voxels.iter()
    {
      /* Determine the voxel-space position. */
      let x = (vox.position.x + (self.resolution / 2) as f32) as i32;
      let y = (vox.position.y + (self.resolution / 2) as f32) as i32;
      let z = (vox.position.z + (self.resolution / 2) as f32) as i32;
      log_assert!(x >= 0 && y >= 0 && z >= 0);
      log_assert!(x < self.resolution as i32 &&
                  y < self.resolution as i32 &&
                  z < self.resolution as i32);

      log_assert!(self.states.is_some());
      let states = self.states.get_mut_ref();

      /* Calculate the index of this voxel in the state grid. */
      let index = (z * ((self.resolution * self.resolution) as i32)) + (y * (self.resolution as i32)) + x;
      log_assert!(index < states.len() as i32);

      /* Update the state grid with this voxel's data. */
      states[index] = self.voxels.len() as u32;
      states[index] |= Visible;

      /* Move this voxel into contiguous memory. */
      self.voxels.push(*vox);
    }

    log_debug!("Enabled {} of {} voxels", self.voxels.len(), self.states.get_mut_ref().len());

    true
  }
}

fn tri_cube_intersect(box_center: math::Vec3f, box_size: f32, tri: &Triangle) -> bool
{
  let _v0;
  let _v1;
  let _v2;
  let mut _min;
  let mut _max;
  let mut _p0 = 0.0;
  let mut _p1 = 0.0;
  let mut _p2 = 0.0;
  let mut _rad;
  let mut _fex;
  let mut _fey;
  let mut _fez;
  let _normal;
  let _e0;
  let _e1;
  let _e2;

  macro_rules! find_min_max
  (
    ($x0:expr, $x1:expr, $x2:expr) =>
    (
      {
        _min = $x0;
        _max = $x0;

        if($x1 < _min){ _min = $x1; }
        if($x1 > _max){ _max = $x1; }
        if($x2 < _min){ _min = $x2; }
        if($x2 > _max){ _max = $x2; }
      }
    )
  )

  /*======================== X-tests ========================*/
  macro_rules! axis_test_x01
  (
    ($a:expr, $b:expr, $fa:expr, $fb:expr) =>
    (
      {
        _p0 = $a * _v0.y - $b * _v0.z;
        _p2 = $a * _v2.y - $b * _v2.z;
        if _p0 < _p2  { _min = _p0; _max = _p2; } else { _min = _p2; _max = _p0; }
        _rad = $fa * box_size + $fb * box_size;
        if _min > _rad || _max < -_rad  { return false; }
      }
    )
  )

  macro_rules! axis_test_x2
  (
    ($a:expr, $b:expr, $fa:expr, $fb:expr) =>
    (
      {
        _p0 = $a * _v0.y - $b * _v0.z;
        _p1 = $a * _v1.y - $b * _v1.z;
        if _p0 < _p1 { _min = _p0; _max = _p1; } else { _min = _p1; _max = _p0; }
        _rad = $fa * box_size + $fb * box_size;
        if _min > _rad || _max < -_rad { return false; }
      }
    )
  )

  /*======================== Y-tests ========================*/

  macro_rules! axis_test_y02
  (
    ($a:expr, $b:expr, $fa:expr, $fb:expr) =>
    (
      {
        _p0 = -$a * _v0.x + $b * _v0.z;
        _p2 = -$a * _v2.x + $b * _v2.z;
        if _p0 < _p2 { _min = _p0; _max = _p2; } else { _min = _p2; _max = _p0; }
        _rad = $fa * box_size + $fb * box_size;
        if _min > _rad || _max < -_rad { return false; }
      }
    )
  )

  macro_rules! axis_test_y1
  (
    ($a:expr, $b:expr, $fa:expr, $fb:expr) =>
    (
      {
        _p0 = -$a * _v0.x + $b * _v0.z;
        _p1 = -$a * _v1.x + $b * _v1.z;
        if _p0 < _p1 { _min = _p0; _max = _p1; } else { _min = _p1; _max = _p0; }
        _rad = $fa * box_size + $fb * box_size;
        if _min > _rad || _max < -_rad { return false; }
      }
    )
  )

  /*======================== Z-tests ========================*/

  macro_rules! axis_test_z12
  (
    ($a:expr, $b:expr, $fa:expr, $fb:expr) =>
    (
      {
        _p1 = $a * _v1.x - $b * _v1.y;
        _p2 = $a * _v2.x - $b * _v2.y;
        if _p2 < _p1 { _min = _p2; _max = _p1;} else { _min = _p1; _max = _p2; }
        _rad = $fa * box_size + $fb * box_size;
        if _min > _rad || _max < -_rad { return false; }
      }
    )
  )


  macro_rules! axis_test_z0
  (
    ($a:expr, $b:expr, $fa:expr, $fb:expr) =>
    (
      {
        _p0 = $a * _v0.x - $b * _v0.y;
        _p1 = $a * _v1.x - $b * _v1.y;
        if _p0 < _p1 { _min = _p0; _max = _p1; } else { _min = _p1; _max = _p0; }
        _rad = $fa * box_size + $fb * box_size;
        if _min > _rad || _max < -_rad { return false; }
      }
    )
  )

  /* Move everything so that the box's center is in (0, 0, 0). */
  _v0 = tri.verts[0].position - box_center;
  _v1 = tri.verts[1].position - box_center;
  _v2 = tri.verts[2].position - box_center;

  /* Computer triangle edges. */
  _e0 = _v1 - _v0; /* Edge 0. */
  _e1 = _v2 - _v1; /* Edge 1. */
  _e2 = _v0 - _v2; /* Edge 2. */

  //log_debug!("[Per voxel SAT] Testing bullet 3 edge 0");
  /* Bullet 3. */
  _fex = _e0.x.abs();
  _fey = _e0.y.abs();
  _fez = _e0.z.abs();
  axis_test_x01!(_e0.z, _e0.y, _fez, _fey);
  axis_test_y02!(_e0.z, _e0.x, _fez, _fex);
  axis_test_z12!(_e0.y, _e0.x, _fey, _fex);

  //log_debug!("[Per voxel SAT] Testing bullet 3 edge 1");
  _fex = _e1.x.abs();
  _fey = _e1.y.abs();
  _fez = _e1.z.abs();
  axis_test_x01!(_e1.z, _e1.y, _fez, _fey);
  axis_test_y02!(_e1.z, _e1.x, _fez, _fex);
  axis_test_z0!(_e1.y, _e1.x, _fey, _fex);

  //log_debug!("[Per voxel SAT] Testing bullet 3 edge 2");
  _fex = _e2.x.abs();
  _fey = _e2.y.abs();
  _fez = _e2.z.abs();
  axis_test_x2!(_e2.z, _e2.y, _fez, _fey);
  axis_test_y1!(_e2.z, _e2.x, _fez, _fex);
  axis_test_z12!(_e2.y, _e2.x, _fey, _fex);

  //log_debug!("[Per voxel SAT] Testing bullet 1");
  /* Bullet 1. */
  /* Test in X-direction */
  find_min_max!(_v0.x, _v1.x, _v2.x);
  if _min > box_size || _max < -box_size { return false; }

  /* Test in Y-direction */
  find_min_max!(_v0.y, _v1.y, _v2.y);
  if _min > box_size || _max < -box_size { return false; }

  /* Test in Z-direction */
  find_min_max!(_v0.z, _v1.z, _v2.z);
  if _min > box_size || _max < -box_size { return false; }

  //log_debug!("[Per voxel SAT] Testing bullet 2");
  /* Bullet 2. */
  _normal = _e0.cross(&_e1);
  plane_cube_intersect(&_normal, &_v0, box_size)
}

fn plane_cube_intersect(normal: &math::Vec3f, vert: &math::Vec3f, box_size: f32) -> bool
{
  let mut vmin: [f32, ..3] = [0.0, 0.0, 0.0];
  let mut vmax: [f32, ..3] = [0.0, 0.0, 0.0];
  let mut v;

  for q in range(0u, 3u)
  {
    v = vert[q];
    if normal[q] > 0.0
    {
      vmin[q] = -box_size - v;
      vmax[q] = box_size - v;
    }
    else
    {
      vmin[q] = box_size - v;
      vmax[q] = -box_size - v;
    }
  }
  if normal[0] * vmin[0] + normal[1] * vmin[1] + normal[2] * vmin[2] > 0.0
  { return false; }
  if normal[0] * vmax[0] + normal[1] * vmax[1] + normal[2] * vmax[2] >= 0.0
  { return true; }

  false
}


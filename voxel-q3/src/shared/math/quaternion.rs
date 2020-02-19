/*
    Copyright 2013 Jesse 'Jeaye' Wilkerson
    See licensing in LICENSE file, or at:
        http://www.opensource.org/licenses/BSD-3-Clause

    File: shared/math/quaternion.rs
    Author: Jesse 'Jeaye' Wilkerson
    Description:
      Quaternions are magic mathematical objects
      which can represent an orientation.
*/

use std::{ f32, cmp };

type Component = f32;

#[deriving(Clone)]
pub struct Quaternion
{
  x: Component,
  y: Component,
  z: Component,
  w: Component,
}

impl Quaternion
{
  pub fn new(x: Component, y: Component, z: Component, w: Component) -> Quaternion
  { Quaternion { x: x, y: y, z: z, w: w } }

  pub fn new_from_vec(vec: &super::Vec3f) -> Quaternion
  { Quaternion { x: vec.x, y: vec.y, z: vec.z, w: 0.0 } }

  pub fn zero() -> Quaternion
  { Quaternion { x: 0.0, y: 0.0, z: 0.0, w: 0.0 } }

  pub fn new_from_axis(axis: &super::Vec3f, angle: f32) -> Quaternion
  {
    let sin_angle = angle.sin();
    let norm_axis = super::Vec3f::new_normalized(axis);

    Quaternion {  x: (norm_axis.x * sin_angle),
                  y: (norm_axis.y * sin_angle),
                  z: (norm_axis.z * sin_angle),
                  w: angle.cos() }
  }

  pub fn new_from_euler(yaw: f32, pitch: f32, roll: f32) -> Quaternion
  {
    let p = pitch * (f32::consts::PI / 180.0) / 2.0;
    let y = yaw * (f32::consts::PI / 180.0) / 2.0;
    let r = roll * (f32::consts::PI / 180.0) / 2.0;

    let sinp = p.sin();
    let siny = y.sin();
    let sinr = r.sin();
    let cosp = p.cos();
    let cosy = y.cos();
    let cosr = r.cos();

    let mut q = Quaternion::new
    (
      sinr * cosp * cosy - cosr * sinp * siny,
      cosr * sinp * cosy + sinr * cosp * siny,
      cosr * cosp * siny - sinr * sinp * cosy,
      cosr * cosp * cosy + sinr * sinp * siny
    );
    q.normalize();

    q
  }

  pub fn new_slerp(lhs: &Quaternion, rhs: &Quaternion, interp: f32) -> Quaternion
  { lhs.slerp(rhs, interp) }

  pub fn get_conjugate(&self) -> Quaternion
  { Quaternion { x: -self.x, y: -self.y, z: -self.z, w: -self.w } }

  pub fn normalize(&mut self)
  {
    let mag = ((self.x * self.x) +
              (self.y * self.y) +
              (self.z * self.z) +
              (self.w * self.w)).sqrt();
    if mag > 0.0
    {
      let one_over = 1.0 / mag;
      self.x *= one_over;
      self.y *= one_over;
      self.z *= one_over;
      self.w *= one_over;
    }
  }

  pub fn dot(&self, rhs: &Quaternion) -> f32
  { (self.x * rhs.x) + (self.y * rhs.y) + (self.z * rhs.z) + (self.w * rhs.w) }

  pub fn slerp(&self, _rhs: &Quaternion, interp: f32) -> Quaternion
  {
    /* Return edge points for out-of-range interps. */
    if interp < 0.0
    { return *self; }
    else if interp >= 1.0
    { return *_rhs };

    let mut cos_omega = self.dot(_rhs);
    let scale = if cos_omega < 0.0
    { -1.0 } /* In the case of a negative dot, negate the quat. */
    else
    { 1.0 };
    cos_omega *= scale;

    let mut ret = *_rhs;
    ret.scale(scale);

    /* We should have to unit quaternions. */
    assert!(cos_omega < 1.1);

    let scale0;
    let scale1;

    if cos_omega > 0.9999
    {
      /* Very close - just use linear interpolation
       * which will protect against division by zero. */
      scale0 = 1.0 - interp;
      scale1 = interp;
    }
    else
    {
      let sin_omega = (1.0 - (cos_omega * cos_omega)).sqrt();
      let omega = sin_omega.atan2(&cos_omega);
      let one_over_sin_omega = 1.0 / sin_omega;

      scale0 = ((1.0 - interp) * omega).sin() * one_over_sin_omega;
      scale1 = (interp * omega).sin() * one_over_sin_omega;
    }

    Quaternion::new
    (
      (scale0 * self.x) + (scale1 * (_rhs.x * scale)),
      (scale0 * self.y) + (scale1 * (_rhs.y * scale)),
      (scale0 * self.z) + (scale1 * (_rhs.z * scale)),
      (scale0 * self.w) + (scale1 * (_rhs.w * scale))
    )
  }

  pub fn compute_w(&mut self)
  {
    let t = 1.0 - (self.x * self.x) - (self.y * self.y) - (self.z * self.z);

    if t < 0.0
    { self.w = 0.0; }
    else
    { self.w = -t.sqrt(); }
  }

  pub fn rotate_vec(&self, vec: &super::Vec3f) -> super::Vec3f
  {
    let mut inv = Quaternion::new(-self.x, -self.y, -self.z, self.w);
    inv.normalize();

    let tmp = self.mul_vec(vec);
    let final = tmp * inv;
    super::Vec3f::new(final.x, final.y, final.z)
  }

  pub fn mul_vec(&self, vec: &super::Vec3f) -> Quaternion
  {
    Quaternion::new
    (
       (self.w * vec.x) + (self.y * vec.z) - (self.z * vec.y),
       (self.w * vec.y) + (self.z * vec.x) - (self.x * vec.z),
       (self.w * vec.z) + (self.x * vec.y) - (self.y * vec.x),
      -(self.x * vec.x) - (self.y * vec.y) - (self.z * vec.z)
    )
  }

  pub fn scale(&mut self, scalar: Component)
  {
    self.x *= scalar;
    self.y *= scalar;
    self.z *= scalar;
    self.w *= scalar;
  }

  pub fn to_str(&self) -> ~str
  {
    format!("({}, {}, {}, {})",
          self.x,
          self.y,
          self.z,
          self.w)
  }

  pub fn to_vec(&self) -> super::Vec3f
  { super::Vec3f::new(self.x, self.y, self.z) }

  pub fn to_mat(&self) -> super::Mat4x4
  {
    let x2 = self.x * self.x;
    let y2 = self.y * self.y;
    let z2 = self.z * self.z;
    let xy = self.x * self.y;
    let xz = self.x * self.z;
    let yz = self.y * self.z;
    let wx = self.w * self.x;
    let wy = self.w * self.y;
    let wz = self.w * self.z;

    /* Magic. */
    super::Mat4x4
    {
      data:
      [
        [ 1.0 - 2.0 * (y2 + z2), 2.0 * (xy - wz), 2.0 * (xz + wy), 0.0 ],
        [ 2.0 * (xy + wz), 1.0 - 2.0 * (x2 + z2), 2.0 * (yz - wx), 0.0 ],
        [ 2.0 * (xz - wy), 2.0 * (yz + wx), 1.0 - 2.0 * (x2 + y2), 0.0 ],
        [ 0.0, 0.0, 0.0, 1.0 ]
      ]
    }
  }
}

impl Mul<Quaternion, Quaternion> for Quaternion
{
  fn mul(&self, rhs: &Quaternion) -> Quaternion
  {
    Quaternion::new
    (
      (self.x * rhs.w) + (self.w * rhs.x) + (self.y * rhs.z) - (self.z * rhs.y),
      (self.y * rhs.w) + (self.w * rhs.y) + (self.z * rhs.x) - (self.x * rhs.z),
      (self.z * rhs.w) + (self.w * rhs.z) + (self.x * rhs.y) - (self.y * rhs.x),
      (self.w * rhs.w) - (self.x * rhs.x) - (self.y * rhs.y) - (self.z * rhs.z)
    )
  }
}

impl cmp::Eq for Quaternion
{
  fn eq(&self, other: &Quaternion) -> bool
  {
    self.x.approx_eq(&other.x) && 
    self.y.approx_eq(&other.y) && 
    self.z.approx_eq(&other.z) &&
    self.w.approx_eq(&other.w)
  }

  fn ne(&self, other: &Quaternion) -> bool
  { !(self == other) }
}

impl cmp::TotalEq for Quaternion
{
  fn equals(&self, other: &Quaternion) -> bool
  { self == other }
}


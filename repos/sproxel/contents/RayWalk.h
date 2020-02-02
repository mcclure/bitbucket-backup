#ifndef __RAY_WALK_H__
#define __RAY_WALK_H__


#include <vector>
#include <ImathVec.h>
#include <ImathLine.h>
#include <ImathBox.h>


inline std::vector<Imath::V3i> walk_ray(const Imath::Line3d &ray, const Imath::Box3i &box)
{
  std::vector<Imath::V3i> list;

  if (box.isEmpty()) return list;

  // setup
  Imath::V3d p0=ray.pos;
  Imath::V3d dp=ray.dir;

  int stepX = (dp.x>=0) ? 1 : -1;
  int stepY = (dp.y>=0) ? 1 : -1;
  int stepZ = (dp.z>=0) ? 1 : -1;

  Imath::V3d tdelta(fabs(dp.x), fabs(dp.y), fabs(dp.z));
  tdelta.x = ((tdelta.x>tdelta.baseTypeEpsilon()) ? 1.0/tdelta.x : 1e30);
  tdelta.y = ((tdelta.y>tdelta.baseTypeEpsilon()) ? 1.0/tdelta.y : 1e30);
  tdelta.z = ((tdelta.z>tdelta.baseTypeEpsilon()) ? 1.0/tdelta.z : 1e30);
  double tdx=tdelta.x;
  double tdy=tdelta.y;
  double tdz=tdelta.z;

  // move to box if outside
  Imath::V3d p=p0;

  Imath::V3d bmin=Imath::V3d(box.min)-p, bmax=p-Imath::V3d(box.max+Imath::V3i(1));

  Imath::V3d tb;
  tb.x = ((dp.x>=0) ? bmin.x : bmax.x);
  tb.y = ((dp.y>=0) ? bmin.y : bmax.y);
  tb.z = ((dp.z>=0) ? bmin.z : bmax.z);
  tb*=tdelta;

  double t=0;
  if (tb.x>t) t=tb.x;
  if (tb.y>t) t=tb.y;
  if (tb.z>t) t=tb.z;

  p+=dp*t;

  // more setup
  int ix=int(floor(p.x));
  int iy=int(floor(p.y));
  int iz=int(floor(p.z));

  double fx=p.x-ix;
  double fy=p.y-iy;
  double fz=p.z-iz;

  int endX = (dp.x>=0) ? box.max.x+1 : box.min.x-1;
  int endY = (dp.y>=0) ? box.max.y+1 : box.min.y-1;
  int endZ = (dp.z>=0) ? box.max.z+1 : box.min.z-1;

  if ((dp.x>=0 && ix>=endX) || (dp.x<0 && ix<=endX)) return list;
  if ((dp.y>=0 && iy>=endY) || (dp.y<0 && iy<=endY)) return list;
  if ((dp.z>=0 && iz>=endZ) || (dp.z<0 && iz<=endZ)) return list;

  double tmx=((dp.x>=0) ? 1.0-fx : fx)*tdelta.x;
  double tmy=((dp.y>=0) ? 1.0-fy : fy)*tdelta.y;
  double tmz=((dp.z>=0) ? 1.0-fz : fz)*tdelta.z;

  // walk grid
  for (;;)
  {
    Imath::V3i ip(ix, iy, iz);
    if (box.intersects(ip)) list.push_back(ip);

    if (tmx<tmy)
    {
      if (tmx<tmz)
      {
        ix+=stepX;
        if (ix==endX) break;
        t=tmx;
        tmx+=tdx;
      }
      else
      {
        iz+=stepZ;
        if (iz==endZ) break;
        t=tmz;
        tmz+=tdz;
      }
    }
    else
    {
      if (tmy<tmz)
      {
        iy+=stepY;
        if (iy==endY) break;
        t=tmy;
        tmy+=tdy;
      }
      else
      {
        iz+=stepZ;
        if (iz==endZ) break;
        t=tmz;
        tmz+=tdz;
      }
    }
  }

  return list;
}


#endif

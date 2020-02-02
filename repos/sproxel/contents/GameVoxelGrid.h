#ifndef __GAME_VOXEL_GRID_H__
#define __GAME_VOXEL_GRID_H__

#include <vector>
#include <algorithm>

#include <ImathBox.h>
#include <ImathBoxAlgo.h>
#include <ImathVec.h>
#include <ImathLine.h>
#include <ImathMatrix.h>

#include "RayWalk.h"


//-*****************************************************************************
template <class T>
class GameVoxelGrid
{
public:
    GameVoxelGrid() : m_cellDimensions(0,0,0)
    {
        resizeData();
    }

    GameVoxelGrid(const Imath::V3i& cellDim)
        : m_cellDimensions(cellDim)
    {
        resizeData();
    }

    GameVoxelGrid(const GameVoxelGrid& gvg)
    {
        m_cellDimensions = gvg.m_cellDimensions;
        m_data = gvg.m_data;
    }
    ~GameVoxelGrid() { }

    // General accessors
    const Imath::V3i& cellDimensions() const { return m_cellDimensions; }
    void setCellDimensions(const Imath::V3i& cd) { m_cellDimensions = cd; resizeData(); }

    const Imath::M44d voxelTransform(const Imath::V3i& v) const
    {
        Imath::M44d vMat;
        vMat.setTranslation(voxelCenter(v));
        return vMat;
    }

    const Imath::Box3d worldBounds() const
    {
        const Imath::Box3d retBox(Imath::V3d(0,0,0),
                                  Imath::V3d(m_cellDimensions.x,
                                             m_cellDimensions.y,
                                             m_cellDimensions.z));
        // (ImathBoxAlgo) This properly computes the world bounding box
        return retBox;
    }

    void set(const Imath::V3i& cell, const T& value)
    {
        m_data[cell.x][cell.y][cell.z] = value;
    }

    void setAll(const T& value)
    {
        for (int x = 0; x < m_cellDimensions.x; x++)
            for (int y = 0; y < m_cellDimensions.y; y++)
                for (int z = 0; z < m_cellDimensions.z; z++)
                    m_data[x][y][z] = value;
    }

    T get(const Imath::V3i& cell)
    {
        return m_data[cell.x][cell.y][cell.z];
    }

    std::vector<Imath::V3i> rayIntersection(Imath::Line3d worldRay)
    {
        return walk_ray(worldRay, Imath::Box3i(Imath::V3i(0), m_cellDimensions-Imath::V3i(1)));
    }

    GameVoxelGrid& operator=(const GameVoxelGrid& other)
    {
        if (this != &other)
        {
            m_cellDimensions = other.cellDimensions();
            resizeData();

            for (int x = 0; x < m_cellDimensions.x; x++)
            {
                for (int y = 0; y < m_cellDimensions.y; y++)
                {
                    for (int z = 0; z < m_cellDimensions.z; z++)
                    {
                        m_data[x][y][z] = other.m_data[x][y][z];
                    }
                }
            }
        }
        return *this;
    }

    void resize(const Imath::V3i &size, const Imath::V3i &offset, const T &value)
    {
        GameVoxelGrid<T> newGrid(size);
        newGrid.setAll(value);

        int x0=offset.x; if (x0<0) x0=0;
        int y0=offset.y; if (y0<0) y0=0;
        int z0=offset.z; if (z0<0) z0=0;

        int x1=offset.x+m_cellDimensions.x; if (x1>size.x) x1=size.x;
        int y1=offset.y+m_cellDimensions.y; if (y1>size.y) y1=size.y;
        int z1=offset.z+m_cellDimensions.z; if (z1>size.z) z1=size.z;

        for (int x=x0; x<x1; ++x)
          for (int y=y0; y<y1; ++y)
            for (int z=z0; z<z1; ++z)
              newGrid.set(Imath::V3i(x, y, z), get(Imath::V3i(x, y, z)-offset));

        *this=newGrid;
    }


private:
    Imath::V3i m_cellDimensions;
    std::vector< std::vector < std::vector< T > > > m_data;

    Imath::V3d voxelCenter(const Imath::V3i& v) const
    {
        return Imath::V3d((double)v.x + 0.5,
                          (double)v.y + 0.5,
                          (double)v.z + 0.5);
    }

    void resizeData()
    {
        // Does not lose existing data
        m_data.resize(m_cellDimensions.x);
        for (int x = 0; x < m_cellDimensions.x; x++)
        {
            m_data[x].resize(m_cellDimensions.y);
            for (int y = 0; y < m_cellDimensions.y; y++)
            {
                m_data[x][y].resize(m_cellDimensions.z);
            }
        }
    }
};

#endif

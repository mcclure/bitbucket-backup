#ifndef __SPROXEL_PROJECT_H__
#define __SPROXEL_PROJECT_H__


#include "VoxelGridGroup.h"


class SproxelProject : public QSharedData
{
public:
  QVector<VoxelGridGroupPtr> sprites;
  QVector<ColorPalettePtr> palettes;
  ColorPalettePtr mainPalette;


  SproxelProject()
  {
    mainPalette=new ColorPalette();
    mainPalette->resize(256);
    mainPalette->setName("main");
    for (int i=0; i<64; ++i)
    {
      mainPalette->setColor(i+64*0, SproxelColor(i/63.0f, i/63.0f, i/63.0f, 1.0f));
      mainPalette->setColor(i+64*1, SproxelColor(i/63.0f,    0.0f,    0.0f, 1.0f));
      mainPalette->setColor(i+64*2, SproxelColor(   0.0f, i/63.0f,    0.0f, 1.0f));
      mainPalette->setColor(i+64*3, SproxelColor(   0.0f,    0.0f, i/63.0f, 1.0f));
    }
    mainPalette->setColor(0, SproxelColor(0, 0, 0, 0));
    palettes.push_back(mainPalette);
  }
};


typedef QExplicitlySharedDataPointer<SproxelProject> SproxelProjectPtr;


bool save_project(QString filename, SproxelProjectPtr project);
SproxelProjectPtr load_project(QString filename);


#endif

#include <map>
#include <QImage>
#include <QColor>
#include <QFileInfo>
#include "ImportExport.h"


static QList<Importer*> importers;
static QList<Exporter*> exporters;


void   register_importer(Importer *p) { if (p && !importers.contains(p)) importers.append(p); }
void unregister_importer(Importer *p) { int i=importers.indexOf(p); if (i>=0) importers.removeAt(i); }

const QList<Importer*>& get_importers() { return importers; }


void   register_exporter(Exporter *p) { if (p && !exporters.contains(p)) exporters.append(p); }
void unregister_exporter(Exporter *p) { int i=exporters.indexOf(p); if (i>=0) exporters.removeAt(i); }

const QList<Exporter*>& get_exporters() { return exporters; }


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ImageImporter : public Importer
{
public:
  virtual QString name() { return "Image files"; }
  virtual QString filter() { return "*.bmp *.gif *.jpg *.jpeg *.png *.tiff *.tif"; }

  virtual bool doImport(const QString &filename, UndoManager *um,
    SproxelProjectPtr project, VoxelGridGroupPtr)
  {
    QImage imported;
    if (!imported.load(filename)) return false;
    imported = imported.mirrored();

    // For now we always import into the Z axis and resize if we need to
    const int imageSizeX = imported.width();
    const int imageSizeY = imported.height();

    VoxelGridGroupPtr spr(new VoxelGridGroup(Imath::V3i(imageSizeX, imageSizeY, 1), ColorPalettePtr()));

    // Splat the data in
    for (int x = 0; x < imageSizeX; x++)
    {
      for (int y = 0; y < imageSizeY; y++)
      {
        const Imath::V3i locale(x, y, 0);
        const QColor qcolor = imported.pixel(x, y);
        const Imath::Color4f color(qcolor.redF(),
                                   qcolor.greenF(),
                                   qcolor.blueF(),
                                   qcolor.alphaF());
        spr->set(locale, color);
      }
    }

    spr->setName(QFileInfo(filename).baseName());
    um->addSprite(project, -1, spr);

    return true;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class SproxelPngImporter : public Importer
{
public:
  virtual QString name() { return "Sproxel PNG files"; }
  virtual QString filter() { return "*.png"; }

  virtual bool doImport(const QString &filename, UndoManager *um,
    SproxelProjectPtr project, VoxelGridGroupPtr)
  {
    QImage readMe;
    if (!readMe.load(filename, "PNG")) return false;

    VoxelGridLayerPtr layer=VoxelGridLayer::fromQImage(readMe, project->mainPalette);
    if (!layer) return false;

    VoxelGridGroupPtr spr(new VoxelGridGroup(layer));
    spr->setName(QFileInfo(filename).baseName());

    um->addSprite(project, -1, spr);

    return true;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class SproxelCsvImporter : public Importer
{
public:
  virtual QString name() { return "Sproxel CSV files"; }
  virtual QString filter() { return "*.csv"; }

  virtual bool doImport(const QString &filename, UndoManager *um,
    SproxelProjectPtr project, VoxelGridGroupPtr)
  {
    int fscanfStatus = 0;
    FILE* fp = fopen(qPrintable(filename), "rb");
    if (!fp) return false;

    // Read the dimensions
    Imath::V3i size(0);
    fscanfStatus = fscanf(fp, "%d,%d,%d\n", &size.x, &size.y, &size.z);

    VoxelGridGroupPtr spr(new VoxelGridGroup(size, ColorPalettePtr()));

    // Read the data
    Imath::Color4f color;
    const Imath::V3i& cellDim = size;
    for (int y = cellDim.y-1; y >= 0; y--)
    {
      for (int z = 0; z < cellDim.z; z++)
      {
        for (int x = 0; x < cellDim.x; x++)
        {
          int r, g, b, a;
          fscanfStatus = fscanf(fp, "#%02X%02X%02X%02X,", &r, &g, &b, &a);

          color.r = r / (float)0xff;
          color.g = g / (float)0xff;
          color.b = b / (float)0xff;
          color.a = a / (float)0xff;
          spr->set(Imath::V3i(x,y,z), color);

          if (x != cellDim.x-1)
            fscanfStatus = fscanf(fp, ",");
        }
        fscanfStatus = fscanf(fp, "\n");
      }
      fscanfStatus = fscanf(fp, "\n");
    }
    fclose(fp);

    spr->setName(QFileInfo(filename).baseName());
    um->addSprite(project, -1, spr);

    return true;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


static QString ensure_ext(const QString &s, const QString &ext)
{
  if (s.endsWith(ext, Qt::CaseInsensitive)) return s;
  return s+ext;
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class SproxelPngExporter : public Exporter
{
public:
  virtual QString name() { return "Sproxel PNG files"; }
  virtual QString filter() { return "*.png"; }

  virtual bool doExport(const QString &filename, SproxelProjectPtr, VoxelGridGroupPtr spr)
  {
    VoxelGridLayerPtr grid=spr->bakeLayers();
    QImage img=grid->makeQImage();
    return img.save(ensure_ext(filename, ".png"), "PNG");
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class SproxelCsvExporter : public Exporter
{
public:
  virtual QString name() { return "Sproxel CSV files"; }
  virtual QString filter() { return "*.csv"; }

  virtual bool doExport(const QString &filename, SproxelProjectPtr, VoxelGridGroupPtr spr)
  {
    FILE* fp = fopen(qPrintable(ensure_ext(filename, ".csv")), "wb");
    if (!fp) return false;

    const Imath::Box3i dim=spr->bounds();
    const Imath::V3i cellDim = dim.size()+Imath::V3i(1);
    fprintf(fp, "%d,%d,%d\n", cellDim.x, cellDim.y, cellDim.z);

    // The csv is laid out human-readable (top->bottom, Y-up, XZ, etc)
    for (int y = cellDim.y-1; y >= 0; y--)
    {
        for (int z = 0; z < cellDim.z; z++)
        {
            for (int x = 0; x < cellDim.x; x++)
            {
                const Imath::V3i curLoc=Imath::V3i(x,y,z)+dim.min;
                Imath::Color4f col = spr->get(curLoc);
                fprintf(fp, "#%02X%02X%02X%02X",
                        (int)(col.r*0xff),
                        (int)(col.g*0xff),
                        (int)(col.b*0xff),
                        (int)(col.a*0xff));
                if (x != cellDim.x-1)
                    fprintf(fp, ",");
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    return true;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ObjExporter : public Exporter
{
public:
  bool asTriangles;

  ObjExporter() : asTriangles(false) {}

  virtual QString name() { return "OBJ files"; }
  virtual QString filter() { return "*.obj"; }

  static void objWritePoly(FILE* fp, bool asTriangles,
    int v0, int v1, int v2, int v3)
  {
    if (!asTriangles)
    {
      fprintf(fp, "f %d %d %d %d\n", v0, v1, v2, v3);
    }
    else
    {
      fprintf(fp, "f %d %d %d\n", v0, v1, v2);
      fprintf(fp, "f %d %d %d\n", v2, v3, v0);
    }
  }

  virtual bool doExport(const QString &in_filename, SproxelProjectPtr, VoxelGridGroupPtr spr)
  {
      QString filename=ensure_ext(in_filename, ".obj");

      // Get file basename and extension
      QFileInfo fi(filename);
      QString basename = fi.completeBaseName();
      QString basedir = fi.absolutePath();

      // Shorthand
      const Imath::Box3i dim=spr->bounds();
      const Imath::V3i cellDim=dim.size()+Imath::V3i(1);
      const int sx = cellDim.x;
      const int sy = cellDim.y;
      const int sz = cellDim.z;

      // Create and write the material file
      std::map<std::string, std::string> mtlMap;

      // Build up the material lists
      for (int y = 0; y < sy; y++)
      {
          for (int z = 0; z < sz; z++)
          {
              for (int x = 0; x < sx; x++)
              {
                  const Imath::Color4f& color = spr->get(Imath::V3i(x, y, z)+dim.min);

                  if (color.a == 0.0f) continue;

                  char mtlName[64];
                  sprintf(mtlName, "mtl%d", (int)mtlMap.size());

                  char colorString[64];
                  sprintf(colorString, "Kd %.4f %.4f %.4f", color.r, color.g, color.b);
                  mtlMap.insert(std::pair<std::string, std::string>(std::string(colorString), std::string(mtlName)));
              }
          }
      }

      // Write .mtl file
      QString mtlFilename = basedir + "/" + basename + ".mtl";
      FILE* fp = fopen(mtlFilename.toAscii().constData(), "wb");
      if (!fp) return false;

      for(std::map<std::string, std::string>::iterator p = mtlMap.begin();
          p != mtlMap.end();
          ++p)
      {
          fprintf(fp, "newmtl %s\n", p->second.c_str());
          fprintf(fp, "illum 4\n");
          fprintf(fp, "%s\n", p->first.c_str());
          fprintf(fp, "Ka 0.00 0.00 0.00\n");
          fprintf(fp, "Tf 1.00 1.00 1.00\n");
          fprintf(fp, "Ni 1.00\n");
          fprintf(fp, "\n");
      }
      fclose(fp);


      // Create and write the obj file
      fp = fopen(filename.toAscii().constData(), "wb");
      if (!fp) return false;

      // Geometry
      const int vertListLength = (sx+1) * (sy+1) * (sz+1);
      int* vertList = new int[vertListLength];
      memset(vertList, 0, sizeof(int)*vertListLength);

      // Material library
      fprintf(fp, "mtllib %s.mtl\n", basename.toAscii().constData());

      // The object's name
      fprintf(fp, "g %s\n", basename.toAscii().constData());

      // Populate the vert list
      int vertIndex = 1;
      for (int y = 0; y < (sy+1); y++)
      {
          for (int z = 0; z < (sz+1); z++)
          {
              for (int x = 0; x < (sx+1); x++)
              {
                  int neighbors = 0;
                  if ((x!=0)  && (y!=0)  && (z!=0)  && (spr->get(Imath::V3i(x-1, y-1, z-1)+dim.min).a != 0.0f)) neighbors++;
                  if ((x!=0)  && (y!=0)  && (z!=sz) && (spr->get(Imath::V3i(x-1, y-1, z  )+dim.min).a != 0.0f)) neighbors++;
                  if ((x!=0)  && (y!=sy) && (z!=0)  && (spr->get(Imath::V3i(x-1, y,   z-1)+dim.min).a != 0.0f)) neighbors++;
                  if ((x!=0)  && (y!=sy) && (z!=sz) && (spr->get(Imath::V3i(x-1, y,   z  )+dim.min).a != 0.0f)) neighbors++;
                  if ((x!=sx) && (y!=0)  && (z!=0)  && (spr->get(Imath::V3i(x,   y-1, z-1)+dim.min).a != 0.0f)) neighbors++;
                  if ((x!=sx) && (y!=0)  && (z!=sz) && (spr->get(Imath::V3i(x,   y-1, z  )+dim.min).a != 0.0f)) neighbors++;
                  if ((x!=sx) && (y!=sy) && (z!=0)  && (spr->get(Imath::V3i(x,   y,   z-1)+dim.min).a != 0.0f)) neighbors++;
                  if ((x!=sx) && (y!=sy) && (z!=sz) && (spr->get(Imath::V3i(x,   y,   z  )+dim.min).a != 0.0f)) neighbors++;

                  if (neighbors == 0 || neighbors == 8)
                      continue;

                  const int vlIndex = (y*(sz+1)*(sx+1)) + (z*(sx+1)) + (x);
                  vertList[vlIndex] = vertIndex;
                  vertIndex++;
              }
          }
      }

      // Write the verts to the OBJ
      for (int y = 0; y < (sy+1); y++)
      {
          for (int z = 0; z < (sz+1); z++)
          {
              for (int x = 0; x < (sx+1); x++)
              {
                  Imath::V3i voxelToCheck = Imath::V3i(x,y,z);
                  if (x == sx) voxelToCheck.x--;
                  if (y == sy) voxelToCheck.y--;
                  if (z == sz) voxelToCheck.z--;

                  const Imath::M44d mat = spr->voxelTransform(voxelToCheck+dim.min);

                  Imath::V3d vert;
                  mat.multVecMatrix(Imath::V3d((x == sx) ? 0.5f : -0.5f,
                                               (y == sy) ? 0.5f : -0.5f,
                                               (z == sz) ? 0.5f : -0.5f), vert);

                  const int vlIndex = (y*(sz+1)*(sx+1)) + (z*(sx+1)) + (x);
                  if (vertList[vlIndex] != 0)
                  {
                      fprintf(fp, "v %f %f %f\n", vert.x, vert.y, vert.z);
                  }
              }
          }
      }

      // Create all faces
      for (int y = 0; y < sy; y++)
      {
          for (int z = 0; z < sz; z++)
          {
              for (int x = 0; x < sx; x++)
              {
                  const Imath::Color4f& color = spr->get(Imath::V3i(x, y, z)+dim.min);
                  if (color.a == 0.0f)
                      continue;

                  // Check for crossings
                  bool crossNegX = false;
                  bool crossPosX = false;
                  if (x == 0)
                      crossNegX = true;
                  else if (color.a != spr->get(Imath::V3i(x-1,y,z)+dim.min).a)
                      crossNegX = true;

                  if (x == sx-1)
                      crossPosX = true;
                  else if (color.a != spr->get(Imath::V3i(x+1,y,z)+dim.min).a)
                      crossPosX = true;

                  bool crossNegY = false;
                  bool crossPosY = false;
                  if (y == 0)
                      crossNegY = true;
                  else if (color.a != spr->get(Imath::V3i(x,y-1,z)+dim.min).a)
                      crossNegY = true;

                  if (y == sy-1)
                      crossPosY = true;
                  else if (color.a != spr->get(Imath::V3i(x,y+1,z)+dim.min).a)
                      crossPosY = true;

                  bool crossNegZ = false;
                  bool crossPosZ = false;
                  if (z == 0)
                      crossNegZ = true;
                  else if (color.a != spr->get(Imath::V3i(x,y,z-1)+dim.min).a)
                      crossNegZ = true;

                  if (z == sz-1)
                      crossPosZ = true;
                  else if (color.a != spr->get(Imath::V3i(x,y,z+1)+dim.min).a)
                      crossPosZ = true;

                  // If there are any crossings, you will need a material
                  if (crossNegX || crossPosX || crossNegY || crossPosY || crossNegZ || crossPosZ)
                  {
                      char colorString[64];
                      sprintf(colorString, "Kd %.4f %.4f %.4f", color.r, color.g, color.b);
                      const std::string mtl = mtlMap.find(colorString)->second;
                      fprintf(fp, "usemtl %s\n", mtl.c_str());
                  }

                  // Fill in the voxels
                  const int* vl = vertList;
                  const int  vi       = ((y)  *(sz+1)*(sx+1)) + ((z)  *(sx+1)) + (x);
                  const int  viNextZ  = ((y)  *(sz+1)*(sx+1)) + ((z+1)*(sx+1)) + (x);
                  const int  viNextY  = ((y+1)*(sz+1)*(sx+1)) + ((z)  *(sx+1)) + (x);
                  const int  viNextZY = ((y+1)*(sz+1)*(sx+1)) + ((z+1)*(sx+1)) + (x);

                  if (crossNegX)
                      objWritePoly(fp, asTriangles, vl[vi],   vl[viNextZ],   vl[viNextZY],   vl[viNextY]);
                  if (crossPosX)
                      objWritePoly(fp, asTriangles, vl[vi+1], vl[viNextY+1], vl[viNextZY+1], vl[viNextZ+1]);

                  if (crossNegY)
                      objWritePoly(fp, asTriangles, vl[vi],      vl[vi+1],     vl[viNextZ+1],  vl[viNextZ]);
                  if (crossPosY)
                      objWritePoly(fp, asTriangles, vl[viNextY], vl[viNextZY], vl[viNextZY+1], vl[viNextY+1]);

                  if (crossNegZ)
                      objWritePoly(fp, asTriangles, vl[vi],      vl[viNextY],  vl[viNextY+1],  vl[vi+1]);
                  if (crossPosZ)
                      objWritePoly(fp, asTriangles, vl[viNextZ], vl[viNextZ+1], vl[viNextZY+1], vl[viNextZY]);
              }
          }
      }

      delete[] vertList;
      fclose(fp);
      return true;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ObjTriangleExporter : public ObjExporter
{
public:
  ObjTriangleExporter() { asTriangles=true; }
  virtual QString name() { return "OBJ triangle files"; }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class PalImporter : public Importer
{
public:
  virtual QString name() { return "JASC-PAL palette files"; }
  virtual QString filter() { return "*.pal"; }

  virtual bool doImport(const QString &filename, UndoManager *um,
    SproxelProjectPtr project, VoxelGridGroupPtr)
  {
    FILE *fp=fopen(qPrintable(filename), "rt");
    if (!fp) return false;

    SproxelColor colors[256];

    char str[9];
    int num=0;
    if (fscanf(fp, "%8s\n", str)!=1 || strcmp(str, "JASC-PAL")!=0) goto error;
    if (fscanf(fp, "%4s\n", str)!=1 || strcmp(str, "0100")!=0) goto error;

    if (fscanf(fp, "%u\n", &num)!=1) goto error;

    if (num>256) num=256;

    for (int i=0; i<num; ++i)
    {
      int r, g, b;
      if (fscanf(fp, "%u %u %u\n", &r, &g, &b)!=3) goto error;
      colors[i]=SproxelColor(r/255.0f, g/255.0f, b/255.0f, 1);
    }

    fclose(fp);

    um->beginMacro("Import palette");
    for (int i=0; i<num; ++i) um->setPaletteColor(project->mainPalette, i, colors[i]);
    um->endMacro();

    return true;

  error:
    fclose(fp);
    return false;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class PalExporter : public Exporter
{
public:
  virtual QString name() { return "JASC-PAL palette files"; }
  virtual QString filter() { return "*.pal"; }

  static int convert(float f)
  {
    int i=int(f*255);
    if (i>255) return 255;
    if (i<0) return 0;
    return i;
  }

  virtual bool doExport(const QString &filename, SproxelProjectPtr project, VoxelGridGroupPtr)
  {
    FILE *fp=fopen(qPrintable(ensure_ext(filename, ".pal")), "wb");
    if (!fp) return false;

    ColorPalettePtr pal=project->mainPalette;

    fprintf(fp, "JASC-PAL\n0100\n%u\n", pal->numColors());

    for (int i=0; i<pal->numColors(); ++i)
    {
      SproxelColor c=pal->color(i);
      fprintf(fp, "%u %u %u\n", convert(c.r), convert(c.g), convert(c.b));
    }

    fclose(fp);

    return true;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void register_builtin_importers_exporters()
{
  #define REG(name) static name s_##name; register_importer(& s_##name);
  REG(SproxelPngImporter)
  REG(SproxelCsvImporter)
  REG(ImageImporter)
  REG(PalImporter)
  #undef REG

  #define REG(name) static name s_##name; register_exporter(& s_##name);
  REG(ObjExporter)
  REG(ObjTriangleExporter)
  REG(SproxelPngExporter)
  REG(SproxelCsvExporter)
  REG(PalExporter)
  #undef REG
}

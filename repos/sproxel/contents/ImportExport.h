#ifndef __IMPORT_EXPORT_H__
#define __IMPORT_EXPORT_H__


#include "SproxelProject.h"
#include "UndoManager.h"


class Importer
{
public:
  virtual QString name()=0;
  virtual QString filter()=0;
  virtual bool doImport(const QString &filename, UndoManager *um,
    SproxelProjectPtr project, VoxelGridGroupPtr current_sprite)=0;
};


void   register_importer(Importer*);
void unregister_importer(Importer*);

const QList<Importer*>& get_importers();


class Exporter
{
public:
  virtual QString name()=0;
  virtual QString filter()=0;
  virtual bool doExport(const QString &filename, SproxelProjectPtr project, VoxelGridGroupPtr current_sprite)=0;
};


void   register_exporter(Exporter*);
void unregister_exporter(Exporter*);

const QList<Exporter*>& get_exporters();


void register_builtin_importers_exporters();


#endif

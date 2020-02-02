#ifndef __UNDO_MANAGER_H__
#define __UNDO_MANAGER_H__

#include "Global.h"
#include "SproxelProject.h"

#include <QString>
#include <QObject>
#include <QVector>
#include <QUndoStack>
#include <QUndoCommand>


// A wrapper class for Sproxel.
// Allows for an easy subset of undo/redo operations.
class UndoManager : public QObject
{
    Q_OBJECT

public:

    enum { ID_SETVOXEL=1, };

    UndoManager();
    virtual ~UndoManager() {}

    void changeEntireVoxelGrid(VoxelGridGroupPtr origGrid,
                               const VoxelGridGroupPtr newGrid);

    void setVoxelColor(VoxelGridGroupPtr origGrid,
                       const Imath::V3i& at,
                       const Imath::Color4f& color,
                       int index);

    void setPaletteColor(ColorPalettePtr pal, int index, const SproxelColor &color);

    void addSprite(SproxelProjectPtr proj, int at, VoxelGridGroupPtr spr);
    void removeSprite(SproxelProjectPtr proj, int at);

    void renameSprite(VoxelGridGroupPtr spr, const QString &name);

    void beginMacro(const QString& macroName);
    void endMacro();
    void clear();

    void undo();
    void redo();

    void setClean();
    bool isClean() const;

    QAction* createUndoAction(QObject *parent, const QString &prefix)
      { return m_undoStack.createUndoAction(parent, prefix); }

    QAction* createRedoAction(QObject *parent, const QString &prefix)
      { return m_undoStack.createRedoAction(parent, prefix); }

    void onSpriteChanged(VoxelGridGroupPtr);
    void onPaletteChanged(ColorPalettePtr);
    void onBeforeSpriteAdded(SproxelProjectPtr, int);
    void onSpriteAdded(SproxelProjectPtr, int);
    void onBeforeSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr);
    void onSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr);

signals:
    void cleanChanged(bool);
    void spriteChanged(VoxelGridGroupPtr);
    void paletteChanged(ColorPalettePtr);
    void beforeSpriteAdded(SproxelProjectPtr, int);
    void spriteAdded(SproxelProjectPtr, int);
    void beforeSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr);
    void spriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr);

private:
    QUndoStack m_undoStack;

};


// Rename sprite
class CmdRenameSprite : public QUndoCommand
{
public:

  CmdRenameSprite(UndoManager *mgr, VoxelGridGroupPtr spr, const QString &name)
    : m_manager(mgr), m_sprite(spr), m_newName(name)
  {
    m_oldName=spr->name();
    setText("Rename sprite");
  }

  virtual void redo()
  {
    m_sprite->setName(m_newName);
    m_manager->onSpriteChanged(m_sprite);
  }

  virtual void undo()
  {
    m_sprite->setName(m_oldName);
    m_manager->onSpriteChanged(m_sprite);
  }

private:
  UndoManager *m_manager;
  VoxelGridGroupPtr m_sprite;
  QString m_oldName, m_newName;
};


// Add sprite to project
class CmdAddSprite : public QUndoCommand
{
public:

  CmdAddSprite(UndoManager *mgr, SproxelProjectPtr proj, int at, VoxelGridGroupPtr spr)
    : m_manager(mgr), m_project(proj), m_index(at), m_sprite(spr)
  {
    setText("Add sprite");
  }

  virtual void redo()
  {
    m_manager->onBeforeSpriteAdded(m_project, m_index);
    m_project->sprites.insert(m_index, m_sprite);
    m_manager->onSpriteAdded(m_project, m_index);
  }

  virtual void undo()
  {
    if (m_index>=m_project->sprites.size() || m_project->sprites[m_index]!=m_sprite) return;

    m_manager->onBeforeSpriteRemoved(m_project, m_index, m_sprite);
    m_project->sprites.remove(m_index);
    m_manager->onSpriteRemoved(m_project, m_index, m_sprite);
  }

private:
  UndoManager *m_manager;
  SproxelProjectPtr m_project;
  VoxelGridGroupPtr m_sprite;
  int m_index;
};


// Remove sprite from project
class CmdRemoveSprite : public QUndoCommand
{
public:

  CmdRemoveSprite(UndoManager *mgr, SproxelProjectPtr proj, int at)
    : m_manager(mgr), m_project(proj), m_index(at)
  {
    m_sprite=m_project->sprites[m_index];
    setText("Remove sprite");
  }

  virtual void redo()
  {
    if (m_index>=m_project->sprites.size() || m_project->sprites[m_index]!=m_sprite) return;

    m_manager->onBeforeSpriteRemoved(m_project, m_index, m_sprite);
    m_project->sprites.remove(m_index);
    m_manager->onSpriteRemoved(m_project, m_index, m_sprite);
  }

  virtual void undo()
  {
    m_manager->onBeforeSpriteAdded(m_project, m_index);
    m_project->sprites.insert(m_index, m_sprite);
    m_manager->onSpriteAdded(m_project, m_index);
  }

private:
  UndoManager *m_manager;
  SproxelProjectPtr m_project;
  VoxelGridGroupPtr m_sprite;
  int m_index;
};


// Change single palette color
class CmdSetPaletteColor : public QUndoCommand
{
public:

  CmdSetPaletteColor(UndoManager *mgr, ColorPalettePtr pal, int index, const SproxelColor &color)
    : m_manager(mgr), m_palette(pal), m_index(index), m_newColor(color)
  {
    m_oldColor=pal->color(index);
    setText("Change palette color");
  }

  virtual void redo()
  {
    m_palette->setColor(m_index, m_newColor);
    m_manager->onPaletteChanged(m_palette);
  }

  virtual void undo()
  {
    m_palette->setColor(m_index, m_oldColor);
    m_manager->onPaletteChanged(m_palette);
  }

private:
  UndoManager *m_manager;
  ColorPalettePtr m_palette;
  int m_index;
  SproxelColor m_oldColor, m_newColor;
};


// ChangeEntireVoxelGrid (which cannot be Macro'ed)
class CmdChangeEntireVoxelGrid : public QUndoCommand
{
public:
    CmdChangeEntireVoxelGrid(UndoManager *mgr, VoxelGridGroupPtr gvg, const VoxelGridGroupPtr newGrid) :
        m_manager(mgr), m_pGvg(gvg)
    {
        m_newGrid = new VoxelGridGroup(*newGrid);
        m_oldGrid = new VoxelGridGroup(*gvg);
        setText("Change grid");
    }

    virtual void redo()
    {
        *m_pGvg = *m_newGrid;
        m_manager->onSpriteChanged(m_pGvg);
    }

    virtual void undo()
    {
        *m_pGvg = *m_oldGrid;
        m_manager->onSpriteChanged(m_pGvg);
    }

private:
    UndoManager *m_manager;
    VoxelGridGroupPtr m_pGvg;
    VoxelGridGroupPtr m_newGrid;
    VoxelGridGroupPtr m_oldGrid;
};


// SetVoxelColor (which can be Macro'ed)
class CmdSetVoxelColor : public QUndoCommand
{
public:

  CmdSetVoxelColor(UndoManager *mgr, VoxelGridGroupPtr spr, VoxelGridLayerPtr layer,
    const Imath::V3i& pos, const Imath::Color4f &color, int index)
    : m_manager(mgr), m_sprite(spr), m_layer(layer)
  {
    m_changes.push_back(Change(pos, m_layer->getColor(pos), m_layer->getInd(pos), color, index));
    setText("Set voxel");
  }

  virtual void redo()
  {
    for (int i=0; i<m_changes.size(); ++i)
    {
      const Change &c=m_changes[i];
      m_layer->set(c.pos, c.newColor, c.newIndex);
    }

    m_manager->onSpriteChanged(m_sprite);
  }

  virtual void undo()
  {
    for (int i=m_changes.size()-1; i>=0; --i)
    {
      const Change &c=m_changes[i];
      m_layer->set(c.pos, c.oldColor, c.oldIndex);
    }

    m_manager->onSpriteChanged(m_sprite);
  }

  virtual int id() const { return UndoManager::ID_SETVOXEL; }

protected:

  struct Change
  {
    Imath::V3i pos;
    SproxelColor oldColor, newColor;
    int oldIndex, newIndex;

    Change() {}
    Change(const Imath::V3i& p, const Imath::Color4f &oc, int oi, const Imath::Color4f &nc, int ni)
      : pos(p), oldColor(oc), oldIndex(oi), newColor(nc), newIndex(ni) {}
  };

  virtual bool mergeWith(const QUndoCommand* other)
  {
    if (other->id() != id()) return false;

    const CmdSetVoxelColor* otherSet = static_cast<const CmdSetVoxelColor*>(other);
    if (otherSet->m_layer!=m_layer || otherSet->m_sprite!=m_sprite) return false;

    m_changes+=otherSet->m_changes;
    return true;
  }

private:
  UndoManager *m_manager;
  VoxelGridGroupPtr m_sprite;
  VoxelGridLayerPtr m_layer;
  QVector<Change> m_changes;
};

#endif

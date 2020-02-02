#include "UndoManager.h"


UndoManager::UndoManager()
{
    QObject::connect(&m_undoStack, SIGNAL(cleanChanged(bool)),
                     this, SIGNAL(cleanChanged(bool)));
}


void UndoManager::onSpriteChanged(VoxelGridGroupPtr spr)
{
  emit spriteChanged(spr);
}


void UndoManager::onPaletteChanged(ColorPalettePtr pal)
{
  emit paletteChanged(pal);
}


void UndoManager::onBeforeSpriteAdded(SproxelProjectPtr proj, int i)
{
  emit beforeSpriteAdded(proj, i);
}


void UndoManager::onSpriteAdded(SproxelProjectPtr proj, int i)
{
  emit spriteAdded(proj, i);
}


void UndoManager::onBeforeSpriteRemoved(SproxelProjectPtr proj, int i, VoxelGridGroupPtr spr)
{
  emit beforeSpriteRemoved(proj, i, spr);
}


void UndoManager::onSpriteRemoved(SproxelProjectPtr proj, int i, VoxelGridGroupPtr spr)
{
  emit spriteRemoved(proj, i, spr);
}


void UndoManager::changeEntireVoxelGrid(VoxelGridGroupPtr origGrid,
                                        const VoxelGridGroupPtr newGrid)
{
    m_undoStack.push(new CmdChangeEntireVoxelGrid(this, origGrid, newGrid));
}


void UndoManager::setVoxelColor(VoxelGridGroupPtr sprite,
                                const Imath::V3i& pos,
                                const Imath::Color4f& color,
                                int index)
{
    // Validity check
    if (!sprite) return;

    VoxelGridLayerPtr layer=sprite->curLayer();
    if (!layer) return;

    m_undoStack.push(new CmdSetVoxelColor(this, sprite, layer, pos, color, index));
}


void UndoManager::setPaletteColor(ColorPalettePtr pal, int index, const SproxelColor &color)
{
  if (!pal) return;

  m_undoStack.push(new CmdSetPaletteColor(this, pal, index, color));
}


void UndoManager::addSprite(SproxelProjectPtr proj, int at, VoxelGridGroupPtr spr)
{
  if (!proj || !spr) return;
  if (at<0 || at>proj->sprites.size()) at=proj->sprites.size();

  m_undoStack.push(new CmdAddSprite(this, proj, at, spr));
}


void UndoManager::removeSprite(SproxelProjectPtr proj, int at)
{
  if (!proj) return;
  if (at<0 || at>=proj->sprites.size()) return;

  m_undoStack.push(new CmdRemoveSprite(this, proj, at));
}


void UndoManager::renameSprite(VoxelGridGroupPtr spr, const QString &name)
{
  if (!spr) return;
  m_undoStack.push(new CmdRenameSprite(this, spr, name));
}


void UndoManager::beginMacro(const QString& macroName)
{
    m_undoStack.beginMacro(macroName);
}


void UndoManager::endMacro()
{
    m_undoStack.endMacro();
}


void UndoManager::clear()
{
    m_undoStack.clear();
}


void UndoManager::setClean()
{
    m_undoStack.setClean();
}


bool UndoManager::isClean() const
{
    return m_undoStack.isClean();
}


void UndoManager::undo()
{
    m_undoStack.undo();
}


void UndoManager::redo()
{
    m_undoStack.redo();
}

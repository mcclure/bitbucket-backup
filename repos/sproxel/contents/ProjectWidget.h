#ifndef __PROJECT_WIDGET_H__
#define __PROJECT_WIDGET_H__

#include <QWidget>
#include <QListView>
#include <QStyledItemDelegate>
#include <QImage>
#include <QPixmap>
#include <QVector>
#include <QSettings>
#include "UndoManager.h"
#include "SproxelProject.h"


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class SpriteListModel : public QAbstractListModel
{
  Q_OBJECT

public:
  SpriteListModel(QSettings *sett, UndoManager *um, QObject* parent = NULL)
    : QAbstractListModel(parent), p_appSettings(sett), p_undoManager(um)
  {
  }

  void setProject(SproxelProjectPtr prj)
  {
    beginResetModel();
    m_project=prj;
    m_icons.clear();
    m_icons.resize(m_project->sprites.size());
    updateIcons();
    endResetModel();
  }

  void updateIcons()
  {
    for (int i=0; i<m_project->sprites.size(); ++i)
      if (!m_icons[i]) updateIcon(i);
  }

signals:
  void spriteSelected(VoxelGridGroupPtr);

public slots:
  void onSpriteChanged(VoxelGridGroupPtr spr);
  void onPaletteChanged(ColorPalettePtr pal);
  void onBeforeSpriteAdded(SproxelProjectPtr, int);
  void onSpriteAdded(SproxelProjectPtr, int);
  void onBeforeSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr);
  void onSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr);

  void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
  SproxelProjectPtr m_project;
  QVector<QPixmap> m_icons;

  QSettings *p_appSettings;
  UndoManager *p_undoManager;

  void updateIcon(int i);

public:

  int rowCount(const QModelIndex&) const
  {
    return m_project->sprites.size();
  }


  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
  {
    if (!index.isValid()) return QVariant();

    if (role==Qt::DisplayRole || role==Qt::EditRole || role==Qt::ToolTipRole)
    {
      return QString(m_project->sprites[index.row()]->name());
    }
    else if (role == Qt::DecorationRole)
    {
      return m_icons[index.row()];
    }
    return QVariant();
  }


  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)
  {
    int row=index.row();
    if (role==Qt::EditRole)
    {
      p_undoManager->renameSprite(m_project->sprites[row], value.toString());
      emit dataChanged(index, index);
      return true;
    }

    return false;
  }


  bool dropMimeData(const QMimeData *mime, Qt::DropAction action, int row, int col, const QModelIndex &parent);


  Qt::DropActions supportedDropActions() const
  {
    return Qt::CopyAction;
  }


  Qt::ItemFlags flags(const QModelIndex& index) const
  {
    Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.isValid())
      return Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | defaultFlags;
    else
      return Qt::ItemIsDropEnabled | defaultFlags;
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class ProjectWidget : public QWidget
{
  Q_OBJECT

public:
  ProjectWidget(QWidget* parent, UndoManager *um, QSettings *sett);
  ~ProjectWidget();

  QSize minimumSizeHint() const;
  QSize sizeHint() const;

  void setProject(SproxelProjectPtr);

signals:
  void spriteSelected(VoxelGridGroupPtr);

public slots:
  void newSprite();
  void deleteSelected();
  void duplicateSelected();

  void indexesMoved(const QModelIndexList &list);

protected:
  void paintEvent(QPaintEvent *event);

private:
  UndoManager *p_undoManager;
  SproxelProjectPtr m_project;
  QListView* m_sprListView;
  SpriteListModel* m_sprListModel;
  QStyledItemDelegate *m_sprDelegate;
};


#endif

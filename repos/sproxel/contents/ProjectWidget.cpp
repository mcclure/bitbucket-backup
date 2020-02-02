#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMimeData>
#include "ProjectWidget.h"
#include "NewGridDialog.h"


#define ICON_SIZE 60
#define WIDGET_W ((ICON_SIZE+10)*1+40)
#define WIDGET_H ((ICON_SIZE+20)*1+30)


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void SpriteListModel::updateIcon(int i)
{
  if (!m_project) return;

  QImage img(ICON_SIZE, ICON_SIZE, QImage::Format_ARGB32_Premultiplied);

  img.fill(p_appSettings->value("GLModelWidget/backgroundColor", QColor(0, 0, 0)).value<QColor>().rgb());

  VoxelGridGroupPtr spr=m_project->sprites[i];
  Imath::Box3i bounds=spr->bounds();
  Imath::V3i size=bounds.size()+Imath::V3i(1);

  float sx=size.x/float(ICON_SIZE);
  float sy=size.y/float(ICON_SIZE);

  float scale=sx;
  if (sy>scale) scale=sy;
  if (scale<0.25f) scale=0.25f;

  int ox=bounds.min.x-int((ICON_SIZE*scale-size.x)*0.5f);
  int oy=bounds.min.y-int((ICON_SIZE*scale-size.y)*0.5f);

  for (int y=0; y<ICON_SIZE; ++y)
  {
    int gy=int(y*scale)+oy;
    for (int x=0; x<ICON_SIZE; ++x)
    {
      int gx=int(x*scale)+ox;

      uint color=qRgba(0, 0, 0, 0);
      for (int gz=bounds.max.z; gz>=bounds.min.z; --gz)
      {
        SproxelColor c=spr->get(Imath::V3i(gx, gy, gz));
        if (c.a==0) continue;

        color=qRgba(int(c.r*255), int(c.g*255), int(c.b*255), int(c.a*255));
        break;
      }

      if (color) img.setPixel(x, y, color);
    }
  }

  img=img.mirrored();

  m_icons[i].convertFromImage(img);

  QModelIndex index=createIndex(i, 0);
  emit dataChanged(index, index);
}


void SpriteListModel::onSpriteChanged(VoxelGridGroupPtr spr)
{
  if (!m_project) return;

  for (int i=0; i<m_project->sprites.size(); ++i)
    if (m_project->sprites[i]==spr)
    {
      m_icons[i]=QPixmap();
      break;
    }
}


void SpriteListModel::onPaletteChanged(ColorPalettePtr pal)
{
  if (!m_project) return;

  for (int i=0; i<m_project->sprites.size(); ++i)
    if (m_project->sprites[i]->hasPalette(pal))
      m_icons[i]=QPixmap();
}


void SpriteListModel::onBeforeSpriteAdded(SproxelProjectPtr proj, int at)
{
  if (m_project!=proj) return;

  beginInsertRows(QModelIndex(), at, at);
}


void SpriteListModel::onSpriteAdded(SproxelProjectPtr proj, int at)
{
  if (m_project!=proj) return;

  m_icons.insert(at, QPixmap());
  updateIcon(at);
  endInsertRows();
}


void SpriteListModel::onBeforeSpriteRemoved(SproxelProjectPtr proj, int at, VoxelGridGroupPtr spr)
{
  if (m_project!=proj) return;

  beginRemoveRows(QModelIndex(), at, at);
}


void SpriteListModel::onSpriteRemoved(SproxelProjectPtr proj, int at, VoxelGridGroupPtr spr)
{
  if (m_project!=proj) return;

  m_icons.remove(at);
  endRemoveRows();
}


void SpriteListModel::currentChanged(const QModelIndex &current, const QModelIndex &)
{
  if (current.isValid())
  {
    emit spriteSelected(m_project->sprites[current.row()]);
  }
}


bool SpriteListModel::dropMimeData(const QMimeData *mime, Qt::DropAction, int row, int, const QModelIndex &parent)
{
  QByteArray encoded = mime->data("application/x-qabstractitemmodeldatalist");
  if (encoded.size())
  {
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    int fromRow=-1;
    stream >> fromRow;

    if (row<0) row=parent.row();
    if (row<0 || fromRow<0) return false;
    if (row==fromRow) return false;

    p_undoManager->beginMacro("Move sprite");
    VoxelGridGroupPtr spr=m_project->sprites[fromRow];
    p_undoManager->removeSprite(m_project, fromRow);
    p_undoManager->addSprite(m_project, row, spr);
    p_undoManager->endMacro();

    return true;
  }

  return false;
}


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


class SpriteItemDelegate : public QStyledItemDelegate
{
public:
  SpriteItemDelegate(QObject *p) : QStyledItemDelegate(p) {}

  QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
  {
    return QSize(ICON_SIZE+10, ICON_SIZE+20);
  }
};


//ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


ProjectWidget::ProjectWidget(QWidget* parent, UndoManager *um, QSettings *sett)
  : QWidget(parent), p_undoManager(um)
{
  QVBoxLayout *layout=new QVBoxLayout(this);

  m_sprListView=new QListView;
  m_sprListView->setUniformItemSizes(true);
  m_sprListView->setViewMode(QListView::IconMode);
  m_sprListView->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
  m_sprListView->setGridSize(QSize(ICON_SIZE+10, ICON_SIZE+20));
  m_sprListView->setMovement(QListView::Snap);
  m_sprListView->setTextElideMode(Qt::ElideMiddle);
  m_sprListView->setResizeMode(QListView::Adjust);

  // set custom delegate to ensure constant item size
  m_sprDelegate=new SpriteItemDelegate(this);
  m_sprListView->setItemDelegate(m_sprDelegate);

  m_sprListModel=new SpriteListModel(sett, um, this);
  m_sprListView->setModel(m_sprListModel);

  QHBoxLayout *buttonBar=new QHBoxLayout(this);
  layout->addLayout(buttonBar);

  QPushButton *button;
  button=new QPushButton("");
  button->setIcon(QIcon(QPixmap(":/icons/layerNew.png")));
  button->setToolTip("New sprite");
  connect(button, SIGNAL(pressed()), this, SLOT(newSprite()));
  buttonBar->addWidget(button);

  button=new QPushButton("");
  button->setIcon(QIcon(QPixmap(":/icons/layerDuplicate.png")));
  button->setToolTip("Clone sprite");
  connect(button, SIGNAL(pressed()), this, SLOT(duplicateSelected()));
  buttonBar->addWidget(button);

  button=new QPushButton("");
  button->setIcon(QIcon(QPixmap(":/icons/layerDelete.png")));
  button->setToolTip("Delete sprite");
  connect(button, SIGNAL(pressed()), this, SLOT(deleteSelected()));
  buttonBar->addWidget(button);

  layout->addWidget(m_sprListView);

  connect(p_undoManager, SIGNAL(spriteChanged(VoxelGridGroupPtr)),
    m_sprListModel, SLOT(onSpriteChanged(VoxelGridGroupPtr)));
  connect(p_undoManager, SIGNAL(spriteChanged(VoxelGridGroupPtr)),
    this, SLOT(update()));

  connect(p_undoManager, SIGNAL(paletteChanged(ColorPalettePtr)),
    m_sprListModel, SLOT(onPaletteChanged(ColorPalettePtr)));

  connect(p_undoManager, SIGNAL(beforeSpriteAdded(SproxelProjectPtr, int)),
    m_sprListModel, SLOT(onBeforeSpriteAdded(SproxelProjectPtr, int)));
  connect(p_undoManager, SIGNAL(spriteAdded(SproxelProjectPtr, int)),
    m_sprListModel, SLOT(onSpriteAdded(SproxelProjectPtr, int)));

  connect(p_undoManager, SIGNAL(beforeSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr)),
    m_sprListModel, SLOT(onBeforeSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr)));
  connect(p_undoManager, SIGNAL(spriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr)),
    m_sprListModel, SLOT(onSpriteRemoved(SproxelProjectPtr, int, VoxelGridGroupPtr)));

  connect(m_sprListView->selectionModel(), SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
    m_sprListModel, SLOT(currentChanged(const QModelIndex &, const QModelIndex &)));

  connect(m_sprListModel, SIGNAL(spriteSelected(VoxelGridGroupPtr)),
    this, SIGNAL(spriteSelected(VoxelGridGroupPtr)));

  connect(m_sprListView, SIGNAL(indexesMoved(const QModelIndexList&)),
    this, SLOT(indexesMoved(const QModelIndexList&)));
}


ProjectWidget::~ProjectWidget()
{
  if (m_sprDelegate) delete m_sprDelegate;
}


QSize ProjectWidget::minimumSizeHint() const
{
  return QSize(WIDGET_W, WIDGET_H);
}


QSize ProjectWidget::sizeHint() const
{
  return QSize(WIDGET_W, WIDGET_H*2);
}


void ProjectWidget::setProject(SproxelProjectPtr prj)
{
  m_project=prj;
  m_sprListModel->setProject(prj);
  m_sprListView->setCurrentIndex(m_sprListModel->index(0, 0));
}


void ProjectWidget::paintEvent(QPaintEvent *event)
{
  m_sprListModel->updateIcons();
  QWidget::paintEvent(event);
}


void ProjectWidget::indexesMoved(const QModelIndexList &)
{
  // reset list layout when item is dropped into empty space
  m_sprListView->setMovement(QListView::Static);
  m_sprListView->setMovement(QListView::Snap);
}


void ProjectWidget::newSprite()
{
  NewGridDialog dlg(this);
  dlg.setModal(true);
  if (dlg.exec())
  {
    VoxelGridGroupPtr sprite(new VoxelGridGroup(dlg.getVoxelSize(),
      dlg.isIndexed()?m_project->mainPalette:ColorPalettePtr()));
    sprite->setName("unnamed");

    int at=m_sprListView->currentIndex().row();
    if (at<0) at=m_project->sprites.size();

    p_undoManager->addSprite(m_project, at, sprite);

    QModelIndex index=m_sprListModel->index(at, 0);
    m_sprListView->setCurrentIndex(index);
    m_sprListView->edit(index);
  }
}


void ProjectWidget::deleteSelected()
{
  QModelIndex cur=m_sprListView->currentIndex();
  if (!cur.isValid()) return;

  if (m_project->sprites.size()<=1) return; // don't delete last sprite - we need to show it in the editor!

  p_undoManager->removeSprite(m_project, cur.row());
}


void ProjectWidget::duplicateSelected()
{
  QModelIndex cur=m_sprListView->currentIndex();
  if (!cur.isValid()) return;

  VoxelGridGroupPtr spr(new VoxelGridGroup(*m_project->sprites[cur.row()]));
  if (!spr->name().endsWith(" copy")) spr->setName(spr->name()+" copy");

  p_undoManager->addSprite(m_project, cur.row()+1, spr);

  QModelIndex index=m_sprListModel->index(cur.row()+1, 0);
  m_sprListView->setCurrentIndex(index);
  m_sprListView->edit(index);
}

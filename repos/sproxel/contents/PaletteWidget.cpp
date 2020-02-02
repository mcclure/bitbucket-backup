#include <iostream>
#include <QPainter>
#include <QPaintEvent>
#include <QColorDialog>
#include <QToolTip>
#include "PaletteWidget.h"
#include "Tools.h"

#define CBOX_W 40
#define CBOX_H 40
#define CBOX_AX m_cboxX
#define CBOX_AY   5
#define CBOX_PX (25+m_cboxX)
#define CBOX_PY  30

#define HR_Y 75

#define PAL_X m_palX
#define PAL_Y 85
#define PAL_NX  8
#define PAL_NY 32
#define PBOX_W m_pboxW
#define PBOX_H m_pboxH


QSize PaletteWidget::minimumSizeHint() const
{
  return QSize(100, PAL_Y+PAL_NY*8+8/2);
}


QSize PaletteWidget::sizeHint() const
{
  return QSize(100, PAL_Y+PAL_NY*PBOX_H+PBOX_H/2);
}


void PaletteWidget::resizeEvent(QResizeEvent *event)
{
  int w=event->size().width();
  int h=event->size().height();

  m_pboxW=(w-5*2)/PAL_NX;
  m_pboxH=(h-PAL_Y-5)/PAL_NY;

  if (m_pboxW<2) m_pboxW=2;
  if (m_pboxH<2) m_pboxH=2;

  m_palX=(w-m_pboxW*PAL_NX)/2;

  m_cboxX=(w-(CBOX_PX-m_cboxX+CBOX_W))/2;

  //printf("palette box: %dx%d\n", m_pboxW, m_pboxH);
}


QRect PaletteWidget::palRect(int index) const
{
  if (index<0) return QRect();

  int x=PAL_X+(index%PAL_NX)*PBOX_W;
  int y=PAL_Y+(index/PAL_NX)*PBOX_H;
  return QRect(x, y, PBOX_W, PBOX_H);
}


QColor PaletteWidget::toQColor(const Imath::Color4f& in)
{
    return QColor((int)(in.r * 255.0f),
                  (int)(in.g * 255.0f),
                  (int)(in.b * 255.0f),
                  (int)(in.a * 255.0f));
}


QColor PaletteWidget::toQColor(const Imath::Color4f& in, float a)
{
    return QColor((int)(in.r * 255.0f),
                  (int)(in.g * 255.0f),
                  (int)(in.b * 255.0f),
                  (int)(   a * 255.0f));
}


Imath::Color4f PaletteWidget::toColor4f(QColor c)
{
  return Imath::Color4f(
    c.red  ()/255.0f,
    c.green()/255.0f,
    c.blue ()/255.0f,
    c.alpha()/255.0f);
}


void PaletteWidget::setPalette(ColorPalettePtr pal)
{
  m_palette=pal;
  update();
}


bool PaletteWidget::event(QEvent *event)
{
  if (event->type()==QEvent::ToolTip)
  {
    const QHelpEvent &he=*(QHelpEvent*)event;
    int ci=clickHit(he.pos());

    if (ci<0 || !m_palette)
    {
      QToolTip::hideText();
      event->ignore();
    }
    else
    {
      SproxelColor c=m_palette->color(ci);
      QToolTip::showText(he.globalPos(),
        QString("%1 (#%2): #%3\nR:%4 G:%5 B:%6 A:%7").arg(ci).arg(ci, 2, 16, QChar('0'))
          .arg(toQColor(c).rgba(), 8, 16, QChar('0'))
          .arg(int(c.r*255)).arg(int(c.g*255)).arg(int(c.b*255)).arg(int(c.a*255)),
        this, palRect(ci));
    }

    return true;
  }

  return QWidget::event(event);
}


void PaletteWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);

  // Some useful colors|tools
  Imath::Color4f brighterBackground = m_backgroundColor * 1.05f;
  brighterBackground.a = 1.0f;
  QPen brighterBackgroundPen = QPen(toQColor(brighterBackground));

  // BG clear
  painter.fillRect(0, 0, width(), height(), QBrush(toQColor(m_backgroundColor)));

  // Active and passive colors get drawn in the upper-left
  painter.fillRect(CBOX_PX  , CBOX_PY  , CBOX_W  , CBOX_H  , QBrush(QColor(0,0,0)));
  painter.fillRect(CBOX_PX+1, CBOX_PY+1, CBOX_W-2, CBOX_H-2, QBrush(QColor(255, 255, 255)));
  painter.fillRect(CBOX_PX+2, CBOX_PY+2, CBOX_W-4, CBOX_H-4, QBrush(toQColor(m_passiveColor, 1)));

  painter.fillRect(CBOX_AX  , CBOX_AY  , CBOX_W  , CBOX_H  , QBrush(QColor(0,0,0)));
  painter.fillRect(CBOX_AX+1, CBOX_AY+1, CBOX_W-2, CBOX_H-2, QBrush(QColor(255, 255, 255)));
  painter.fillRect(CBOX_AX+2, CBOX_AY+2, CBOX_W-4, CBOX_H-4, QBrush(toQColor(m_activeColor, 1)));

  // Horizontal rule
  painter.setPen(brighterBackgroundPen);
  painter.drawLine(QPoint(0, HR_Y), QPoint(width(), HR_Y));

  // Palette grid
  if (m_palette)
  {
    for (int y=0; y<PAL_NY; ++y)
      for (int x=0; x<PAL_NX; ++x)
      {
        Imath::Color4f c=m_palette->color(y*PAL_NX+x);
        painter.fillRect(PAL_X+x*PBOX_W, PAL_Y+y*PBOX_H, PBOX_W, PBOX_H, toQColor(c, 1));
      }

    if (m_hilightIndex>=0)
    {
      int x=PAL_X+(m_hilightIndex%PAL_NX)*PBOX_W;
      int y=PAL_Y+(m_hilightIndex/PAL_NX)*PBOX_H;
      painter.fillRect(x  , y  , PBOX_W  , PBOX_H  , QColor(0, 0, 0));
      painter.fillRect(x+1, y+1, PBOX_W-2, PBOX_H-2, QColor(255, 255, 255));
      painter.fillRect(x+2, y+2, PBOX_W-4, PBOX_H-4, toQColor(m_palette->color(m_hilightIndex), 1));
    }

    if (m_activeIndex>=0)
    {
      int x=PAL_X+(m_activeIndex%PAL_NX)*PBOX_W;
      int y=PAL_Y+(m_activeIndex/PAL_NX)*PBOX_H;
      painter.fillRect(x+PBOX_W/4, y+PBOX_H/2, PBOX_W/2, 1, toQColor(SproxelColor(1)-m_activeColor, 1));
      painter.fillRect(x+PBOX_W/2, y+PBOX_H/4, 1, PBOX_H/2, toQColor(SproxelColor(1)-m_activeColor, 1));
    }

    /*
    if (m_passiveIndex>=0)
    {
      int x=PAL_X+(m_passiveIndex%PAL_NX)*PBOX_W;
      int y=PAL_Y+(m_passiveIndex/PAL_NX)*PBOX_H;
      painter.fillRect(x+PBOX_W/4, y+PBOX_H/2, PBOX_W/2, 1, toQColor(SproxelColor(1)-m_passiveColor, 1));
    }
    */
  }
}


void PaletteWidget::mousePressEvent(QMouseEvent* event)
{
  QColor color;

  int ci=clickHit(event->pos());

  setHilight(ci);

  switch(ci)
  {
    case HIT_NONE:
      break;

    case HIT_ACTIVE_COLOR_BOX:
      color = QColorDialog::getColor(toQColor(m_activeColor), this,
        "Select active color", QColorDialog::ShowAlphaChannel);

      if (color.isValid()) setActiveColor(toColor4f(color), -1);
      break;

    case HIT_PASSIVE_COLOR_BOX:
      color = QColorDialog::getColor(toQColor(m_passiveColor), this,
        "Select passive color", QColorDialog::ShowAlphaChannel);

      if (color.isValid()) setPassiveColor(toColor4f(color), -1);
      break;

    default:
      if (IsLeftButton(event))
      {
        if (m_palette) setActiveColor(m_palette->color(ci), ci);
      }
      else if (IsRightButton(event))
      {
        if (m_palette)
        {
          color = QColorDialog::getColor(toQColor(m_palette->color(ci)), this,
            QString("Select palette color %1 (#%2)").arg(ci).arg(ci, 2, 16, QChar('0')),
            QColorDialog::ShowAlphaChannel);

          if (color.isValid()) p_undoManager->setPaletteColor(m_palette, ci, toColor4f(color));
        }
      }
      break;
  }
}


void PaletteWidget::mouseMoveEvent(QMouseEvent* event)
{
  setHilight(clickHit(event->pos()));
}


void PaletteWidget::setHilight(int ci)
{
  if (m_hilightIndex!=ci)
  {
    int oldIndex=m_hilightIndex;
    m_hilightIndex=ci;
    repaint(palRect(oldIndex));
    repaint(palRect(ci));
  }
}


void PaletteWidget::mouseDoubleClickEvent(QMouseEvent* /*event*/)
{
  // Emit palette changed signal
  // emit activeColorChanged(Imath::Color4f(1.0f, 0.0f, 1.0f, 1.0f));

  //std::cout << event->pos().x() << " " << event->pos().y() << std::endl;
  //std::cout << clickHit(event->pos()) << std::endl;
}


void PaletteWidget::leaveEvent(QEvent *)
{
  setHilight(HIT_NONE);
}


int PaletteWidget::clickHit(const QPoint& p)
{
  if (p.x() >= CBOX_AX && p.y() >= CBOX_AY && p.x() < CBOX_AX+CBOX_W && p.y() < CBOX_AY+CBOX_H)
    return HIT_ACTIVE_COLOR_BOX;
  else if (p.x() >= CBOX_PX && p.y() >= CBOX_PY && p.x() < CBOX_PX+CBOX_W && p.y() < CBOX_PY+CBOX_H)
    return HIT_PASSIVE_COLOR_BOX;

  int nx=p.x()-PAL_X, ny=p.y()-PAL_Y;
  if (nx>=0 && ny>=0)
  {
    nx/=PBOX_W;
    ny/=PBOX_H;
    if (nx<PAL_NX && ny<PAL_NY) return ny*PAL_NX+nx;
  }

  return HIT_NONE;
}


void PaletteWidget::setActiveColor(const Imath::Color4f& color, int index)
{
    int oldIndex=m_activeIndex;
    m_activeColor = color;
    m_activeIndex = index;
    emit activeColorChanged(m_activeColor, m_activeIndex);
    repaint(0, 0, width(), HR_Y);
    repaint(palRect(oldIndex));
    repaint(palRect(m_activeIndex));
}


void PaletteWidget::setPassiveColor(const Imath::Color4f& color, int index)
{
    int oldIndex=m_passiveIndex;
    m_passiveColor = color;
    m_passiveIndex = index;
    //emit activeColorChanged(m_activeColor);
    repaint(0, 0, width(), HR_Y);
    repaint(palRect(oldIndex));
    repaint(palRect(m_passiveIndex));
}


void PaletteWidget::swapColors()
{
    Imath::Color4f copy = m_activeColor; m_activeColor = m_passiveColor; m_passiveColor = copy;
    int            icpy = m_activeIndex; m_activeIndex = m_passiveIndex; m_passiveIndex = icpy;
    emit activeColorChanged(m_activeColor, m_activeIndex);
    repaint(0, 0, width(), HR_Y);
    repaint(palRect(m_activeIndex));
    repaint(palRect(m_passiveIndex));
}

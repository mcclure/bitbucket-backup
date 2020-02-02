#ifndef __PALETTE_WIDGET_H__
#define __PALETTE_WIDGET_H__

#include <QColor>
#include <QWidget>
#include <QPalette>
#include <ImathColor.h>
#include "VoxelGridGroup.h"
#include "UndoManager.h"


class PaletteWidget : public QWidget
{
    Q_OBJECT

public:
    PaletteWidget(QWidget* parent, UndoManager *um) :
        QWidget(parent),
        p_undoManager(um),
        m_activeColor(1.0f, 1.0f, 1.0f, 1.0f),
        m_passiveColor(0.0f, 0.0f, 0.0f, 1.0f),
        m_activeIndex(-1),
        m_passiveIndex(-1),
        m_hilightIndex(HIT_NONE),
        m_pboxW(8), m_pboxH(8),
        m_palX(0), m_cboxX(0)
        {
            setMouseTracking(true);
            QPalette pal;
            QColor winColor = pal.color(QPalette::Window);
            m_backgroundColor = Imath::Color4f((float)winColor.red()/255.0f,
                                               (float)winColor.green()/255.0f,
                                               (float)winColor.blue()/255.0f, 1.0f);

            connect(p_undoManager, SIGNAL(paletteChanged(ColorPalettePtr)),
              this, SLOT(onPaletteChanged(ColorPalettePtr)));
        }
    ~PaletteWidget() {}

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    ColorPalettePtr getPalette() const { return m_palette; }
    void setPalette(ColorPalettePtr pal);

signals:
    void activeColorChanged(const Imath::Color4f& color, int index);

public slots:
    void setActiveColor (const Imath::Color4f& color, int index);
    void setPassiveColor(const Imath::Color4f& color, int index);
    void swapColors();
    void onPaletteChanged(ColorPalettePtr pal) { if (pal==m_palette) update(); }

protected:
    void paintEvent(QPaintEvent *event);

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void resizeEvent(QResizeEvent *event);

    bool event(QEvent *event);

private:
    Imath::Color4f m_activeColor;
    Imath::Color4f m_passiveColor;
    Imath::Color4f m_backgroundColor;
    int m_activeIndex;
    int m_passiveIndex;

    ColorPalettePtr m_palette;

    int m_hilightIndex;

    UndoManager *p_undoManager;

    int m_palX, m_pboxW, m_pboxH, m_cboxX;

    QRect palRect(int index) const;
    void setHilight(int index);

    QColor toQColor(const Imath::Color4f& in);
    QColor toQColor(const Imath::Color4f& in, float alpha);
    Imath::Color4f toColor4f(QColor c);

    enum HitType
    {
      HIT_NONE              = -1,
      HIT_ACTIVE_COLOR_BOX  = -2,
      HIT_PASSIVE_COLOR_BOX = -3,
    };

    int clickHit(const QPoint& clickSpot);
};

#endif

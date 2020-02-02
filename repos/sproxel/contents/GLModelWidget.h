#ifndef __GL_MODEL_WIDGET_H__
#define __GL_MODEL_WIDGET_H__

#include <vector>

#include <QGLWidget>
#include <QSettings>

#include "Tools.h"
#include "Global.h"
#include "GLCamera.h"
#include "UndoManager.h"
#include "VoxelGridGroup.h"

#include <ImathBox.h>
#include <ImathVec.h>
#include <ImathColor.h>

class GLModelWidget : public QGLWidget
{
    Q_OBJECT

public:
    enum CubeFaceMask { FACE_POSY = 0x01, FACE_NEGY = 0x02,
                        FACE_POSX = 0x04, FACE_NEGX = 0x08,
                        FACE_POSZ = 0x10, FACE_NEGZ = 0x20,
                        FACE_NONE = 0x00, FACE_ALL  = 0x3f };

public:
    GLModelWidget(QWidget* parent, QSettings* appSettings, UndoManager *undoManager, VoxelGridGroupPtr sprite);
    ~GLModelWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    VoxelGridGroupPtr getSprite() const { return m_gvg; }

    void saveSettings();

public:
    void frame(bool fullExtents);
    void handleArrows(QKeyEvent *event);

    void resizeAndClearVoxelGrid(const Imath::V3i& size);
    void resizeAndShiftVoxelGrid(const Imath::V3i& size, const Imath::V3i& shift);
    void reresVoxelGrid(const float scale);

    void shiftVoxels(const SproxelAxis axis, const bool up, const bool wrap);
    void mirrorVoxels(const SproxelAxis axis);
    void rotateVoxels(const SproxelAxis axis, const int dir);
    void setVoxelColor(const Imath::V3i& index, const Imath::Color4f color, int ind);

    // Accessors
    const Imath::V3i& activeVoxel() const { return m_activeVoxel; }
    const Imath::Color4f& activeColor() const { return m_activeColor; }
    int activeIndex() const { return m_activeIndex; }
    bool drawGrid() const { return m_drawGrid; }
    bool drawVoxelGrid() const { return m_drawVoxelGrid; }
    bool drawBoundingBox() const { return m_drawBoundingBox; }
    bool drawSpriteBounds() const { return m_drawSpriteBounds; }
    bool shiftWrap() const { return m_shiftWrap; }
    SproxelAxis currentAxis() const { return m_currAxis; }

signals:
    void colorSampled(const Imath::Color4f& color, int index);

public slots:
    void setSprite(VoxelGridGroupPtr sprite);
    void setActiveTool(const SproxelTool tool);
    void setDrawGrid(const bool value) { m_drawGrid = value; updateGL(); }
    void setDrawVoxelGrid(const bool value) { m_drawVoxelGrid = value; updateGL(); }
    void setDrawBoundingBox(const bool value) { m_drawBoundingBox = value; updateGL(); }
    void setDrawSpriteBounds(const bool value) { m_drawSpriteBounds = value; updateGL(); }
    void setShiftWrap(const bool value) { m_shiftWrap = value; }
    void setCurrentAxis(const SproxelAxis val) { m_currAxis = val; updateGL(); }    // TODO: Change tool as well.
    void setActiveColor(const Imath::Color4f& c, int i) { m_activeColor = c; m_activeIndex=i; }
    void onSpriteChanged(VoxelGridGroupPtr spr) { if (spr==m_gvg) update(); }
    void onPaletteChanged(ColorPalettePtr pal) { if (m_gvg && m_gvg->hasPalette(pal)) update(); }
    void frameFull() { frame(true); }
    void frameData() { frame(false); }

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);

private:
    GLCamera m_cam;
    double m_cameraSnapStep;
    Imath::V2d m_cameraSnapDelta;

    VoxelGridGroupPtr m_gvg;
    std::vector<Imath::V3i> m_previews;

    UndoManager *p_undoManager;

    Imath::V3i m_activeVoxel;
    Imath::Color4f m_activeColor;
    int m_activeIndex;

    QPoint m_lastMouse;
    bool m_drawGrid;
    bool m_drawVoxelGrid;
    bool m_drawBoundingBox;
    bool m_drawSpriteBounds;
    bool m_shiftWrap;

    SproxelAxis m_currAxis;
    ToolState* m_activeTool;

    double* glMatrix(const Imath::M44d& m);
    void objWritePoly(FILE* fp, bool asTriangles,
                      const int& v0, const int& v1, const int& v2, const int& v3);

    Imath::Box3d dataBounds();
    void centerGrid();

    CubeFaceMask computeVoxelFaceMask(const Imath::V3i& index);

    void glDrawAxes();
    void glDrawGrid(const int size,
                    const int gridCellSize,
                    const Imath::Color4f& gridColor,
                    const Imath::Color4f& bgColor);

    void glDrawCubeWire();
    void glDrawCubePoly(const CubeFaceMask mask, bool smooth);

    void glDrawVoxelGrid();
    void glDrawActiveVoxel();
    void glDrawPreviewVoxels();
    void glDrawVoxelCenter(const size_t sx, const size_t sy, const size_t sz);

    QSettings* p_appSettings;
};


#endif

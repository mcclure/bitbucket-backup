#include <QtGui>
#include <QtOpenGL>

#include <map>
#include <cmath>
#include <iostream>
#include <algorithm>

#include <ImathLine.h>
#include <ImathBoxAlgo.h>

#include "GLModelWidget.h"

#define DEBUG_ME (0)

Imath::Box3d fakeBounds(Imath::V3d(-50, -50, -50), Imath::V3d(50, 50, 50));

GLModelWidget::GLModelWidget(QWidget* parent, QSettings* appSettings, UndoManager *undoManager, VoxelGridGroupPtr sprite)
    : QGLWidget(parent),
      m_cam(),
      m_cameraSnapStep(45.0),
      m_cameraSnapDelta(m_cameraSnapStep/2.0, m_cameraSnapStep/2.0),
      m_gvg(sprite),
      m_previews(),
      p_undoManager(undoManager),
      m_activeVoxel(-1,-1,-1),
      m_activeColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_activeIndex(-1),
      m_lastMouse(),
      m_drawGrid(true),
      m_drawVoxelGrid(true),
      m_drawBoundingBox(false),
      m_drawSpriteBounds(true),
      m_shiftWrap(true),
      m_currAxis(Y_AXIS),
      m_activeTool(NULL),
      p_appSettings(appSettings)
{
    m_drawGrid=p_appSettings->value("GLModelWidget/drawGrid", true).toBool();
    m_drawVoxelGrid=p_appSettings->value("GLModelWidget/drawVoxelGrid", true).toBool();
    m_drawBoundingBox=p_appSettings->value("GLModelWidget/drawBoundingBox", false).toBool();
    m_drawSpriteBounds=p_appSettings->value("GLModelWidget/drawSpriteBounds", true).toBool();

    // Default empty grid
    //centerGrid();

    // Always shoot rays through the scene - even when a mouse button isn't pressed
    setMouseTracking(true);

    connect(p_undoManager, SIGNAL(spriteChanged(VoxelGridGroupPtr)),
      this, SLOT(onSpriteChanged(VoxelGridGroupPtr)));

    connect(p_undoManager, SIGNAL(paletteChanged(ColorPalettePtr)),
      this, SLOT(onPaletteChanged(ColorPalettePtr)));
}


GLModelWidget::~GLModelWidget()
{
    //makeCurrent();
    //glDeleteLists(object, 1);

    delete m_activeTool;
}


void GLModelWidget::saveSettings()
{
  p_appSettings->setValue("GLModelWidget/drawGrid", m_drawGrid);
  p_appSettings->setValue("GLModelWidget/drawVoxelGrid", m_drawVoxelGrid);
  p_appSettings->setValue("GLModelWidget/drawBoundingBox", m_drawBoundingBox);
  p_appSettings->setValue("GLModelWidget/drawSpriteBounds", m_drawSpriteBounds);
}


void GLModelWidget::setSprite(VoxelGridGroupPtr sprite)
{
  m_gvg=sprite;
  //centerGrid();
  updateGL();
}


void GLModelWidget::resizeAndClearVoxelGrid(const Imath::V3i& size)
{
    VoxelGridLayerPtr layer(new VoxelGridLayer());
    layer->resize(Imath::Box3i(Imath::V3i(0), size-Imath::V3i(1)));
    layer->setName("main layer");

    m_gvg->clear();
    m_gvg->insertLayerAbove(0, layer);

    centerGrid();
    updateGL();
}


void GLModelWidget::resizeAndShiftVoxelGrid(const Imath::V3i& sizeInc,
                                            const Imath::V3i& shift)
{
    Imath::Box3i box=m_gvg->bounds();
    Imath::V3i newSize = box.size() + Imath::V3i(1) + sizeInc;

    //== FIXME: current implementation collapses all layers, should be per-layer operation

    VoxelGridGroupPtr newGridPtr(new VoxelGridGroup(newSize, ColorPalettePtr()));
    VoxelGridGroup &newGrid = *newGridPtr;

    for (int x = box.min.x; x <= box.max.x; x++)
    {
        const int xDest = x + shift.x;
        for (int y = box.min.y; y <= box.max.y; y++)
        {
            const int yDest = y + shift.y;
            for (int z = box.min.z; z <= box.max.z; z++)
            {
                const int zDest = z + shift.z;
                if ((xDest < 0 || xDest >= newSize.x) ||
                    (yDest < 0 || yDest >= newSize.y) ||
                    (zDest < 0 || zDest >= newSize.z))
                    continue;

                newGrid.set(Imath::V3i(xDest, yDest, zDest),
                            m_gvg->get(Imath::V3i(x, y, z)));
            }
        }
    }

    p_undoManager->changeEntireVoxelGrid(m_gvg, newGridPtr);
    centerGrid();
    updateGL();
}


void GLModelWidget::reresVoxelGrid(const float scale)
{
    //== FIXME: make it work with layers

    Imath::V3i newDims;
    const Imath::V3i oldDims = m_gvg->bounds().size()+Imath::V3i(1);
    newDims.x = (int)((float)oldDims.x * scale);
    newDims.y = (int)((float)oldDims.y * scale);
    newDims.z = (int)((float)oldDims.z * scale);
    if (newDims.x <= 0 && newDims.y <= 0 && newDims.z <= 0)
        return;

    VoxelGridGroupPtr newGridPtr(new VoxelGridGroup(newDims, ColorPalettePtr()));
    VoxelGridGroup &newGrid = *newGridPtr;
    newGrid.setTransform(m_gvg->transform());


    Imath::M44d transform;
    Imath::V3d oldTranslation = m_gvg->transform().translation();
    transform[3][0] = oldTranslation.x * scale;
    transform[3][1] = oldTranslation.y * scale;
    transform[3][2] = oldTranslation.z * scale;
    newGrid.setTransform(transform);
    // TODO: Maybe scaling the world or the gvg would be fun

    // Move the stuff over, "changing resolution"
    // TODO: Make this less naieve.  Right now, when down-res'ing, you get a filled
    //       voxel if there is *anything* in its up-res'ed version.
    //       Could also take neighbors into account (round corners) when up-res'ing, etc.
    for (int x = 0; x < oldDims.x; x++)
    {
        for (int y = 0; y < oldDims.y; y++)
        {
            for (int z = 0; z < oldDims.z; z++)
            {
                const Imath::Color4f curCol = m_gvg->get(Imath::V3i(x,y,z));
                if (curCol.a != 0.0f)
                {
                    for (int xx = 0; xx < scale; xx++)
                    {
                        for (int yy = 0; yy < scale; yy++)
                        {
                            for (int zz = 0; zz < scale; zz++)
                            {
                                const Imath::V3i lowIndex((int)((float)x*scale)+xx,
                                                          (int)((float)y*scale)+yy,
                                                          (int)((float)z*scale)+zz);
                                newGrid.set(lowIndex, curCol);
                            }
                        }
                    }
                }
            }
        }
    }

    p_undoManager->changeEntireVoxelGrid(m_gvg, newGridPtr);
    updateGL();
}


QSize GLModelWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}


QSize GLModelWidget::sizeHint() const
{
    return QSize(400, 400);
}


void GLModelWidget::setActiveTool(const SproxelTool tool)
{
    delete m_activeTool;
    switch (tool)
    {
        case TOOL_RAY: break;   // TODO: Add icon and implement!
        case TOOL_SPLAT: m_activeTool = new SplatToolState(p_undoManager); break;
        case TOOL_FLOOD: m_activeTool = new FloodToolState(p_undoManager); break;
        case TOOL_DROPPER: m_activeTool = new DropperToolState(p_undoManager); break;
        case TOOL_ERASER: m_activeTool = new EraserToolState(p_undoManager); break;
        case TOOL_REPLACE: m_activeTool = new ReplaceToolState(p_undoManager); break;
        case TOOL_SLAB: m_activeTool = new SlabToolState(p_undoManager); break;
        case TOOL_LINE: m_activeTool = new LineToolState(p_undoManager); break;
    }
}


void GLModelWidget::initializeGL()
{
    QColor bg = p_appSettings->value("GLModelWidget/backgroundColor", QColor(161,161,161)).value<QColor>();
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 0.0);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);     // Not sure if i should let these go or not.

    m_cam.setSize(400, 400);
    m_cam.lookAt(Imath::V3d(28, 21, 28), Imath::V3d(0.0, 0.0, 0.0));
    m_cam.setFovy(37.849289);
}


void GLModelWidget::resizeGL(int width, int height)
{
    m_cam.setSize(width, height);
    m_cam.autoSetClippingPlanes(fakeBounds);
}


void GLModelWidget::paintGL()
{
    QColor bg = p_appSettings->value("GLModelWidget/backgroundColor", QColor(161,161,161)).value<QColor>();
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 0.0);

    m_cam.apply();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_drawGrid)
    {
        // Shift the grid to the floor of the voxel grid
        // TODO: Eventually stop moving this to the floor and just keep it at 0,0,0
        Imath::Box3d worldBox = m_gvg->worldBounds();
        glPushMatrix();
        glTranslatef(0, worldBox.min.y, 0);

        // Grid drawing with color conversion
        QColor tempG  = p_appSettings->value("GLModelWidget/gridColor", QColor(0,0,0)).value<QColor>();
        QColor tempBG = p_appSettings->value("GLModelWidget/backgroundColor", QColor(161,161,161)).value<QColor>();
        glDrawGrid(p_appSettings->value("GLModelWidget/gridSize", 16).toInt(),
                   p_appSettings->value("GLModelWidget/gridCellSize", 1).toInt(),
                   Imath::Color4f(tempG.redF(),  tempG.greenF(),  tempG.blueF(),  1.0f),
                   Imath::Color4f(tempBG.redF(), tempBG.greenF(), tempBG.blueF(), 1.0f));
        glPopMatrix();
    }

    if (m_drawSpriteBounds)
    {
      Imath::Box3d box=m_gvg->worldBounds();

      if (!box.isEmpty())
      {
        const Imath::V3d &min=box.min;
        const Imath::V3d &max=box.max;

        QColor tempG = p_appSettings->value("GLModelWidget/gridColor", QColor(0,0,0)).value<QColor>();
        glColor3f(tempG.redF(), tempG.greenF(), tempG.blueF());
        glLineWidth(2);
        glDisable(GL_DEPTH_TEST);

        glBegin(GL_LINE_LOOP);
        glVertex3f(min.x, min.y, min.z);
        glVertex3f(max.x, min.y, min.z);
        glVertex3f(max.x, min.y, max.z);
        glVertex3f(min.x, min.y, max.z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        glVertex3f(min.x, max.y, min.z);
        glVertex3f(max.x, max.y, min.z);
        glVertex3f(max.x, max.y, max.z);
        glVertex3f(min.x, max.y, max.z);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(min.x, min.y, min.z);
        glVertex3f(min.x, max.y, min.z);
        glVertex3f(max.x, min.y, min.z);
        glVertex3f(max.x, max.y, min.z);
        glVertex3f(min.x, min.y, max.z);
        glVertex3f(min.x, max.y, max.z);
        glVertex3f(max.x, min.y, max.z);
        glVertex3f(max.x, max.y, max.z);
        glEnd();

        glLineWidth(1);
        glEnable(GL_DEPTH_TEST);
      }
    }

    glDrawAxes();


    // Draw colored centers
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHT0);

    GLfloat ambient[] = {0.0, 0.0, 0.0, 1.0};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    Imath::V3f camPos = m_cam.translation();
    GLfloat lightDir[4];
    lightDir[0] = camPos.x;
    lightDir[1] = camPos.y;
    lightDir[2] = camPos.z;
    lightDir[3] = 0.0; // w=0.0 means directional

    glLightfv(GL_LIGHT0, GL_POSITION, lightDir);

    GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    bool drawOutlines=p_appSettings->value("GLModelWidget/drawVoxelOutlines", 0).toBool();
    bool drawSmoothCubes=p_appSettings->value("GLModelWidget/drawSmoothVoxels", 0).toBool();

    const Imath::Box3i dim = m_gvg->bounds();
    for (int x = dim.min.x; x <= dim.max.x; x++)
    {
        for (int y = dim.min.y; y <= dim.max.y; y++)
        {
            for (int z = dim.min.z; z <= dim.max.z; z++)
            {
                const Imath::V3i index = Imath::V3i(x,y,z);
                Imath::Color4f cellColor = m_gvg->get(index);
                const Imath::M44d mat = m_gvg->voxelTransform(index);

                if (cellColor.a != 0.0)
                {
                    glColor4f(cellColor.r, cellColor.g, cellColor.b, 0.2f);

                    glPushMatrix();
                    glMultMatrixd(glMatrix(mat));

                    CubeFaceMask mask = computeVoxelFaceMask(index);

                    if (drawOutlines)
                    {
                        // TODO: Make line width a setting
                        // TODO: Learn how to fix these polygon offset values to work properly.
                        glEnable(GL_POLYGON_OFFSET_FILL);
                        glPolygonOffset(1.0, 1.0);
                        glDrawCubePoly(mask, drawSmoothCubes);
                        glDisable(GL_POLYGON_OFFSET_FILL);

                        //glLineWidth(1.5);
                        glDisable(GL_LIGHTING);
                        glEnable(GL_POLYGON_OFFSET_LINE);
                        glPolygonOffset(1.0, -5.0);
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        glColor3f(1.0f - cellColor.r,
                                  1.0f - cellColor.g,
                                  1.0f - cellColor.b);
                        glDrawCubePoly(mask, false);
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        glDisable(GL_POLYGON_OFFSET_LINE);
                        glEnable(GL_LIGHTING);
                        //glLineWidth(1.0);
                    }
                    else
                    {
                        glDrawCubePoly(mask, drawSmoothCubes);
                    }

                    glPopMatrix();
                }
            }
        }
    }
    glDisable(GL_LIGHT0);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);

    //glDisable(GL_BLEND);


    // DRAW UNPROJECTED LINE
    if (DEBUG_ME)
    {
        const Imath::Line3d& lastRay = m_activeTool->ray();
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
        glBegin(GL_LINES);
        glVertex3d(lastRay.pos.x, lastRay.pos.y, lastRay.pos.z);
        glVertex3f(lastRay.pos.x + lastRay.dir.x * 100.0,
                   lastRay.pos.y + lastRay.dir.y * 100.0,
                   lastRay.pos.z + lastRay.dir.z * 100.0);
        glEnd();
    }

    // Grid stuff
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_drawVoxelGrid)
    {
        glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
        glDrawVoxelGrid();
    }

    if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
    {
        glColor4f(1.0f, 0.0f, 0.0f, 0.2f);
        glDrawPreviewVoxels();
    }

    if (m_activeVoxel != Imath::V3i(-1,-1,-1))
    {
        glColor4f(0.0f, 0.0f, 1.0f, 0.2f);
        glDrawActiveVoxel();
    }

    glDisable(GL_BLEND);

    // Draw text stuff
    QFont font;
    font.setPointSize(10);
    glColor3f(1.0f, 1.0f, 1.0f);
    const char *sliceName[3] = { "Axis X, Slice YZ",
                                 "Axis Y, Slice XZ",
                                 "Axis Z, Slice XY" };
    renderText(10, 20, QString(sliceName[m_currAxis]), font);
    //renderText(10, 32, QString("%1, %2, %3")
    //                   .arg(m_activeVoxel.x)
    //                   .arg(m_activeVoxel.y)
    //                   .arg(m_activeVoxel.z));

    // DRAW BOUNDING BOX
    if (m_drawBoundingBox)
    {
        Imath::Box3d ext = dataBounds();
        Imath::V3d& min = ext.min;
        Imath::V3d& max = ext.max;

        if (!ext.isEmpty())
        {
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
            glVertex3f(min.x, min.y, min.z);
            glVertex3f(max.x, min.y, min.z);
            glVertex3f(max.x, min.y, max.z);
            glVertex3f(min.x, min.y, max.z);
            glEnd();

            glBegin(GL_LINE_LOOP);
            glVertex3f(min.x, max.y, min.z);
            glVertex3f(max.x, max.y, min.z);
            glVertex3f(max.x, max.y, max.z);
            glVertex3f(min.x, max.y, max.z);
            glEnd();

            glBegin(GL_LINES);
            glVertex3f(min.x, min.y, min.z);
            glVertex3f(min.x, max.y, min.z);
            glVertex3f(max.x, min.y, min.z);
            glVertex3f(max.x, max.y, min.z);
            glVertex3f(min.x, min.y, max.z);
            glVertex3f(min.x, max.y, max.z);
            glVertex3f(max.x, min.y, max.z);
            glVertex3f(max.x, max.y, max.z);
            glEnd();
        }
    }

    glLoadIdentity();
}


// Transform an Imath::M44d into a matrix usable by OpenGL.
double* GLModelWidget::glMatrix(const Imath::M44d& m)
{
    return (double*)(&m);
}


void GLModelWidget::glDrawGrid(const int size,
                               const int gridCellSize,
                               const Imath::Color4f& gridColor,
                               const Imath::Color4f& bgColor)
{
    // TODO: Query and restore depth test
    glDisable(GL_DEPTH_TEST);

    // Lighter grid lines
    const Imath::Color4f lightColor = ((bgColor - gridColor) * 0.80f) + gridColor;

    Imath::Box3i ext=m_gvg->bounds();
    if (ext.isEmpty() || !m_drawSpriteBounds)
    {
      ext=Imath::Box3i(Imath::V3i(-size), Imath::V3i(size));
    }
    else
    {
      ext.min-=Imath::V3i(1);
      ext.max+=Imath::V3i(2);
    }

    glBegin(GL_LINES);
    glColor4f(lightColor.r, lightColor.g, lightColor.b, 1.0f);
    for (int i = ext.min.x; i <= ext.max.x; i++)
    {
        if (i == 0) continue;
        if (i % gridCellSize) continue;
        glVertex3f(i, 0, ext.max.z);
        glVertex3f(i, 0, ext.min.z);
    }
    for (int i = ext.min.z; i <= ext.max.z; i++)
    {
        if (i == 0) continue;
        if (i % gridCellSize) continue;
        glVertex3f(ext.max.x, 0, i);
        glVertex3f(ext.min.x, 0, i);
    }
    glEnd();

    // Darker main lines
    // TODO: Query and restore line width
    glLineWidth(2);
    glBegin(GL_LINES);
    glColor4f(gridColor.r, gridColor.g, gridColor.b, 1.0f);
    glVertex3f(ext.min.x, 0, 0);
    glVertex3f(ext.max.x, 0, 0);
    glVertex3f(0, 0, ext.min.z);
    glVertex3f(0, 0, ext.max.z);
    glEnd();
    glLineWidth(1);

    glEnable(GL_DEPTH_TEST);
}


GLModelWidget::CubeFaceMask GLModelWidget::computeVoxelFaceMask(const Imath::V3i& index)
{
    CubeFaceMask returnMask = FACE_NONE;

    const Imath::Color4f& cellColor = m_gvg->get(index);
    if (cellColor.a == 0.0f)
        return returnMask;

    // Positive
    if (m_gvg->get(Imath::V3i(index.x+1, index.y, index.z)).a == 0.0)
        returnMask = (CubeFaceMask)(returnMask | FACE_POSX);

    if (m_gvg->get(Imath::V3i(index.x, index.y+1, index.z)).a == 0.0)
        returnMask = (CubeFaceMask)(returnMask | FACE_POSY);

    if (m_gvg->get(Imath::V3i(index.x, index.y, index.z+1)).a == 0.0)
        returnMask = (CubeFaceMask)(returnMask | FACE_POSZ);

    // Negative
    if (m_gvg->get(Imath::V3i(index.x-1, index.y, index.z)).a == 0.0)
        returnMask = (CubeFaceMask)(returnMask | FACE_NEGX);

    if (m_gvg->get(Imath::V3i(index.x, index.y-1, index.z)).a == 0.0)
        returnMask = (CubeFaceMask)(returnMask | FACE_NEGY);

    if (m_gvg->get(Imath::V3i(index.x, index.y, index.z-1)).a == 0.0)
        returnMask = (CubeFaceMask)(returnMask | FACE_NEGZ);

    return returnMask;
}


void GLModelWidget::glDrawAxes()
{
    // A little heavy-handed, but it gets the job done.
    GLCamera localCam;
    localCam.setSize(50, 50);
    localCam.setFovy(15);
    localCam.setRotation(m_cam.rotation());
    const Imath::V3d distance = (m_cam.translation() - m_cam.pointOfInterest()).normalized();
    localCam.setTranslation(distance*15.0);
    localCam.setCenterOfInterest(m_cam.centerOfInterest());

    // Set the new camera
    glLoadIdentity();
    localCam.apply();

    // Draw the axes
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();
    glEnable(GL_DEPTH_TEST);

    // Restore old camera
    glLoadIdentity();
    m_cam.apply();
}


void GLModelWidget::glDrawCubeWire()
{
    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f( 0.5, -0.5, -0.5);
    glVertex3f( 0.5, -0.5,  0.5);
    glVertex3f(-0.5, -0.5,  0.5);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.5,  0.5, -0.5);
    glVertex3f( 0.5,  0.5, -0.5);
    glVertex3f( 0.5,  0.5,  0.5);
    glVertex3f(-0.5,  0.5,  0.5);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f(-0.5,  0.5, -0.5);
    glVertex3f( 0.5, -0.5, -0.5);
    glVertex3f( 0.5,  0.5, -0.5);
    glVertex3f( 0.5, -0.5,  0.5);
    glVertex3f( 0.5,  0.5,  0.5);
    glVertex3f(-0.5, -0.5,  0.5);
    glVertex3f(-0.5,  0.5,  0.5);
    glEnd();
}


void GLModelWidget::glDrawCubePoly(const CubeFaceMask mask, bool smooth)
{
  glBegin(GL_QUADS);

  if (smooth)
  {
    float ns=0.07f, nf=sqrtf(1.0f-ns*ns*2);

    if (mask & FACE_POSY)
    {
      glNormal3f(   ns,   nf,-  ns);
      glVertex3f( 0.5f, 0.5f,-0.5f);
      glNormal3f(-  ns,   nf,-  ns);
      glVertex3f(-0.5f, 0.5f,-0.5f);
      glNormal3f(-  ns,   nf,   ns);
      glVertex3f(-0.5f, 0.5f, 0.5f);
      glNormal3f(   ns,   nf,   ns);
      glVertex3f( 0.5f, 0.5f, 0.5f);
    }

    if (mask & FACE_NEGY)
    {
      glNormal3f(   ns,-  nf,   ns);
      glVertex3f( 0.5f,-0.5f, 0.5f);
      glNormal3f(-  ns,-  nf,   ns);
      glVertex3f(-0.5f,-0.5f, 0.5f);
      glNormal3f(-  ns,-  nf,-  ns);
      glVertex3f(-0.5f,-0.5f,-0.5f);
      glNormal3f(   ns,-  nf,-  ns);
      glVertex3f( 0.5f,-0.5f,-0.5f);
    }

    if (mask & FACE_POSZ)
    {
      glNormal3f(   ns,   ns,   nf);
      glVertex3f( 0.5f, 0.5f, 0.5f);
      glNormal3f(-  ns,   ns,   nf);
      glVertex3f(-0.5f, 0.5f, 0.5f);
      glNormal3f(-  ns,-  ns,   nf);
      glVertex3f(-0.5f,-0.5f, 0.5f);
      glNormal3f(   ns,-  ns,   nf);
      glVertex3f( 0.5f,-0.5f, 0.5f);
    }

    if (mask & FACE_NEGZ)
    {
      glNormal3f(   ns,-  ns,-  nf);
      glVertex3f( 0.5f,-0.5f,-0.5f);
      glNormal3f(-  ns,-  ns,-  nf);
      glVertex3f(-0.5f,-0.5f,-0.5f);
      glNormal3f(-  ns,   ns,-  nf);
      glVertex3f(-0.5f, 0.5f,-0.5f);
      glNormal3f(   ns,   ns,-  nf);
      glVertex3f( 0.5f, 0.5f,-0.5f);
    }

    if (mask & FACE_POSX)
    {
      glNormal3f(   nf,   ns,-  ns);
      glVertex3f( 0.5f, 0.5f,-0.5f);
      glNormal3f(   nf,   ns,   ns);
      glVertex3f( 0.5f, 0.5f, 0.5f);
      glNormal3f(   nf,-  ns,   ns);
      glVertex3f( 0.5f,-0.5f, 0.5f);
      glNormal3f(   nf,-  ns,-  ns);
      glVertex3f( 0.5f,-0.5f,-0.5f);
    }

    if (mask & FACE_NEGX)
    {
      glNormal3f(-  nf,   ns,   ns);
      glVertex3f(-0.5f, 0.5f, 0.5f);
      glNormal3f(-  nf,   ns,-  ns);
      glVertex3f(-0.5f, 0.5f,-0.5f);
      glNormal3f(-  nf,-  ns,-  ns);
      glVertex3f(-0.5f,-0.5f,-0.5f);
      glNormal3f(-  nf,-  ns,   ns);
      glVertex3f(-0.5f,-0.5f, 0.5f);
    }
  }
  else
  {
    if (mask & FACE_POSY)
    {
      glNormal3f( 0.0f, 1.0f, 0.0f);
      glVertex3f( 0.5f, 0.5f,-0.5f);
      glVertex3f(-0.5f, 0.5f,-0.5f);
      glVertex3f(-0.5f, 0.5f, 0.5f);
      glVertex3f( 0.5f, 0.5f, 0.5f);
    }

    if (mask & FACE_NEGY)
    {
      glNormal3f( 0.0f,-1.0f, 0.0f);
      glVertex3f( 0.5f,-0.5f, 0.5f);
      glVertex3f(-0.5f,-0.5f, 0.5f);
      glVertex3f(-0.5f,-0.5f,-0.5f);
      glVertex3f( 0.5f,-0.5f,-0.5f);
    }

    if (mask & FACE_POSZ)
    {
      glNormal3f( 0.0f, 0.0f, 1.0f);
      glVertex3f( 0.5f, 0.5f, 0.5f);
      glVertex3f(-0.5f, 0.5f, 0.5f);
      glVertex3f(-0.5f,-0.5f, 0.5f);
      glVertex3f( 0.5f,-0.5f, 0.5f);
    }

    if (mask & FACE_NEGZ)
    {
      glNormal3f( 0.0f, 0.0f,-1.0f);
      glVertex3f( 0.5f,-0.5f,-0.5f);
      glVertex3f(-0.5f,-0.5f,-0.5f);
      glVertex3f(-0.5f, 0.5f,-0.5f);
      glVertex3f( 0.5f, 0.5f,-0.5f);
    }

    if (mask & FACE_POSX)
    {
      glNormal3f( 1.0f, 0.0f, 0.0f);
      glVertex3f( 0.5f, 0.5f,-0.5f);
      glVertex3f( 0.5f, 0.5f, 0.5f);
      glVertex3f( 0.5f,-0.5f, 0.5f);
      glVertex3f( 0.5f,-0.5f,-0.5f);
    }

    if (mask & FACE_NEGX)
    {
      glNormal3f(-1.0f, 0.0f, 0.0f);
      glVertex3f(-0.5f, 0.5f, 0.5f);
      glVertex3f(-0.5f, 0.5f,-0.5f);
      glVertex3f(-0.5f,-0.5f,-0.5f);
      glVertex3f(-0.5f,-0.5f, 0.5f);
    }
  }

  glEnd();
}


void GLModelWidget::glDrawVoxelGrid()
{
    const Imath::Box3i dim = m_gvg->bounds();

    // TODO: Optimize the intersects stuff (or just ignore it)
    for (int x = dim.min.x; x <= dim.max.x; x++)
    {
        for (int y = dim.min.y; y <= dim.max.y+1; y++)
        {
            for (int z = dim.min.z; z <= dim.max.z+1; z++)
            {
                if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
                {
                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y-1,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z-1)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y-1,z-1)) != m_previews.end())
                        continue;
                }

                if (Imath::V3i(x,y,z) == m_activeVoxel   || Imath::V3i(x,y-1,z) == m_activeVoxel ||
                    Imath::V3i(x,y,z-1) == m_activeVoxel || Imath::V3i(x,y-1,z-1) == m_activeVoxel)
                {
                    continue;
                }

                const Imath::M44d mat = m_gvg->voxelTransform(Imath::V3i(x,y,z));

                glPushMatrix();
                glMultMatrixd(glMatrix(mat));

                glBegin(GL_LINES);
                glVertex3f(-0.5, -0.5, -0.5);
                glVertex3f( 0.5, -0.5, -0.5);
                glEnd();

                glPopMatrix();
            }
        }
    }

    for (int x = dim.min.x; x <= dim.max.x+1; x++)
    {
        for (int y = dim.min.y; y <= dim.max.y; y++)
        {
            for (int z = dim.min.z; z <= dim.max.z+1; z++)
            {
                if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
                {
                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z-1)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y,z-1)) != m_previews.end())
                        continue;
                }

                if (Imath::V3i(x,y,z) == m_activeVoxel   || Imath::V3i(x-1,y,z) == m_activeVoxel ||
                    Imath::V3i(x,y,z-1) == m_activeVoxel || Imath::V3i(x-1,y,z-1) == m_activeVoxel)
                {
                    continue;
                }

                const Imath::M44d mat = m_gvg->voxelTransform(Imath::V3i(x,y,z));

                glPushMatrix();
                glMultMatrixd(glMatrix(mat));

                glBegin(GL_LINES);
                glVertex3f(-0.5, -0.5, -0.5);
                glVertex3f(-0.5,  0.5, -0.5);
                glEnd();

                glPopMatrix();
            }
        }
    }

    for (int x = dim.min.x; x <= dim.max.x+1; x++)
    {
        for (int y = dim.min.y; y <= dim.max.y+1; y++)
        {
            for (int z = dim.min.z; z <= dim.max.z; z++)
            {
                if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
                {
                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x,y-1,z)) != m_previews.end())
                        continue;

                    if (std::find(m_previews.begin(), m_previews.end(), Imath::V3i(x-1,y-1,z)) != m_previews.end())
                        continue;
                }

                if (Imath::V3i(x,y,z) == m_activeVoxel   || Imath::V3i(x-1,y,z) == m_activeVoxel ||
                    Imath::V3i(x,y-1,z) == m_activeVoxel || Imath::V3i(x-1,y-1,z) == m_activeVoxel)
                {
                    continue;
                }

                const Imath::M44d mat = m_gvg->voxelTransform(Imath::V3i(x,y,z));

                glPushMatrix();
                glMultMatrixd(glMatrix(mat));

                glBegin(GL_LINES);
                glVertex3f(-0.5, -0.5, -0.5);
                glVertex3f(-0.5, -0.5,  0.5);
                glEnd();

                glPopMatrix();
            }
        }
    }
}


void GLModelWidget::glDrawActiveVoxel()
{
    const Imath::M44d mat = m_gvg->voxelTransform(m_activeVoxel);
    glPushMatrix();
    glMultMatrixd(glMatrix(mat));
    glDrawCubeWire();
    glPopMatrix();
}


void GLModelWidget::glDrawPreviewVoxels()
{
    Imath::Color4f curColor;
    glGetFloatv(GL_CURRENT_COLOR, (float*)&curColor);

    for (unsigned int i = 0; i < m_previews.size(); i++)
    {
        float scalar = 1.0f - ((float)i / (float)(m_previews.size()));
        curColor.r = curColor.r * scalar;
        curColor.g = curColor.g * scalar;
        curColor.b = curColor.b * scalar;

        const Imath::M44d mat = m_gvg->voxelTransform(m_previews[i]);
        glPushMatrix();
        glMultMatrixd(glMatrix(mat));
        glColor4f(curColor.r, curColor.g, curColor.b, curColor.a);
        glDrawCubeWire();
        glPopMatrix();
    }
}


void GLModelWidget::glDrawVoxelCenter(const size_t x, const size_t y, const size_t z)
{
    const Imath::V3d location = m_gvg->voxelTransform(Imath::V3i(x,y,z)).translation();

    glPointSize(5);
    glBegin(GL_POINTS);
    glVertex3f(location.x, location.y, location.z);
    glEnd();
    glPointSize(1);
}


void GLModelWidget::mousePressEvent(QMouseEvent *event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool ctrlDown = event->modifiers() & Qt::ControlModifier;
    //const bool shiftDown = event->modifiers() & Qt::ShiftModifier;

    // If you click on the GLModelWidget window, bring it forward.
    setFocus();

    if (altDown)
    {
        m_lastMouse = event->pos();
    }
    else if (ctrlDown)
    {
        if (IsLeftButton(event))
        {
            const Imath::Line3d localLine =
                    m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));

            // TODO: Figure out how to restore old tool properly in ReleaseEvent
            // CTRL+LMB is always replace - switch tools and execute
            SproxelTool currentTool = m_activeTool->type();
            setActiveTool(TOOL_REPLACE);
            m_activeTool->set(m_gvg, localLine, m_activeColor, m_activeIndex);
            m_activeTool->execute();
            setActiveTool(currentTool);
            updateGL();
        }
    }
    else
    {
        Imath::Line3d localLine =
                m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));

        if (IsLeftButton(event))
        {
            bool draggingEnabled = p_appSettings->value("GLModelWidget/dragEnabled", 1).toBool();
            m_activeTool->setDragSupport(draggingEnabled);
            m_activeTool->set(m_gvg, localLine, m_activeColor, m_activeIndex);

            if (m_activeTool->type() == TOOL_SLAB)
                dynamic_cast<SlabToolState*>(m_activeTool)->setAxis(currentAxis());

            else if (m_activeTool->type() == TOOL_DROPPER)
            {
                // TODO: Coalesce this dropper code so i don't repeat it everywhere
                std::vector<Imath::V3i> ints = m_activeTool->voxelsAffected();
                if (ints.size() != 0)
                {
                    Imath::Color4f result = m_gvg->get(ints[0]);
                    emit colorSampled(result, m_gvg->getInd(ints[0]));
                }
                return;
            }

            m_activeTool->execute();
            updateGL();
        }
        else if (IsMidButton(event))
        {
            // Middle button is always the color picker
            // TODO: Restore old tool properly in ReleaseEvent
            SproxelTool currentTool = m_activeTool->type();
            setActiveTool(TOOL_DROPPER);
            m_activeTool->set(m_gvg, localLine, m_activeColor, m_activeIndex);

            // TODO: Coalesce this dropper code so i don't repeat it everywhere
            std::vector<Imath::V3i> ints = m_activeTool->voxelsAffected();
            if (ints.size() != 0)
            {
                Imath::Color4f result = m_gvg->get(ints[0]);
                emit colorSampled(result, m_gvg->getInd(ints[0]));
            }
            setActiveTool(currentTool);
            m_activeTool->setDragSupport(p_appSettings->value("GLModelWidget/dragEnabled", 1).toInt());
        }
        else if (IsRightButton(event))
        {
            // Right button is always delete
            // TODO: Restore old tool properly in ReleaseEvent
            SproxelTool currentTool = m_activeTool->type();
            setActiveTool(TOOL_ERASER);
            m_activeTool->set(m_gvg, localLine, m_activeColor, m_activeIndex);
            m_activeTool->execute();
            setActiveTool(currentTool);
            m_activeTool->setDragSupport(p_appSettings->value("GLModelWidget/dragEnabled", 1).toInt());
            updateGL();
        }
        return;
    }
}


void GLModelWidget::mouseMoveEvent(QMouseEvent *event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool shiftDown = event->modifiers() & Qt::ShiftModifier;

    if (altDown)
    {
        // Camera movement
        const int dx = event->pos().x() - m_lastMouse.x();
        const int dy = event->pos().y() - m_lastMouse.y();
        m_lastMouse = event->pos();

        if ((event->buttons() & (Qt::LeftButton | Qt::MidButton)) == (Qt::LeftButton | Qt::MidButton) ||
            (IsRightButton(event)))
        {
            m_cam.dolly(Imath::V2d(dx, dy));
            m_cam.autoSetClippingPlanes(fakeBounds);
        }
        else if (IsLeftButton(event))
        {
            if (shiftDown)
            {
                // Camera snap
                if (m_cameraSnapDelta.x <= 0)
                {
                    double fm = fmod(m_cam.rotation().y, m_cameraSnapStep);
                    if (fm > 0.0)
                        m_cam.rotateAngle(Imath::V2d(m_cameraSnapStep-fm, 0.0));
                    else if (fm < 0.0)
                        m_cam.rotateAngle(Imath::V2d(-fm, 0.0));
                    else
                        m_cam.rotateAngle(Imath::V2d(45.0, 0.0));
                    m_cameraSnapDelta.x = m_cameraSnapStep/2.0;
                }
                else if (m_cameraSnapDelta.x >= 45.0)
                {
                    double fm = fmod(m_cam.rotation().y, m_cameraSnapStep);
                    if (fm > 0.0)
                        m_cam.rotateAngle(Imath::V2d(-fm, 0.0));
                    else if (fm < 0.0)
                        m_cam.rotateAngle(Imath::V2d(-(m_cameraSnapStep+fm), 0.0));
                    else
                        m_cam.rotateAngle(Imath::V2d(-45.0, 0.0));
                    m_cameraSnapDelta.x = m_cameraSnapStep/2.0;
                }

                if (m_cameraSnapDelta.y <= 0)
                {
                    double fm = fmod(m_cam.rotation().x, m_cameraSnapStep);
                    if (fm > 0.0)
                        m_cam.rotateAngle(Imath::V2d(0.0, m_cameraSnapStep-fm));
                    else if (fm < 0.0)
                        m_cam.rotateAngle(Imath::V2d(0.0, -fm));
                    else
                        m_cam.rotateAngle(Imath::V2d(0.0, 45.0));
                    m_cameraSnapDelta.y = m_cameraSnapStep/2.0;
                }
                else if (m_cameraSnapDelta.y >= 45.0)
                {
                    double fm = fmod(m_cam.rotation().x, m_cameraSnapStep);
                    if (fm > 0.0)
                        m_cam.rotateAngle(Imath::V2d(0.0, -fm));
                    else if (fm < 0.0)
                        m_cam.rotateAngle(Imath::V2d(0.0, -(m_cameraSnapStep+fm)));
                    else
                        m_cam.rotateAngle(Imath::V2d(0.0, -45.0));
                    m_cameraSnapDelta.y = m_cameraSnapStep/2.0;
                }

                m_cameraSnapDelta.x += (double)dx / 4.0;
                m_cameraSnapDelta.y += (double)dy / 4.0;
            }
            else
            {
                // Standard rotation
                m_cam.rotate(Imath::V2d(dx, dy));
                m_cam.autoSetClippingPlanes(fakeBounds);

                // Reset the snap
                m_cameraSnapDelta.x = m_cameraSnapDelta.y = m_cameraSnapStep/2.0;
            }
        }
        else if (IsMidButton(event))
        {
            m_cam.track(Imath::V2d(dx, dy));
            m_cam.autoSetClippingPlanes(fakeBounds);
        }
        updateGL();
    }
    else
    {
        const Imath::Line3d localLine =
                m_cam.unproject(Imath::V2d(event->pos().x(), height() - event->pos().y()));

        m_activeTool->set(m_gvg, localLine, m_activeColor, m_activeIndex);

        // Left click means you're tooling.
        if (IsLeftButton(event))
        {
            // If your active tool supports drag, tool away
            if (m_activeTool->supportsDrag())
            {
                // You want your preview to update even when you're tooling
                m_previews = m_activeTool->voxelsAffected();

                // Tool execution
                m_activeTool->execute();

                // TODO: Coalesce this dropper code so i don't repeat it everywhere
                if (m_activeTool->type() == TOOL_DROPPER)
                {
                    std::vector<Imath::V3i> ints = m_activeTool->voxelsAffected();
                    if (ints.size() != 0)
                    {
                        Imath::Color4f result = m_gvg->get(ints[0]);
                        emit colorSampled(result, m_gvg->getInd(ints[0]));
                    }
                }
                updateGL();
            }
        }
        else
        {
            // Tool preview
            if (p_appSettings->value("GLModelWidget/previewEnabled", 1).toBool())
            {
                if (m_activeTool->type() == TOOL_SLAB)
                    dynamic_cast<SlabToolState*>(m_activeTool)->setAxis(currentAxis());

                m_previews = m_activeTool->voxelsAffected();
                updateGL();
            }
        }
    }
}


void GLModelWidget::mouseReleaseEvent(QMouseEvent*)
{
    m_cameraSnapDelta.x = m_cameraSnapDelta.y = m_cameraSnapStep/2.0;
    //toRight = 0.0;
}

Imath::Box3d GLModelWidget::dataBounds()
{
    Imath::Box3d retBox;

    Imath::Box3i dim=m_gvg->bounds();

    for (int x = dim.min.x; x <= dim.max.x; x++)
    {
        for (int y = dim.min.y; y <= dim.max.y; y++)
        {
            for (int z = dim.min.z; z <= dim.max.z; z++)
            {
                if (m_gvg->get(Imath::V3i(x,y,z)).a != 0.0f)
                {
                    retBox.extendBy(Imath::V3d(x,  y,  z));
                    retBox.extendBy(Imath::V3d(x+1,y+1,z+1));
                }
            }
        }
    }

    // (ImathBoxAlgo) This properly computes the world bounding box
    return Imath::transform(retBox, m_gvg->transform());
}


void GLModelWidget::centerGrid()
{
    Imath::M44d transform;
    Imath::Box3i dim = m_gvg->bounds();
    transform.setTranslation(Imath::V3d(-(dim.min.x+dim.max.x+1)/2.0, 0, -(dim.min.z+dim.max.z+1)/2.0));
    m_gvg->setTransform(transform);
}


void GLModelWidget::frame(bool fullExtents)
{
    // Frame on data extents if they're present.  Otherwise grid world bounds
    Imath::Box3d ext = dataBounds();
    if (ext.isEmpty() || fullExtents)
        ext = m_gvg->worldBounds();

    m_cam.frame(ext);
    m_cam.autoSetClippingPlanes(fakeBounds);
    updateGL();
}


void GLModelWidget::handleArrows(QKeyEvent *event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool ctrlDown = event->modifiers() & Qt::ControlModifier;

    // If you're holding alt, you're moving the camera
    if (altDown)
    {
        // TODO: Movement speed - inverse axes - multiple keys
        if (event->key() == Qt::Key_Left)  m_cam.rotate(Imath::V2d(-19,  0));
        if (event->key() == Qt::Key_Right) m_cam.rotate(Imath::V2d( 19,  0));
        if (event->key() == Qt::Key_Up)    m_cam.rotate(Imath::V2d( 0, -19));
        if (event->key() == Qt::Key_Down)  m_cam.rotate(Imath::V2d( 0,  19));

        updateGL();
        return;
    }


    // If you hit an arrow key and you're invisible, make visible
    if (m_activeVoxel == Imath::V3i(-1,-1,-1))
        m_activeVoxel = Imath::V3i(0,0,0);


    // Which way does camera up go?
    int udInc = 0;
    int* camUD = NULL;
    Imath::V3d camYVec;
    m_cam.transform().multDirMatrix(Imath::V3d(0.0, 1.0, 0.0), camYVec);

    // TODO: Optimize since these are all obvious dot product results
    Imath::V3d objectXVec; m_gvg->transform().multDirMatrix(Imath::V3d(1.0, 0.0, 0.0), objectXVec);
    Imath::V3d objectYVec; m_gvg->transform().multDirMatrix(Imath::V3d(0.0, 1.0, 0.0), objectYVec);
    Imath::V3d objectZVec; m_gvg->transform().multDirMatrix(Imath::V3d(0.0, 0.0, 1.0), objectZVec);

    double xDot = camYVec.dot(objectXVec);
    double yDot = camYVec.dot(objectYVec);
    double zDot = camYVec.dot(objectZVec);

    if (fabs(xDot) > fabs(yDot) && fabs(xDot) > fabs(zDot))
    {
        camUD = &m_activeVoxel.x;
        if (xDot > 0) udInc = 1;
        else          udInc = -1;
    }
    else if (fabs(zDot) > fabs(yDot) && fabs(zDot) > fabs(xDot))
    {
        camUD = &m_activeVoxel.z;
        if (zDot > 0) udInc = 1;
        else          udInc = -1;
    }
    else if (fabs(yDot) > fabs(xDot) && fabs(yDot) > fabs(zDot))
    {
        camUD = &m_activeVoxel.y;
        if (yDot > 0) udInc = 1;
        else          udInc = -1;
    }


    // Which way does camera right go?
    int rlInc = 0;
    int* camRL = NULL;
    Imath::V3d camXVec; m_cam.transform().multDirMatrix(Imath::V3d(1.0, 0.0, 0.0), camXVec);
    xDot = camXVec.dot(objectXVec);
    yDot = camXVec.dot(objectYVec);
    zDot = camXVec.dot(objectZVec);

    if (fabs(xDot) >= fabs(yDot) && fabs(xDot) >= fabs(zDot))
    {
        camRL = &m_activeVoxel.x;
        if (xDot > 0) rlInc = 1;
        else          rlInc = -1;
    }
    else if (fabs(zDot) >= fabs(yDot) && fabs(zDot) >= fabs(xDot))
    {
        camRL = &m_activeVoxel.z;
        if (zDot > 0) rlInc = 1;
        else          rlInc = -1;
    }
    else if (fabs(yDot) >= fabs(xDot) && fabs(yDot) >= fabs(zDot))
    {
        camRL = &m_activeVoxel.y;
        if (yDot > 0) rlInc = 1;
        else          rlInc = -1;
    }


    // Which way does camera depth go?
    int fbInc = 0;
    int* camFB = NULL;
    Imath::V3d camZVec; m_cam.transform().multDirMatrix(Imath::V3d(0.0, 0.0, -1.0), camZVec);
    xDot = camZVec.dot(objectXVec);
    yDot = camZVec.dot(objectYVec);
    zDot = camZVec.dot(objectZVec);

    if (fabs(xDot) >= fabs(yDot) && fabs(xDot) >= fabs(zDot))
    {
        camFB = &m_activeVoxel.x;
        if (xDot > 0) fbInc = 1;
        else          fbInc = -1;
    }
    else if (fabs(zDot) >= fabs(yDot) && fabs(zDot) >= fabs(xDot))
    {
        camFB = &m_activeVoxel.z;
        if (zDot > 0) fbInc = 1;
        else          fbInc = -1;
    }
    else if (fabs(yDot) >= fabs(xDot) && fabs(yDot) >= fabs(zDot))
    {
        camFB = &m_activeVoxel.y;
        if (yDot > 0) fbInc = 1;
        else          fbInc = -1;
    }

    // Apply the results
    switch (event->key())
    {
        case Qt::Key_Left:  *camRL += -rlInc; break;
        case Qt::Key_Right: *camRL +=  rlInc; break;
        case Qt::Key_Up:    if (ctrlDown) *camFB +=  fbInc; else *camUD +=  udInc; break;
        case Qt::Key_Down:  if (ctrlDown) *camFB += -fbInc; else *camUD += -udInc; break;
        default: break;
    }

    // Clamp on the edges
    Imath::Box3i dim=m_gvg->bounds();
    if (m_activeVoxel.x > dim.max.x) m_activeVoxel.x = dim.max.x;
    if (m_activeVoxel.y > dim.max.y) m_activeVoxel.y = dim.max.y;
    if (m_activeVoxel.z > dim.max.z) m_activeVoxel.z = dim.max.z;
    if (m_activeVoxel.x < dim.min.x) m_activeVoxel.x = dim.min.x;
    if (m_activeVoxel.y < dim.min.y) m_activeVoxel.y = dim.min.y;
    if (m_activeVoxel.z < dim.min.z) m_activeVoxel.z = dim.min.z;


    updateGL();
}


void GLModelWidget::shiftVoxels(const SproxelAxis axis, const bool up, const bool wrap)
{
    //== FIXME: current implementation only operates on the current layer
    //          maybe it should work for all the layers at once.  Or be an option.

    // Simplifiers for which way to shift
    size_t tan0AxisDim = 0;
    size_t tan1AxisDim = 0;
    size_t primaryAxisDim = 0;
    switch (axis)
    {
        case X_AXIS: primaryAxisDim = m_gvg->curLayer()->size().x;
                        tan0AxisDim = m_gvg->curLayer()->size().y;
                        tan1AxisDim = m_gvg->curLayer()->size().z; break;
        case Y_AXIS: primaryAxisDim = m_gvg->curLayer()->size().y;
                        tan0AxisDim = m_gvg->curLayer()->size().x;
                        tan1AxisDim = m_gvg->curLayer()->size().z; break;
        case Z_AXIS: primaryAxisDim = m_gvg->curLayer()->size().z;
                        tan0AxisDim = m_gvg->curLayer()->size().x;
                        tan1AxisDim = m_gvg->curLayer()->size().y; break;
    }

    // Simplifiers for wrapping
    size_t backupIndex = 0;
    size_t clearIndex = primaryAxisDim - 1;
    if (up) std::swap(backupIndex, clearIndex);


    p_undoManager->beginMacro("Shift");

    // Backup the necessary slice
    Imath::Color4f* sliceBackup = NULL;
    int* sliceBackupInd = NULL;
    if (wrap)
    {
        sliceBackup    = new Imath::Color4f[tan0AxisDim * tan1AxisDim];
        sliceBackupInd = new            int[tan0AxisDim * tan1AxisDim];
        for (size_t a = 0; a < tan0AxisDim; a++)
        {
            for (size_t b = 0; b < tan1AxisDim; b++)
            {
                Imath::V3i index(-1, -1, -1);
                switch (axis)
                {
                    case X_AXIS: index = Imath::V3i(backupIndex, a, b); break;
                    case Y_AXIS: index = Imath::V3i(a, backupIndex, b); break;
                    case Z_AXIS: index = Imath::V3i(a, b, backupIndex); break;
                }
                sliceBackup   [a + (b * tan0AxisDim)] = m_gvg->get   (index);
                sliceBackupInd[a + (b * tan0AxisDim)] = m_gvg->getInd(index);
            }
        }
    }

    // Shift everyone over
    if (up)
    {
        for (size_t a = backupIndex; a > clearIndex; a--)
        {
            for (size_t b = 0; b < tan0AxisDim; b++)
            {
                for (size_t c = 0; c < tan1AxisDim; c++)
                {
                    Imath::V3i nextIndex(-1, -1, -1);
                    switch (axis)
                    {
                        case X_AXIS: setVoxelColor(Imath::V3i(a, b, c), m_gvg->get(Imath::V3i(a-1, b, c)), m_gvg->getInd(Imath::V3i(a-1, b, c))); break;
                        case Y_AXIS: setVoxelColor(Imath::V3i(b, a, c), m_gvg->get(Imath::V3i(b, a-1, c)), m_gvg->getInd(Imath::V3i(b, a-1, c))); break;
                        case Z_AXIS: setVoxelColor(Imath::V3i(b, c, a), m_gvg->get(Imath::V3i(b, c, a-1)), m_gvg->getInd(Imath::V3i(b, c, a-1))); break;
                    }

                }
            }
        }
    }
    else
    {
        for (size_t a = backupIndex; a < clearIndex; a++)
        {
            for (size_t b = 0; b < tan0AxisDim; b++)
            {
                for (size_t c = 0; c < tan1AxisDim; c++)
                {
                    Imath::V3i nextIndex(-1, -1, -1);
                    switch (axis)
                    {
                        case X_AXIS: setVoxelColor(Imath::V3i(a, b, c), m_gvg->get(Imath::V3i(a+1, b, c)), m_gvg->getInd(Imath::V3i(a+1, b, c))); break;
                        case Y_AXIS: setVoxelColor(Imath::V3i(b, a, c), m_gvg->get(Imath::V3i(b, a+1, c)), m_gvg->getInd(Imath::V3i(b, a+1, c))); break;
                        case Z_AXIS: setVoxelColor(Imath::V3i(b, c, a), m_gvg->get(Imath::V3i(b, c, a+1)), m_gvg->getInd(Imath::V3i(b, c, a+1))); break;
                    }
                }
            }
        }
    }

    // Either clear or set the wrap-to slice
    for (size_t a = 0; a < tan0AxisDim; a++)
    {
        for (size_t b = 0; b < tan1AxisDim; b++)
        {
            Imath::V3i workIndex(-1, -1, -1);
            switch (axis)
            {
                case X_AXIS: workIndex = Imath::V3i(clearIndex, a, b); break;
                case Y_AXIS: workIndex = Imath::V3i(a, clearIndex, b); break;
                case Z_AXIS: workIndex = Imath::V3i(a, b, clearIndex); break;
            }

            if (wrap)
                setVoxelColor(workIndex, sliceBackup[a + (b * tan0AxisDim)], sliceBackupInd[a + (b * tan0AxisDim)]);
            else
                setVoxelColor(workIndex, Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f), 0);
        }
    }

    if (sliceBackup   ) delete[] sliceBackup   ;
    if (sliceBackupInd) delete[] sliceBackupInd;

    p_undoManager->endMacro();
    updateGL();
}


void GLModelWidget::rotateVoxels(const SproxelAxis axis, const int dir)
{
    //== FIXME: current implementation flattens the layers

    // Set some new dimensions
    Imath::V3i newDim(0,0,0);
    Imath::V3i oldDim = m_gvg->bounds().size()+Imath::V3i(1);
    switch (axis)
    {
        case X_AXIS: newDim.x = oldDim.x; newDim.y = oldDim.z; newDim.z = oldDim.y; break;
        case Y_AXIS: newDim.x = oldDim.z; newDim.y = oldDim.y; newDim.z = oldDim.x; break;
        case Z_AXIS: newDim.x = oldDim.y; newDim.y = oldDim.x; newDim.z = oldDim.z; break;
    }

    VoxelGridGroupPtr newGridPtr(new VoxelGridGroup(newDim, ColorPalettePtr()));
    VoxelGridGroup &newGrid = *newGridPtr;
    newGrid.setTransform(m_gvg->transform());

    // Do the rotation
    for (int x = 0; x < newDim.x; x++)
    {
        for (int y = 0; y < newDim.y; y++)
        {
            for (int z = 0; z < newDim.z; z++)
            {
                Imath::V3i oldLocation(0,0,0);
                switch (axis)
                {
                    case X_AXIS: if (dir > 0) oldLocation = Imath::V3i(x, oldDim.y-1-z, y);
                                 else         oldLocation = Imath::V3i(x, z, oldDim.z-1-y);
                                 break;
                    case Y_AXIS: if (dir > 0) oldLocation = Imath::V3i(z, y, oldDim.z-1-x);
                                 else         oldLocation = Imath::V3i(oldDim.x-1-z, y, x);
                                 break;
                    case Z_AXIS: if (dir > 0) oldLocation = Imath::V3i(oldDim.x-1-y, x, z);
                                 else         oldLocation = Imath::V3i(y, oldDim.y-1-x, z);
                                 break;
                }
                newGrid.set(Imath::V3i(x,y,z), m_gvg->get(oldLocation));
            }
        }
    }

    p_undoManager->changeEntireVoxelGrid(m_gvg, newGridPtr);

    centerGrid();
    updateGL();
}


void GLModelWidget::mirrorVoxels(const SproxelAxis axis)
{
    //== FIXME: current implementation collapses all layers, should be per-layer operation
    VoxelGridGroupPtr backup(new VoxelGridGroup(*m_gvg));

    p_undoManager->beginMacro("Mirror");

    Imath::Box3i dim=m_gvg->bounds();

    for (int x = dim.min.x; x <= dim.max.x; x++)
    {
        for (int y = dim.min.y; y <= dim.max.y; y++)
        {
            for (int z = dim.min.z; z <= dim.max.z; z++)
            {
                Imath::V3i oldLocation(-1, -1, -1);
                switch (axis)
                {
                    case X_AXIS: oldLocation = Imath::V3i(dim.max.x+dim.min.x-x, y, z); break;
                    case Y_AXIS: oldLocation = Imath::V3i(x, dim.max.y+dim.min.y-y, z); break;
                    case Z_AXIS: oldLocation = Imath::V3i(x, y, dim.max.z+dim.min.z-z); break;
                }
                setVoxelColor(Imath::V3i(x,y,z), backup->get(oldLocation), backup->getInd(oldLocation));
            }
        }
    }

    p_undoManager->endMacro();
    updateGL();
}


// This is only here for the MainWindow now.  Should be changed.
void GLModelWidget::setVoxelColor(const Imath::V3i& index, const Imath::Color4f color, int ind)
{
    // Validity check
    /*
    const Imath::V3i& cd = m_gvg->cellDimensions();
    if (index.x < 0     || index.y < 0     || index.z < 0 ||
        index.x >= cd.x || index.y >= cd.y || index.z >= cd.z)
        return;
    */

    p_undoManager->setVoxelColor(m_gvg, index, color, ind);
}

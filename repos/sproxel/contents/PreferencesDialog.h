#ifndef _SETTINGS_DIALOG_H_
#define _SETTINGS_DIALOG_H_

#include <iostream>

#include <QDialog>
#include <QPainter>
#include <QSettings>
#include <QListWidget>
#include <QStackedWidget>

class GeneralPage;
class ModelViewPage;
class VoxelPage;
class GridPage;
class LightingPage;
class GuidesPage;
class PalettePage;
class WIPPage;


////////////////////////////////////////////////////////////////////////////////
// Main preferences dialog /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QWidget* parent = 0, QSettings* appSettings = NULL);

public slots:
    void changePage(QListWidgetItem* current, QListWidgetItem* previous);
    void reject();

signals:
    void preferenceChanged();

private:
    QSettings* m_pAppSettings;
    QListWidget* m_pContentsWidget;
    QStackedWidget* m_pPagesWidget;

    // Keep track of all the sub-pages
    GeneralPage* m_pGeneralPage;
    ModelViewPage* m_pModelViewPage;
    VoxelPage* m_pVoxelPage;
    GridPage* m_pGridPage;
    LightingPage* m_pLightingPage;
    GuidesPage* m_pGuidesPage;
    PalettePage* m_pPalettePage;
    WIPPage* m_pWIPPage;
};



////////////////////////////////////////////////////////////////////////////////
// Preference Sub-Pages ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class GeneralPage : public QWidget
{   Q_OBJECT
public:
    GeneralPage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    bool m_saveWindowPositionsOrig;
    bool m_frameOnOpenOrig;
    void restoreOriginals();

signals:
    void preferenceChanged();

public slots:
    void setSaveWindowPositions(int state);
    void setFrameOnOpen(int state);
};


class ModelViewPage : public QWidget
{   Q_OBJECT
public:
    ModelViewPage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    QColor m_backgroundColorOrig;
    bool m_dragEnabledOrig;
    bool m_previewEnabledOrig;
    int m_voxelDisplayOrig;
    void restoreOriginals();

signals:
    void preferenceChanged();

public slots:
    void setBackgroundColor(const QColor& value);
    void setDragEnabled(int value);
    void setPreviewEnabled(int value);
};


class VoxelPage : public QWidget
{   Q_OBJECT
public:
    VoxelPage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    bool m_drawOutlinesOrig, m_drawSmoothOrig;
    void restoreOriginals();

signals:
    void preferenceChanged();

public slots:
    void setDrawOutlines(int value);
    void setDrawSmooth(int value);
};


class GridPage : public QWidget
{   Q_OBJECT
public:
    GridPage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    QColor m_gridColorOrig;
    int m_gridSizeOrig;
    int m_gridCellSizeOrig;
    void restoreOriginals();

signals:
    void preferenceChanged();

public slots:
    void setGridColor(const QColor& value);
    void setGridSize(int value);
    void setgridCellSize(int value);
};


class LightingPage : public QWidget
{   Q_OBJECT
public:
    LightingPage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    void restoreOriginals();
};


class GuidesPage : public QWidget
{   Q_OBJECT
public:
    GuidesPage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    void restoreOriginals();
};


class PalettePage : public QWidget
{   Q_OBJECT
public:
    PalettePage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    void restoreOriginals();
};


class WIPPage : public QWidget
{   Q_OBJECT
public:
    WIPPage(QWidget* parent = NULL, QSettings* appSettings = NULL);

    QSettings* m_pAppSettings;
    void restoreOriginals();
};



////////////////////////////////////////////////////////////////////////////////
// Helper Widgets //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class ColorWidget : public QWidget
{
    Q_OBJECT

public:
    ColorWidget(QWidget* parent = NULL) :
        QWidget(parent),
        m_color(255,0,0)
    {
        setMinimumSize(50, 22);
    }

    void setColor(const QColor& nc) { m_color = nc; }
    QColor color() { return m_color; }

public slots:
    void colorChangedSlot(QColor color);

signals:
    void colorChanged(QColor color);

protected:
    void paintEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);

private:
    QColor m_color;
};

#endif

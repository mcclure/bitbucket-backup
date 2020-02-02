#include <QtGui>
#include <iostream>

#include "PreferencesDialog.h"


////////////////////////////////////////////////////////////////////////////////
// Main preferences dialog /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
PreferencesDialog::PreferencesDialog(QWidget* parent, QSettings* appSettings) :
    QDialog(parent),
    m_pAppSettings(appSettings)
{
    m_pContentsWidget = new QListWidget;
    m_pContentsWidget->setMovement(QListView::Static);
    m_pContentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pContentsWidget->setMinimumWidth(128);
    m_pContentsWidget->setMaximumWidth(128);
    m_pContentsWidget->setCurrentRow(0);

    QListWidgetItem* generalItem = new QListWidgetItem(m_pContentsWidget);
    generalItem->setText(tr("General"));
    generalItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* modelViewItem = new QListWidgetItem(m_pContentsWidget);
    modelViewItem->setText(tr("Model View"));
    modelViewItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* voxelItem = new QListWidgetItem(m_pContentsWidget);
    voxelItem->setText(tr("  Voxel"));
    voxelItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* gridItem = new QListWidgetItem(m_pContentsWidget);
    gridItem->setText(tr("  Grid"));
    gridItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* lightingItem = new QListWidgetItem(m_pContentsWidget);
    lightingItem->setText(tr("  Lighting"));
    lightingItem->setFlags(Qt::ItemIsSelectable);

    QListWidgetItem* guidesItem = new QListWidgetItem(m_pContentsWidget);
    guidesItem->setText(tr("  Image Guides"));
    guidesItem->setFlags(Qt::ItemIsSelectable);

    QListWidgetItem* paletteItem = new QListWidgetItem(m_pContentsWidget);
    paletteItem->setText(tr("Palette"));
    paletteItem->setFlags(Qt::ItemIsSelectable);

    QListWidgetItem* keysItem = new QListWidgetItem(m_pContentsWidget);
    keysItem->setText(tr("Custom Keys"));
    keysItem->setFlags(Qt::ItemIsSelectable);

    m_pContentsWidget->setCurrentItem(generalItem);

    connect(m_pContentsWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));

    m_pPagesWidget = new QStackedWidget;
    m_pGeneralPage = new GeneralPage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pGeneralPage);
    m_pModelViewPage = new ModelViewPage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pModelViewPage);
    m_pVoxelPage = new VoxelPage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pVoxelPage);
    m_pGridPage = new GridPage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pGridPage);
    m_pLightingPage = new LightingPage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pLightingPage);
    m_pGuidesPage = new GuidesPage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pGuidesPage);
    m_pPalettePage = new PalettePage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pPalettePage);
    m_pWIPPage = new WIPPage(this, m_pAppSettings);
    m_pPagesWidget->addWidget(m_pWIPPage);

    QPushButton* cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QPushButton* okButton = new QPushButton(tr("OK"));
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout* horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(m_pContentsWidget);
    horizontalLayout->addWidget(m_pPagesWidget, 1);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Sproxel Settings"));
    resize(QSize(530, 300));

    // Daisy chain the signals
    QObject::connect(m_pGeneralPage,   SIGNAL(preferenceChanged()), this, SIGNAL(preferenceChanged()));
    QObject::connect(m_pModelViewPage, SIGNAL(preferenceChanged()), this, SIGNAL(preferenceChanged()));
    QObject::connect(m_pVoxelPage,     SIGNAL(preferenceChanged()), this, SIGNAL(preferenceChanged()));
    QObject::connect(m_pGridPage,      SIGNAL(preferenceChanged()), this, SIGNAL(preferenceChanged()));
}

void PreferencesDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (!current)
        current = previous;

    m_pPagesWidget->setCurrentIndex(m_pContentsWidget->row(current));
}

void PreferencesDialog::reject()
{
    // Restore all the settings this dialog had to begin with.
    m_pGeneralPage->restoreOriginals();
    m_pModelViewPage->restoreOriginals();
    m_pVoxelPage->restoreOriginals();
    m_pGridPage->restoreOriginals();
    m_pLightingPage->restoreOriginals();
    m_pGuidesPage->restoreOriginals();
    m_pPalettePage->restoreOriginals();
    m_pWIPPage->restoreOriginals();
    done(Rejected);
}



////////////////////////////////////////////////////////////////////////////////
// Preference Sub-Pages ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
GeneralPage::GeneralPage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QCheckBox* saveWindowPositions = new QCheckBox("Save Window Positions On Exit", this);
    QCheckBox* frameOnOpen = new QCheckBox("Frame Model On Open", this);

    QVBoxLayout* stuffzLayout = new QVBoxLayout;
    stuffzLayout->addWidget(saveWindowPositions);
    stuffzLayout->addWidget(frameOnOpen);

    QGroupBox* configGroup = new QGroupBox();
    configGroup->setLayout(stuffzLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // Populate the settings
    if (m_pAppSettings->value("saveUILayout", true).toBool())
        saveWindowPositions->setCheckState(Qt::Checked);
    else
        saveWindowPositions->setCheckState(Qt::Unchecked);
    if (m_pAppSettings->value("frameOnOpen", false).toBool())
        frameOnOpen->setCheckState(Qt::Checked);
    else
        frameOnOpen->setCheckState(Qt::Unchecked);

    // Backup original values
    m_saveWindowPositionsOrig = saveWindowPositions->isChecked();
    m_frameOnOpenOrig = frameOnOpen->isChecked();

    // Hook up the signals
    QObject::connect(saveWindowPositions, SIGNAL(stateChanged(int)),
                     this, SLOT(setSaveWindowPositions(int)));
    QObject::connect(frameOnOpen, SIGNAL(stateChanged(int)),
                     this, SLOT(setFrameOnOpen(int)));
}

void GeneralPage::restoreOriginals()
{
    m_pAppSettings->setValue("saveUILayout", m_saveWindowPositionsOrig);
    m_pAppSettings->setValue("frameOnOpen", m_frameOnOpenOrig);
}

void GeneralPage::setSaveWindowPositions(int state)
{
    m_pAppSettings->setValue("saveUILayout", state);
    emit preferenceChanged();
}

void GeneralPage::setFrameOnOpen(int state)
{
    m_pAppSettings->setValue("frameOnOpen", state);
    emit preferenceChanged();
}


// MODELVIEW PAGE //
ModelViewPage::ModelViewPage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QLabel* backgroundColor = new QLabel("Window Background Color", this);
    ColorWidget* bgColorSelect = new ColorWidget(this);
    QCheckBox* dragEnabled = new QCheckBox("Tool Dragging Enabled", this);
    QCheckBox* previewEnabled = new QCheckBox("Tool Preview Enabled", this);

    QGroupBox* modelViewGroup = new QGroupBox();

    QGridLayout* gridLayout = new QGridLayout;
    gridLayout->addWidget(backgroundColor, 0, 0);
    gridLayout->addWidget(bgColorSelect, 0, 1);
    gridLayout->addWidget(dragEnabled, 2, 0);
    gridLayout->addWidget(previewEnabled, 3, 0);
    modelViewGroup->setLayout(gridLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(modelViewGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // Populate the settings
    bgColorSelect->setColor(m_pAppSettings->value("GLModelWidget/backgroundColor",
                                                  QColor(161,161,161)).value<QColor>());
    if (m_pAppSettings->value("GLModelWidget/dragEnabled", true).toBool())
        dragEnabled->setCheckState(Qt::Checked);
    else
        dragEnabled->setCheckState(Qt::Unchecked);
    if (m_pAppSettings->value("GLModelWidget/previewEnabled", true).toBool())
        previewEnabled->setCheckState(Qt::Checked);
    else
        previewEnabled->setCheckState(Qt::Unchecked);


    // Backup original values
    m_backgroundColorOrig = bgColorSelect->color();
    m_dragEnabledOrig = dragEnabled->isChecked();
    m_previewEnabledOrig = previewEnabled->isChecked();

    // Hook up the signals
    QObject::connect(bgColorSelect, SIGNAL(colorChanged(QColor)),
                     this, SLOT(setBackgroundColor(QColor)));
    QObject::connect(dragEnabled, SIGNAL(stateChanged(int)),
                     this, SLOT(setDragEnabled(int)));
    QObject::connect(previewEnabled, SIGNAL(stateChanged(int)),
                     this, SLOT(setPreviewEnabled(int)));
}

void ModelViewPage::restoreOriginals()
{
    m_pAppSettings->setValue("GLModelWidget/backgroundColor", m_backgroundColorOrig);
    m_pAppSettings->setValue("GLModelWidget/dragEnabled", m_dragEnabledOrig);
    m_pAppSettings->setValue("GLModelWidget/previewEnabled", m_previewEnabledOrig);
}

void ModelViewPage::setBackgroundColor(const QColor& value)
{
    m_pAppSettings->setValue("GLModelWidget/backgroundColor", value);
    emit preferenceChanged();
}

void ModelViewPage::setDragEnabled(int state)
{
    m_pAppSettings->setValue("GLModelWidget/dragEnabled", state);
    emit preferenceChanged();
}

void ModelViewPage::setPreviewEnabled(int state)
{
    m_pAppSettings->setValue("GLModelWidget/previewEnabled", state);
    emit preferenceChanged();
}


// VOXEL PAGE //
VoxelPage::VoxelPage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QCheckBox* drawOutlines = new QCheckBox("Draw Outlines", this);
    QCheckBox* drawSmooth   = new QCheckBox("Draw Smooth Voxels", this);

    QGroupBox* gridGroup = new QGroupBox();

    QGridLayout* gridLayout = new QGridLayout;
    gridLayout->addWidget(drawOutlines, 0, 0);
    gridLayout->addWidget(drawSmooth, 1, 0);
    gridGroup->setLayout(gridLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(gridGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // Populate the settings
    if (m_pAppSettings->value("GLModelWidget/drawVoxelOutlines", false).toBool())
        drawOutlines->setCheckState(Qt::Checked);
    else
        drawOutlines->setCheckState(Qt::Unchecked);

    if (m_pAppSettings->value("GLModelWidget/drawSmoothVoxels", false).toBool())
        drawSmooth->setCheckState(Qt::Checked);
    else
        drawSmooth->setCheckState(Qt::Unchecked);

    // Backup original values
    m_drawOutlinesOrig = drawOutlines->isChecked();
    m_drawSmoothOrig = drawSmooth->isChecked();

    // Hook up the signals
    QObject::connect(drawOutlines, SIGNAL(stateChanged(int)),
                     this, SLOT(setDrawOutlines(int)));
    QObject::connect(drawSmooth, SIGNAL(stateChanged(int)),
                     this, SLOT(setDrawSmooth(int)));
}

void VoxelPage::restoreOriginals()
{
    m_pAppSettings->setValue("GLModelWidget/drawVoxelOutlines", m_drawOutlinesOrig);
    m_pAppSettings->setValue("GLModelWidget/drawSmoothVoxels", m_drawSmoothOrig);
}

void VoxelPage::setDrawOutlines(int value)
{
    m_pAppSettings->setValue("GLModelWidget/drawVoxelOutlines", value);
    emit preferenceChanged();
}

void VoxelPage::setDrawSmooth(int value)
{
    m_pAppSettings->setValue("GLModelWidget/drawSmoothVoxels", value);
    emit preferenceChanged();
}


// GRID PAGE //
GridPage::GridPage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QLabel* gridColor = new QLabel("Grid Color", this);
    ColorWidget* gridColorSelect = new ColorWidget(this);

    QLabel* gridSize = new QLabel("Grid Size", this);
    QSpinBox* sizeSpinBox = new QSpinBox;
    sizeSpinBox->setRange(0, 200);
    sizeSpinBox->setSingleStep(1);
    sizeSpinBox->setValue(16);

    QLabel* gridCellSize = new QLabel("Grid Cell Size", this);
    QSpinBox* cellSizeSpinBox = new QSpinBox;
    cellSizeSpinBox->setRange(1, 200);
    cellSizeSpinBox->setSingleStep(1);
    cellSizeSpinBox->setValue(1);

    QGroupBox* gridGroup = new QGroupBox();

    QGridLayout* gridLayout = new QGridLayout;
    gridLayout->addWidget(gridColor, 0, 0);
    gridLayout->addWidget(gridColorSelect, 0, 1);
    gridLayout->addWidget(gridSize, 1, 0);
    gridLayout->addWidget(sizeSpinBox, 1, 1);
    gridLayout->addWidget(gridCellSize, 2, 0);
    gridLayout->addWidget(cellSizeSpinBox, 2, 1);
    gridGroup->setLayout(gridLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(gridGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // Populate the settings
    gridColorSelect->setColor(m_pAppSettings->value("GLModelWidget/gridColor",
                                                    QColor(0,0,0)).value<QColor>());
    sizeSpinBox->setValue(m_pAppSettings->value("GLModelWidget/gridSize", 16).toInt());
    cellSizeSpinBox->setValue(m_pAppSettings->value("GLModelWidget/gridCellSize", 1).toInt());

    // Backup original values
    m_gridColorOrig = gridColorSelect->color();
    m_gridSizeOrig = sizeSpinBox->value();
    m_gridCellSizeOrig = cellSizeSpinBox->value();

    // Hook up the signals
    QObject::connect(gridColorSelect, SIGNAL(colorChanged(QColor)),
                     this, SLOT(setGridColor(QColor)));
    QObject::connect(sizeSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(setGridSize(int)));
    QObject::connect(cellSizeSpinBox, SIGNAL(valueChanged(int)),
                     this, SLOT(setgridCellSize(int)));
}

void GridPage::restoreOriginals()
{
    m_pAppSettings->setValue("GLModelWidget/gridColor", m_gridColorOrig);
    m_pAppSettings->setValue("GLModelWidget/gridSize", m_gridSizeOrig);
    m_pAppSettings->setValue("GLModelWidget/gridCellSize", m_gridCellSizeOrig);
}

void GridPage::setGridSize(int value)
{
    m_pAppSettings->setValue("GLModelWidget/gridSize", value);
    emit preferenceChanged();
}

void GridPage::setgridCellSize(int value)
{
    m_pAppSettings->setValue("GLModelWidget/gridCellSize", value);
    emit preferenceChanged();
}

void GridPage::setGridColor(const QColor& value)
{
    m_pAppSettings->setValue("GLModelWidget/gridColor", value);
    emit preferenceChanged();
}


// LIGHTING PAGE //
LightingPage::LightingPage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QCheckBox* fixedDir = new QCheckBox("Fixed Light Direction", this);
    QLabel* lightDir = new QLabel("Light Direction", this);
    QLabel* lightColor = new QLabel("Light Color", this);
    QLabel* ambientColor = new QLabel("Ambient Color", this);

    QVBoxLayout* stuffzLayout = new QVBoxLayout;
    stuffzLayout->addWidget(fixedDir);
    stuffzLayout->addWidget(lightDir);
    stuffzLayout->addWidget(lightColor);
    stuffzLayout->addWidget(ambientColor);

    QGroupBox* configGroup = new QGroupBox();
    configGroup->setLayout(stuffzLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // Populate the settings
    // Backup original values
    // Hook up the signals
}

void LightingPage::restoreOriginals()
{

}


// GUIDES PAGE //
GuidesPage::GuidesPage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QGroupBox* guidesGroup = new QGroupBox(tr("Guide Filenames"));

    QLabel* xyLabel = new QLabel(tr("XY Plane:"));
    QLineEdit* xyEdit = new QLineEdit;

    QLabel* xzLabel = new QLabel(tr("XZ Plane:"));
    QLineEdit* xzEdit = new QLineEdit;

    QLabel* yzLabel = new QLabel(tr("YZ Plane:"));
    QLineEdit* yzEdit = new QLineEdit;

    QGridLayout* packagesLayout = new QGridLayout;
    packagesLayout->addWidget(xyLabel, 0, 0);
    packagesLayout->addWidget(xyEdit, 0, 1);
    packagesLayout->addWidget(xzLabel, 1, 0);
    packagesLayout->addWidget(xzEdit, 1, 1);
    packagesLayout->addWidget(yzLabel, 2, 0);
    packagesLayout->addWidget(yzEdit, 2, 1);
    guidesGroup->setLayout(packagesLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(guidesGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // Populate the settings
    // Backup original values
    // Hook up the signals
}

void GuidesPage::restoreOriginals()
{

}


// PALETTE PAGE //
PalettePage::PalettePage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QGroupBox* guidesGroup = new QGroupBox(tr("Guide Filenames"));

    QCheckBox* saveActiveColors = new QCheckBox("Save Active Colors", this);

    QLabel* filenameLabel = new QLabel(tr("Palette filename:"));
    QLineEdit* filenameEdit = new QLineEdit;

    QGridLayout* packagesLayout = new QGridLayout;
    packagesLayout->addWidget(saveActiveColors, 0, 0, 1, 2);
    packagesLayout->addWidget(filenameLabel, 1, 0);
    packagesLayout->addWidget(filenameEdit, 1, 1);
    guidesGroup->setLayout(packagesLayout);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(guidesGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // Populate the settings
    // Backup original values
    // Hook up the signals
}

void PalettePage::restoreOriginals()
{

}


// WIP PAGE //
WIPPage::WIPPage(QWidget* parent, QSettings* appSettings) :
    QWidget(parent),
    m_pAppSettings(appSettings)
{
    QLabel* wip = new QLabel("Soon", this);
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(wip);
    setLayout(mainLayout);
}

void WIPPage::restoreOriginals()
{

}



////////////////////////////////////////////////////////////////////////////////
// Helper Widgets //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ColorWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(2, 2, width()-4, height()-4, QBrush(m_color));

    const int wm1 = width()-1;
    const int hm1 = height()-1;

    painter.setPen(QPen(QColor(0,0,0)));
    painter.drawLine(0,   0,   wm1, 0);
    painter.drawLine(0,   0,   0,   hm1);
    painter.drawLine(wm1, hm1, 0,   hm1);
    painter.drawLine(wm1, hm1, wm1, 0);
}


void ColorWidget::colorChangedSlot(QColor color)
{
    m_color = color;
    emit colorChanged(m_color);
    update();
}


void ColorWidget::mousePressEvent(QMouseEvent*)
{
    QColor backupColor = m_color;

    QColorDialog qcd(m_color, this);
    connect(&qcd, SIGNAL(currentColorChanged(QColor)),
            this, SLOT(colorChangedSlot(QColor)));
    qcd.exec();

    QColor newColor = qcd.selectedColor();
    if (newColor.isValid())
        colorChangedSlot(newColor);
    else
        colorChangedSlot(backupColor);
}

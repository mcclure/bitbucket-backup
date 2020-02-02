#include "LayersWidget.h"

#include <iostream>
#include <algorithm>

#include <QLabel>
#include <QEvent>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>

QSize LayersWidget::minimumSizeHint() const
{
    return QSize(200, 100);
}


QSize LayersWidget::sizeHint() const
{
    return QSize(200, 200);
}

LayersWidget::LayersWidget(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Layers");

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    setLayout(vLayout);

    QGridLayout* topGridLayout = new QGridLayout();
    vLayout->addLayout(topGridLayout);

    // Layer blend mode
    QLabel* modeLabel = new QLabel("Mode");
    QComboBox* modeComboBox = new QComboBox();
    modeComboBox->addItem("Normal");
    topGridLayout->addWidget(modeLabel, 0, 0, 1, 1);
    topGridLayout->addWidget(modeComboBox, 0, 1, 1, 3);

    // Layer table
    m_listModel = new LayerListModel(this);
    m_listView = new LayerListView(this);
    m_listView->setModel(m_listModel);
    topGridLayout->addWidget(m_listView, 1, 0, 1, 4);

    // Set the top layer to be active on startup.
    m_listView->setCurrentIndex(m_listModel->index(0, 0));

    // Checkbox for ignoring faceset objects
    //QCheckBox* lockedCheckBox = new QCheckBox("Locked");
    //topGridLayout->addWidget(lockedCheckBox, 1, 0, 1, 2);

    // Move Layer Up
    //QPushButton* buttonUp = new QPushButton("UP");
    //buttonUp->setMaximumSize(50, 26);
    //connect(buttonUp, SIGNAL(pressed()), 
    //        this,     SLOT(moveSelectedUp()));

    // Move Layer Down
    //QPushButton* buttonDown = new QPushButton("DOWN");
    //buttonDown->setMaximumSize(50, 26);
    //connect(buttonDown, SIGNAL(pressed()), 
    //        this,       SLOT(moveSelectedDown()));

    // New Layer
    QPushButton* buttonNew = new QPushButton(" New");
    buttonNew->setToolTip("New Layer");
    buttonNew->setIcon(QIcon(QPixmap(":/icons/layerNew.png")));
    connect(buttonNew, SIGNAL(pressed()),
            this,      SLOT(newLayer()));

    // Duplicate Layer
    QPushButton* buttonDup = new QPushButton("");
    buttonDup->setToolTip("Duplicate Layer");
    buttonDup->setIcon(QIcon(QPixmap(":/icons/layerDuplicate.png")));
    buttonDup->setMaximumSize(50, 26);
    connect(buttonDup, SIGNAL(pressed()), 
            this,      SLOT(duplicateSelected()));

    // Delete Layer
    QPushButton* buttonDel = new QPushButton("");
    buttonDel->setToolTip("Delete Layer");
    buttonDel->setIcon(QIcon(QPixmap(":/icons/layerDelete.png")));
    buttonDel->setMaximumSize(50, 26);
    connect(buttonDel, SIGNAL(pressed()), 
            this,      SLOT(deleteSelected()));

    topGridLayout->addWidget(buttonNew,  2, 0, 1, 2);
    topGridLayout->addWidget(buttonDup,  2, 2, 1, 1);
    topGridLayout->addWidget(buttonDel,  2, 3, 1, 1);
}


void LayersWidget::newLayer()
{
    LayerListModel* mdl = dynamic_cast<LayerListModel*>(m_listModel);
    int row = m_listView->currentIndex().row();
    if (row == -1)
        row = 0;

    mdl->insertRow(row);

    LayerObject newLayer("New", true);
    m_listModel->listdata[row] = newLayer;
    m_listView->setCurrentIndex(mdl->index(row));
}


void LayersWidget::deleteSelected()
{
    LayerListModel* mdl = dynamic_cast<LayerListModel*>(m_listModel);
    int row = m_listView->currentIndex().row();
    if (row == -1)
        return;

    mdl->removeRow(row);
}


void LayersWidget::duplicateSelected()
{
    LayerListModel* mdl = dynamic_cast<LayerListModel*>(m_listModel);
    int row = m_listView->currentIndex().row();
    if (row == -1)
        return;

    LayerObject newLayer(mdl->listdata[row].name + " Dupe", m_listModel->listdata[row].visible);
    mdl->insertRow(row);
    mdl->listdata[row] = newLayer;
    m_listView->setCurrentIndex(mdl->index(row));
}


// void LayersWidget::moveSelectedUp()
// {
//     LayerListModel* mdl = dynamic_cast<LayerListModel*>(m_listModel);
//     int row = m_listView->currentIndex().row();
//     if (row == 0 or row == -1)
//         return;
// 
//     std::swap(mdl->listdata[row-1],mdl->listdata[row]);
//     m_listView->setCurrentIndex(mdl->index(row-1));
// }


// void LayersWidget::moveSelectedDown()
// {
//     LayerListModel* mdl = dynamic_cast<LayerListModel*>(m_listModel);
//     int row = m_listView->currentIndex().row();
//     if (row == mdl->rowCount(QModelIndex())-1 or row == -1)
//         return;
// 
//     std::swap(mdl->listdata[row+1],mdl->listdata[row]);
//     m_listView->setCurrentIndex(mdl->index(row+1));
// }


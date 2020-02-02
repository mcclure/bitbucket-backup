#ifndef __LAYERS_WIDGET_H__
#define __LAYERS_WIDGET_H__

#include <QWidget>
#include <QListView>
#include <QMouseEvent>

#include <iostream>

class ListView;
class LayerListView;
class LayerListModel;


////////////////////////////////////////////////////////////////////////////////
// TODO: Replace this & all instances of this with a real layer object
class LayerObject
{
public:
    LayerObject() : visible(true), name("") { }
    LayerObject(const std::string& n, bool v) : visible(v), name(n) { }
    
public:
    bool visible;
    std::string name;
    
public:
    LayerObject& operator=(const LayerObject& other)
    {
        if (this != &other)
        {
            visible = other.visible;
            name = other.name;
        }
        return *this;
    };
};


////////////////////////////////////////////////////////////////////////////////
class LayersWidget : public QWidget
{
    Q_OBJECT
            
public:
    LayersWidget(QWidget* parent = NULL);
    ~LayersWidget() {}

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:
    void newLayer();
    void deleteSelected();
    void duplicateSelected();
    //void moveSelectedUp();
    //void moveSelectedDown();
    
private:
    LayerListView* m_listView;
    LayerListModel* m_listModel;
};


////////////////////////////////////////////////////////////////////////////////
class LayerListModel : public QAbstractListModel
{
public:
    LayerListModel(QObject* parent = NULL) : 
        QAbstractListModel(parent),
        listdata(),
        eyePic(":/icons/layerEye.png"),
        blankPic(":/icons/layerEye.png")
    {
        // Init data in - this is obviously silly and needs to be elsewhere
        listdata.push_back(LayerObject(std::string("Layer 1"), true));
        listdata.push_back(LayerObject(std::string("Layer 2"), false));
        listdata.push_back(LayerObject(std::string("Layer 3"), true));
        listdata.push_back(LayerObject(std::string("Layer 4"), true));
        
        // Clear out the eye image so as to create an 'invisible' image.
        blankPic.fill(QColor(255, 255, 255, 0));
    }
    ~LayerListModel() {}

public:
    // TODO: Make private
    std::vector<LayerObject> listdata;
    QPixmap eyePic;
    QPixmap blankPic;

public:
    int rowCount(const QModelIndex&) const
    {
        return listdata.size();
    }
    
    bool insertRows(int row, int, const QModelIndex& parent = QModelIndex())
    {
        beginInsertRows(parent, row, row);
        listdata.insert(listdata.begin() + row, LayerObject());
        endInsertRows();
        return true;
    }
    
    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex())
    {
        beginRemoveRows(parent, row, row);
        listdata.erase(listdata.begin() + row);
        endRemoveRows();
        return true;
    }
    
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const
    {
        if (!index.isValid())
            return QVariant();
        
        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            return QVariant(QString(listdata[index.row()].name.c_str()));
        }
        else if (role == Qt::DecorationRole)
        {
            if (listdata[index.row()].visible == true)
                return QVariant(eyePic);
            else
                return QVariant(blankPic);
        }
        return QVariant();
    }
    
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)
    {
        int row = index.row();
        if (role == Qt::EditRole)
        {
            listdata[row].name = value.toString().toStdString();
            emit dataChanged(index, index);
        }
        else if (role == Qt::DecorationRole)
        {
            // SLOW COMPARE!
            if (value.value<QPixmap>().toImage() == blankPic.toImage())
                listdata[row].visible = false;
            else
                listdata[row].visible = true;
        }
        return true;
    }
    
    Qt::DropActions supportedDropActions() const
    {
        return Qt::MoveAction;
    }
    
    Qt::ItemFlags flags(const QModelIndex& index) const
    {
        Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (index.isValid())
        {
            return (Qt::ItemIsDragEnabled | Qt::ItemIsEditable | defaultFlags);
        }
        else
        {
            return (Qt::ItemIsDropEnabled | Qt::ItemIsEditable | defaultFlags);
        }
    }
};


////////////////////////////////////////////////////////////////////////////////
class LayerListView : public QListView
{
public:
    LayerListView(QWidget* parent = NULL) : QListView(parent)
    {
        setDragDropMode(QListView::InternalMove);
        setDragDropOverwriteMode(false);
        setViewMode(QListView::ListMode);
        setMovement(QListView::Snap);
        setUniformItemSizes(true);
    }
    ~LayerListView() {}

protected:
    void mousePressEvent(QMouseEvent* event)
    {
        QModelIndex index = indexAt(event->pos());
        if (event->pos().x() < 53)
        {
            LayerListModel* mdl = dynamic_cast<LayerListModel*>(model());
            if (mdl->listdata[index.row()].visible)
                mdl->listdata[index.row()].visible = false;
            else
                mdl->listdata[index.row()].visible = true;
            emit dataChanged(index, index);
        }
        
        // Proceed with the parent's control
        QListView::mousePressEvent(event);
    }
};


#endif

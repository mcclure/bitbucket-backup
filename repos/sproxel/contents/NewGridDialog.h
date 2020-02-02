#ifndef NEWGRIDDIALOG_H
#define NEWGRIDDIALOG_H

#include <QDialog>

#include <ImathVec.h>

namespace Ui {
    class NewGridDialog;
}

class NewGridDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewGridDialog(QWidget *parent = 0);
    ~NewGridDialog();

    Imath::V3i getVoxelSize();
    bool isIndexed();

public slots:
    int exec();

private:
    Ui::NewGridDialog *ui;

    static Imath::V3i lastSize;
    static bool lastIndexed;
};

#endif // NEWGRIDDIALOG_H

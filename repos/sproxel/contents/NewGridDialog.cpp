#include "NewGridDialog.h"
#include "ui_NewGridDialog.h"


Imath::V3i NewGridDialog::lastSize(16);
bool NewGridDialog::lastIndexed=false;


NewGridDialog::NewGridDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGridDialog)
{
    ui->setupUi(this);
    ui->width ->setValue(lastSize.x);
    ui->height->setValue(lastSize.y);
    ui->depth ->setValue(lastSize.z);

    if (lastIndexed)
      ui->dataIndexed->setChecked(true);
    else
      ui->dataRGBA->setChecked(true);
}

NewGridDialog::~NewGridDialog()
{
    delete ui;
}

int NewGridDialog::exec()
{
  int r=QDialog::exec();

  if (r)
  {
    lastSize=getVoxelSize();
    lastIndexed=isIndexed();
  }

  return r;
}

Imath::V3i NewGridDialog::getVoxelSize()
{
    return Imath::V3i(ui->width->value(),
                      ui->height->value(),
                      ui->depth->value());
}

bool NewGridDialog::isIndexed()
{
  return ui->dataIndexed->isChecked();
}

#ifndef __SPROXEL_CONSOLE_WIDGET_H__
#define __SPROXEL_CONSOLE_WIDGET_H__


#include <QPlainTextEdit>
#include <QAction>


class ConsoleWidget : public QPlainTextEdit
{
  Q_OBJECT

public:

  ConsoleWidget(const QString &title, QWidget *parent=NULL);

  QAction* toggleViewAction() const { return viewAction; }

  QSize sizeHint() const;

protected:

  QAction *viewAction;

  void closeEvent(QCloseEvent* event)
  {
    hide();
    if (viewAction) viewAction->setChecked(false);
    event->ignore();
  }
};


#endif

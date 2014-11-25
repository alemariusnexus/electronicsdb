#ifndef SQLGENERATORDIALOG_H_
#define SQLGENERATORDIALOG_H_

#include "global.h"
#include <QtGui/QWidget>
#include <ui_SQLGeneratorDialog.h>
#include "DatabaseConnection.h"



class SQLGeneratorDialog : public QDialog
{
	Q_OBJECT

public:
	SQLGeneratorDialog(QWidget* parent = NULL);

private slots:
	void genCodeButtonClicked();

private:
	Ui_SQLGeneratorDialog ui;
};

#endif /* SQLGENERATORDIALOG_H_ */

#pragma once

#include "ui_mainwindow.h"
#include <QMainWindow>

namespace Ui
{
	class Ui::MainWindow;
} // namespace Ui

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget * parent = nullptr);
	~MainWindow();
	
	MainWindow(MainWindow&&) = delete;
	MainWindow(const MainWindow&) = delete;
	MainWindow& operator=(MainWindow&&) = delete;
	MainWindow& operator=(const MainWindow&) = delete;

	Ui::MainWindow * ui;

private slots:
	void on_button_clicked();

private:
};

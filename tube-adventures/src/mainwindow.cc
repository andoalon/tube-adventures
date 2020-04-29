#include "mainwindow.hh"
#include <QMessageBox>

using namespace std::literals::chrono_literals;

MainWindow::MainWindow(QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->my_button, SIGNAL(clicked()), this, SLOT(on_button_clicked()));
}

void MainWindow::on_button_clicked()
{
	this->close();
}

MainWindow::~MainWindow()
{
	delete ui;
}

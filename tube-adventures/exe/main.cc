#include "mainwindow.hh"

#include <QApplication>

int main(int argc, char * argv[])
{
	QApplication app(argc, argv);

	MainWindow window;
	window.show();

	const auto retval = app.exec();

	return retval;
}

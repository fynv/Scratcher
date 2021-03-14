#include "Scratcher.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	Scratcher scratcher;
	scratcher.show();

	return app.exec();
}

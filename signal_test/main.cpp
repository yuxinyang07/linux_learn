#include "mainwindow.h"
#include <QApplication>
#include <signal.h>

void signal_handle(int num)
{
    printf("get a signal \n");
}


int main(int argc, char *argv[])
{
 //   QApplication a(argc, argv);
 //   MainWindow w;
 //   w.show();
  //  signal(SIGKILL,signal_handle);
 //   signal(SIGINT,signal_handle);
  //  signal(SIGQUIT,signal_handle);
 //   return a.exec();
    return  0;
}

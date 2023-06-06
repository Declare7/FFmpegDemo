
#include "MainWindow.h"
#include "ui_MainWindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->tabVideoInput, &VideoInputUI::printMsg, this, &MainWindow::onPrintMsg);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onPrintMsg(QString msg)
{
    ui->textEdit->append(msg);
}



void MainWindow::on_pushButton_clicked()
{
    ui->textEdit->clear();
}


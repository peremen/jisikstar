#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "converthandler.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
}

MainWindow::~MainWindow()
{
    this->convertThread.quit();
    this->convertThread.wait();
    delete ui;
}

void MainWindow::on_btnOpen_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open DioDict File"), QString(),
                                                    tr("DioDict Data Files (*.dat)"));
    if (fileName.length() > 0) ui->txtDiodictFile->setText(fileName);

    if (ui->rdTab->isChecked()) {
        ui->txtStarDictFile->setText(fileName.replace(".dat", ".tab"));
    } else if (ui->rdBabylon->isChecked()) {
        ui->txtStarDictFile->setText(fileName.replace(".dat", ".bgl"));
    } else if (ui->rdStardict->isChecked()) {
        ui->txtStarDictFile->setText(fileName.replace(".dat", ".tar.bz2"));
    }
}

void MainWindow::on_action_Open_triggered()
{
    return on_btnOpen_clicked();
}

void MainWindow::on_btnSave_clicked()
{
    QString filter_tab = tr("Tabbed Files (*.tab)");
    QString filter_bgl = tr("Babylon Files (*.bgl)");
    QString filter_dz = tr("Compressed StarDict Files (*.tar.bz2");
    QFileDialog fd(this, tr("Save Dictionary Source File"));
    fd.setAcceptMode(QFileDialog::AcceptSave);

    if (ui->rdTab->isChecked()) {
        fd.setNameFilter(filter_tab);
        fd.setDefaultSuffix(".tab");
    } else if (ui->rdBabylon->isChecked()) {
        fd.setNameFilter(filter_bgl);
        fd.setDefaultSuffix(".bgl");
    } else if (ui->rdStardict->isChecked()) {
        fd.setNameFilter(filter_dz);
        fd.setDefaultSuffix(".tar.bz2");
    }
    if (fd.exec()) {
        ui->txtStarDictFile->setText(fd.selectedFiles()[0]);
    }
}

void MainWindow::on_btnConvert_clicked()
{
    ConvertHandler *h;

    if (ui->txtStarDictFile->text().isEmpty()) {
        QMessageBox::warning(this, tr("JisikStar"), tr("No output file specified."));
        return;
    }

    h = new ConvertHandler();
    h->setSource(ui->txtDiodictFile->text());
    h->setDestination(ui->txtStarDictFile->text());

    if (ui->rdTab->isChecked()) {
        h->setFileType(Snotra::Tab);
    } else if (ui->rdBabylon->isChecked()) {
        h->setFileType(Snotra::Babylon);
    } else if (ui->rdStardict->isChecked()) {
        h->setFileType(Snotra::Stardict);
    } else {
        Q_ASSERT(false);
    }

    h->moveToThread(&this->convertThread);
    connect(&this->convertThread, SIGNAL(started()), h, SLOT(convert()));
    connect(&this->convertThread, SIGNAL(finished()), h, SLOT(deleteLater()));
    connect(h, SIGNAL(conversionFinished(int)), this, SLOT(on_conversionFinished(int)));

    ui->btnConvert->setEnabled(false);
    ui->btnSave->setEnabled(false);
    ui->btnOpen->setEnabled(false);
    ui->txtDiodictFile->setReadOnly(true);
    ui->txtStarDictFile->setReadOnly(true);

    ui->statusBar->showMessage(tr("Converting..."));
    ui->progressBar->setMaximum(0);
    ui->progressBar->setVisible(true);
    this->convertThread.start();
}

void MainWindow::on_action_Convert_triggered()
{
    return on_btnConvert_clicked();
}

void MainWindow::on_action_Exit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_conversionFinished(int status) {
    ui->statusBar->showMessage(tr("Ready"));
    ui->progressBar->setMaximum(1);
    ui->progressBar->setVisible(false);

    ui->btnConvert->setEnabled(true);
    ui->btnSave->setEnabled(true);
    ui->btnOpen->setEnabled(true);
    ui->txtDiodictFile->setReadOnly(false);
    ui->txtStarDictFile->setReadOnly(false);

    switch(status) {
    case 0:
        QMessageBox::information(this, tr("JisikStar"), tr("Conversion complete."));
        break;
    case 1:
        QMessageBox::information(this, tr("JisikStar"), tr("File open failed."));
        break;
    case 2:
        QMessageBox::information(this, tr("JisikStar"), tr("Unsupported file."));
        break;
    default:
        break;
    }
}

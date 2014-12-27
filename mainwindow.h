#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnOpen_clicked();
    void on_action_Open_triggered();
    void on_btnSave_clicked();
    void on_btnConvert_clicked();
    void on_action_Convert_triggered();
    void on_action_Exit_triggered();

public slots:
    void on_conversionFinished(int status);

private:
    Ui::MainWindow *ui;
    QThread convertThread;

};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include "imglabel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    ImgLabel * imglabel;
    void adjust_size();


private:
    Ui::MainWindow *ui;
    double cond_times[10] = {1.0, 0.25, 0.5, 0.75, 1.25, 1.5, 2.0, 2.5, 3.0, 5.0};
    int cond_count = 10;

    int fast_forward_cands[8] = {-100, -20, -5, -1, 1, 5, 20, 100};
    int fast_forward_count = 8;

private slots:
    void on_open_movie_clicked();
    void on_control_clicked();
    void play_times_changed();
    void on_jump_frame_clicked();
    void fast_forward_clicked(int index);
    //void fast_forward_frame_clicked(int index);
};
#endif // MAINWINDOW_H

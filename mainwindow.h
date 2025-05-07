#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QStringList>
#include <QPushButton>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_matrix_raster_data.h>

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void updatePlot();
    void changeMode();

private:
    Ui::MainWindow *ui;

    QwtPlot *rxFftPlot;
    QwtPlotCurve *rxFftCurve;

    QwtPlot *txFftPlot;
    QwtPlotCurve *txFftCurve;

    QwtPlot *rxWaterfallPlot;
    QwtPlotSpectrogram *rxSpectrogram;
    QwtMatrixRasterData *rxRasterData;
    QVector<QVector<double>> rxWaterfallData;

    QTimer *timer;
    QPushButton *modeButton;

    int fftSize = 512;
    int maxLines = 200;
    int currentMode = 0;
    bool pttEnabled = false;

    QStringList modeNames = {
        "Gaussian Pulse",
        "Sine Wave",
        "Double Peak",
        "Random Noise",
        "Moving Carrier"
    };
};

#endif // MAINWINDOW_H

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QtMath>
#include <QRandomGenerator>
#include <QMessageBox>
#include <qwt_color_map.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowState(Qt::WindowMaximized);

    // === RX FFT ===
    rxFftPlot = new QwtPlot(this);
    rxFftPlot->setCanvasBackground(Qt::black);
    rxFftPlot->setAxisTitle(QwtPlot::xBottom, "RX Frequency");
    rxFftPlot->setAxisTitle(QwtPlot::yLeft, "Power");
    rxFftPlot->setAxisScale(QwtPlot::xBottom, 0, fftSize);
    rxFftCurve = new QwtPlotCurve("RX FFT");
    rxFftCurve->setPen(Qt::green, 2.0);
    rxFftCurve->attach(rxFftPlot);

    // === TX FFT ===
    txFftPlot = new QwtPlot(this);
    txFftPlot->setCanvasBackground(Qt::black);
    txFftPlot->setAxisTitle(QwtPlot::xBottom, "TX Frequency");
    txFftPlot->setAxisTitle(QwtPlot::yLeft, "Power");
    txFftPlot->setAxisScale(QwtPlot::xBottom, 0, fftSize);
    txFftCurve = new QwtPlotCurve("TX FFT");
    txFftCurve->setPen(Qt::magenta, 2.0);
    txFftCurve->attach(txFftPlot);

    QHBoxLayout *fftLayout = new QHBoxLayout();
    fftLayout->addWidget(rxFftPlot);
    fftLayout->addWidget(txFftPlot);
    ui->fftPlotWidget->setLayout(fftLayout);

    // === Waterfall ===
    rxWaterfallPlot = new QwtPlot(this);
    rxWaterfallPlot->setCanvasBackground(Qt::black);
    rxWaterfallPlot->setAxisTitle(QwtPlot::xBottom, "RX Frequency");
    rxWaterfallPlot->setAxisTitle(QwtPlot::yLeft, "Time");
    rxWaterfallPlot->setAxisScale(QwtPlot::xBottom, 0, fftSize);
    rxWaterfallPlot->setAxisScale(QwtPlot::yLeft, 0, maxLines);

    rxSpectrogram = new QwtPlotSpectrogram();
    rxRasterData = new QwtMatrixRasterData();
    rxRasterData->setInterval(Qt::XAxis, QwtInterval(0, fftSize));
    rxRasterData->setInterval(Qt::YAxis, QwtInterval(0, maxLines));
    rxRasterData->setInterval(Qt::ZAxis, QwtInterval(0, 100));
    rxRasterData->setResampleMode(QwtMatrixRasterData::NearestNeighbour);

    QVector<double> initData(maxLines * fftSize, 0.0);
    rxRasterData->setValueMatrix(initData, fftSize);
    rxSpectrogram->setData(rxRasterData);

    QwtLinearColorMap *colorMap = new QwtLinearColorMap(Qt::cyan, Qt::red);
    colorMap->addColorStop(0.5, Qt::yellow);
    rxSpectrogram->setColorMap(colorMap);
    rxSpectrogram->attach(rxWaterfallPlot);

    QVBoxLayout *wfLayout = new QVBoxLayout();
    wfLayout->addWidget(rxWaterfallPlot);

    // === Mode Button ===
    modeButton = new QPushButton("Change FFT Mode", this);
    wfLayout->addWidget(modeButton);
    connect(modeButton, &QPushButton::clicked, this, &MainWindow::changeMode);

    ui->waterfallPlotWidget->setLayout(wfLayout);

    // === PTT Toggle ===
    connect(ui->pttButton, &QPushButton::toggled, this, [=](bool checked){
        pttEnabled = checked;
    });

    // === Sync RX/Spin ===
    connect(ui->rxFreqSlider, &QSlider::valueChanged, this, [=](int val){
        ui->rxFreqSpin->setValue(val / 10.0);
    });
    connect(ui->rxFreqSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
        ui->rxFreqSlider->setValue(static_cast<int>(val * 10));
    });

    connect(ui->txFreqSlider, &QSlider::valueChanged, this, [=](int val){
        ui->txFreqSpinBox->setValue(val / 10.0);
    });
    connect(ui->txFreqSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
        ui->txFreqSlider->setValue(static_cast<int>(val * 10));
    });

    connect(ui->connectButton, &QPushButton::clicked, this, [=](){
        QString ip = ui->ipLineEdit->text();
        QString port = ui->portLineEdit->text();
        QMessageBox::information(this, "Connection", "Bağlanılacak IP: " + ip + ", Port: " + port);
    });

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updatePlot);
    timer->start(100);
}

void MainWindow::updatePlot()
{
    QVector<double> rxFft(fftSize);
    QVector<double> txFft(fftSize);
    static double phase = 0;

    for (int i = 0; i < fftSize; ++i) {
        double t = static_cast<double>(i) / fftSize;

        switch (currentMode) {
        case 0: rxFft[i] = 60 * qExp(-qPow((t - 0.5) * 20, 2)) + QRandomGenerator::global()->bounded(10); break;
        case 1: rxFft[i] = 30 * qSin(2 * M_PI * 10 * t) + QRandomGenerator::global()->bounded(5); break;
        case 2: rxFft[i] = 40 * qExp(-qPow((t - 0.3) * 20, 2)) + 30 * qExp(-qPow((t - 0.7) * 20, 2)) + QRandomGenerator::global()->bounded(5); break;
        case 3: rxFft[i] = QRandomGenerator::global()->bounded(0, 60); break;
        case 4: rxFft[i] = 50 * qExp(-qPow((t - 0.3 - 0.2 * qSin(phase)) * 20, 2)); break;
        default: rxFft[i] = 0; break;
        }

        if (pttEnabled)
            txFft[i] = 30 * qSin(2 * M_PI * 10 * t + phase) + QRandomGenerator::global()->bounded(5);
        else
            txFft[i] = 0;
    }

    phase += 0.05;

    QVector<double> x(fftSize);
    for (int i = 0; i < fftSize; ++i)
        x[i] = i;

    rxFftCurve->setSamples(x, rxFft);
    rxFftPlot->replot();

    txFftCurve->setSamples(x, txFft);
    txFftPlot->replot();

    rxWaterfallData.push_front(rxFft);
    if (rxWaterfallData.size() > maxLines)
        rxWaterfallData.pop_back();

    QVector<double> flatData;
    flatData.reserve(maxLines * fftSize);
    for (const auto &row : rxWaterfallData)
        flatData += row;

    rxRasterData->setValueMatrix(flatData, fftSize);
    rxWaterfallPlot->replot();
}

void MainWindow::changeMode()
{
    currentMode = (currentMode + 1) % modeNames.size();
    QMessageBox::information(this, "FFT Mode Changed", "Current Mode: " + modeNames[currentMode]);
}

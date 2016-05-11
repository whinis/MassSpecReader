#ifndef READERWINDOW_H
#define READERWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include "qcustomplot.h"

namespace Ui {
class ReaderWindow;
}

class ReaderWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ReaderWindow(QWidget *parent = 0);
    ~ReaderWindow();

    void addVisibilityButtons(int graphID);

public slots:

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void on_resetAction_clicked();

    void on_graphAction_clicked();

    void on_cutoffSelect_currentIndexChanged(int index);

    void on_saveGraphButton_clicked();

    void on_pushButton_3_clicked();

    void on_AddFile_clicked();

    void on_RemoveFile_clicked(QString name);

    void on_graphToggle_clicked(QString name);

    void on_About_clicked();

    void on_averageCutoffType_currentIndexChanged(int index);

    void on_HideAll_clicked();

    void on_ShowAll_clicked();

    void on_actionSet_CI_Range_triggered();

    void on_actionSet_EI_Range_triggered();

    void on_actionCalculate_Ratio_triggered();

    void on_actionCalculate_All_Ratios_triggered();

    void on_legendToggle_clicked();

    void on_actionSum_Peaks_triggered();

    void on_actionSum_Peaks_all_graphs_triggered();

    void on_pushButton_4_clicked();

    void on_resetGraph_clicked();

    void on_yGrow_clicked();

    void on_yShrink_clicked();

    void on_xShrink_clicked();

    void on_xGrow_clicked();

private:

    Ui::ReaderWindow *ui;

    QString cutoffName;

    QString lastDirectory;

    QLineEdit* fileNameBox;

    QLineEdit* cutoffBox;

    QTextEdit* peakBox;

    QCustomPlot* plot;

    QStackedWidget* mainWindow;

    QListWidget* files;

    int cutoffType;

    QList<QString> peaks;



    QList<QString> fileList;
    QList<double>      cutoffValueList;
    QList<int>      cutoffTypeList;
    QList<double>      fileVoltageList;




    QSignalMapper* signalMapper;
    QSignalMapper* graphSignalMapper;

    int lastFileID;


    double CI;
    double ciStart;
    double ciEnd;

    double EI;
    double eiStart;
    double eiEnd;

    int zoomX;

    int zoomY;
};

#endif // READERWINDOW_H

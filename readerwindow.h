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

    void on_legendToggle_clicked();

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




    QSignalMapper* signalMapper;
    QSignalMapper* graphSignalMapper;

    int lastFileID;


    double CI;
    double EI;
};

#endif // READERWINDOW_H

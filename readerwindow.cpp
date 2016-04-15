#include "readerwindow.h"
#include "ui_readerwindow.h"

ReaderWindow::ReaderWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ReaderWindow)
{
    lastDirectory = "";
    cutoffName = "";

    ui->setupUi(this);
    mainWindow = centralWidget()->findChild<QStackedWidget *>("stackedWidget");
    fileNameBox = mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("fileNameBox");
    cutoffBox = mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("cutoffAmount");
    cutoffBox->setVisible(false);


    mainWindow->findChild<QWidget *>("InputPage")->findChild<QComboBox *>("averageCutoffType")->setVisible(false);
    mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("averageCutoff")->setVisible(false);


    plot = mainWindow->findChild<QWidget *>("GraphPage")->findChild<QCustomPlot *>("plot");
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    plot->xAxis->setLabel("M/Z");
    plot->yAxis->setLabel("Intensity");
    plot->legend->setVisible(true);

    peakBox = mainWindow->findChild<QWidget *>("GraphPage")->findChild<QTextEdit *>("PeakList");

    signalMapper = new QSignalMapper(this);
    connect(signalMapper, SIGNAL(mapped(QString)),this, SLOT(on_RemoveFile_clicked(QString)));
    lastFileID = 0;

    graphSignalMapper = new QSignalMapper(this);
    connect(graphSignalMapper, SIGNAL(mapped(QString)),this, SLOT(on_graphToggle_clicked(QString)));

    QAction *aboutAction = ui->menuBar->addAction("About");
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(on_About_clicked()));
}
ReaderWindow::~ReaderWindow()
{
    delete ui;
}

void ReaderWindow::on_pushButton_2_clicked()
{
    QWidget* item = mainWindow->findChild<QWidget *>("VisibilityBox");
    QLayoutItem *child;
    while ((child = item->layout()->takeAt(0)) != 0)  {
        delete child->widget();
        delete child;
    }

    plot->clearGraphs();
    plot->addGraph();
    plot->addGraph();
    plot->graph(0)->setPen(QPen(Qt::red));
    qsrand( QDateTime::currentDateTime().toTime_t() );
    if(fileList.length()>1){
        addVisibilityButtons(0);
        plot->graph(0)->setName("Original Average");
        plot->graph(1)->setName("Cleaned Average");
    }else{
        plot->graph(0)->setVisible(false);
        plot->graph(1)->setVisible(false);
    }

    QVector<double> avgX;
    QVector<double> cntX;
    QVector<double> avgY;
    QVector<double> avgYc;
    for(int f = 0; f < fileList.length(); f++){

        QVector<double> x;
        QVector<double> y;
        QVector<double> yc;

        //calc graph id
        int graphID = (f+1)*2;

        //make sure graphs exist
        //Name graphs and give random color
        if(plot->graph(graphID) == 0){
            plot->addGraph();
            plot->graph(graphID)->setPen(QPen(QColor(qrand() % 255,qrand() % 255,qrand() % 255)));
            plot->graph(graphID)->setName("Original "+QString::number(f+1));
        }
        if(plot->graph(graphID+1) == 0){
            plot->addGraph();
            plot->graph(graphID+1)->setPen(QPen(QColor(qrand() % 255,qrand() % 255,qrand() % 255)));
            plot->graph(graphID+1)->setName("Cleaned "+QString::number(f+1));
        }

        //load file
        QFile file(fileList.at(f));
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << file.errorString();
           // return;
        }else{
            //load in data points and find min and max
            double max = 0;
            double min = INFINITE;
            while (!file.atEnd()) {
                QByteArray line = file.readLine();
                QList<QByteArray> split = line.split(',');
                x.append(split.first().toDouble());
                double temp = split.at(1).simplified().toDouble();
                y.append(temp);

                //used for later
                if(temp > max)
                    max = temp;

                if(temp < min)
                    min = temp;
            }
            //set min to zero
            for(int i=0; i<y.length();i++){
                y[i] = y[i] - min;
            }
            // plot original data just shifted
            plot->graph(graphID)->setData(x, y);

            //calculate cutoff
            double cutoff = 0;
            int cType = cutoffTypeList.at(f);
            double cVal = cutoffValueList.at(f);
            if(cType > 0){
                // Determine cutoff value if any
                if(cType == 1){
                    //select median as cutoff
                    QVector<double> medianSort = y;
                    std::sort(medianSort.begin(),medianSort.end());
                    cutoff = medianSort.at(y.length()/2);
                }else if(cType == 2){
                    //select multiple of median as cutoff
                    QVector<double> medianSort = y;
                    std::sort(medianSort.begin(),medianSort.end());
                    cutoff = medianSort.at(y.length()/2) * cVal;
                }else if( cType == 3){
                    //select percentage of max as cutoff
                    cutoff = (max/100) * cVal;
                }else if (cType == 4){
                    //absolute cutoff
                    cutoff = cVal;
                }
            }
            double sum = 0;
            QString peak = "";

            //cutoff those values
            for(int i=0; i<y.length();i++){
                double temp = y.at(i);
                if(avgX.contains(x.at(i))){
                    avgY[avgX.indexOf(x.at(i))] += temp;
                    cntX[avgX.indexOf(x.at(i))]++;
                }else{
                    avgX.append(x.at(i));
                    avgY.append(temp);
                    cntX.append(1);
                }
                if(temp > cutoff){
                    if(sum == 0)
                        peak.append(QString::number(x.at(i)) + " -  ");
                    sum += temp;
                    yc.append(temp);
                }else{
                    yc.append(0);
                    if( sum > 0){
                        peak.append(QString::number(x.at(i-1)) + ":\t "+ QString::number(sum) + "\r\n");
                        sum = 0;
                    }
                }
                if(sum > 0 && i == (y.length()-1)){
                    peak.append(QString::number(x.at(i-1)) + ":\t "+ QString::number(sum) + "\r\n");
                }
            }
            //graph "cleaned" data
            plot->graph(graphID+1)->setData(x, yc);

            // add toggle buttons
            addVisibilityButtons(graphID);


            peakBox->setText(peak);
        }
    }

    //if we have an average calculcate it
    if(fileList.length()>1){

        // plot average graph
        double max = 0;
        double min = INFINITE;

        for(int i=0; i<avgY.length();i++){
            avgY[i] = avgY[i]/cntX[i];

            //used for later
            if(avgY[i] > max)
                max = avgY[i];

            if(avgY[i] < min)
                min = avgY[i];
        }

        //correct for noise
        for(int i=0; i<avgY.length();i++){
            avgY[i] = avgY[i] - min;
        }

        plot->graph(0)->setData(avgX, avgY);

        double cutoff = 0;
        int cType = mainWindow->findChild<QWidget *>("InputPage")->findChild<QComboBox *>("averageCutoffType")->currentIndex();
        double cVal = mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("averageCutoff")->text().toDouble();
        if(cType > 0){
            // Determine cutoff value if any
            if(cType == 1){
                QVector<double> medianSort = avgY;
                std::sort(medianSort.begin(),medianSort.end());
                cutoff = medianSort.at(avgY.length()/2);
            }else if(cType == 2){
                QVector<double> medianSort = avgY;
                std::sort(medianSort.begin(),medianSort.end());
                cutoff = medianSort.at(avgY.length()/2) * cVal;
            }else if( cType == 3){
                cutoff = (max/100) * cVal;
            }else if (cType == 4){
                cutoff = cVal;
            }
            qDebug() << QString::number(cutoff);


        }
        double sum = 0;
        QString peak = "";

        //cutoff those values
        for(int i=0; i<avgY.length();i++){
            double temp = avgY.at(i);
            if(temp > cutoff){
                if(sum == 0)
                    peak.append(QString::number(avgX.at(i)) + " - ");
                sum += temp;
                avgYc.append(temp);
            }else{
                avgYc.append(0);
                if( sum > 0){
                    peak.append(QString::number(avgX.at(i-1)) + " : "+ QString::number(sum) + "\r\n");
                    sum = 0;
                }
            }
            if(sum > 0 && i == (avgY.length()-1)){
                peak.append(QString::number(avgX.at(i-1)) + ":\t "+ QString::number(sum) + "\r\n");
            }
        }
        plot->graph(1)->setData(avgX, avgYc);

        peakBox->setText(peak);
    }

    //add pushed to push visibility buttons
    QWidget* graphBox = new QWidget();
    QVBoxLayout* graphLayout = new QVBoxLayout();

    graphLayout->addStretch();
    graphBox->setLayout(graphLayout);
    mainWindow->findChild<QWidget *>("VisibilityBox")->layout()->addWidget(graphBox);

    //rescale graph
    plot->graph(2)->rescaleAxes();
    plot->replot();
    mainWindow->setCurrentIndex(1);
}


//Change page to input page
void ReaderWindow::on_resetAction_clicked()
{
    mainWindow->setCurrentIndex(0);
}

// change to graph page
void ReaderWindow::on_graphAction_clicked()
{
    mainWindow->setCurrentIndex(1);
}

//select file
void ReaderWindow::on_pushButton_clicked()
{
    fileNameBox->setText(QFileDialog::getOpenFileName(this,
        tr("Open CSV"), lastDirectory, tr("CSV Files (*.csv *.txt)")));
    QFileInfo fileInfo(fileNameBox->text());
    lastDirectory = fileInfo.canonicalFilePath();
}

//save new cutOff values
void ReaderWindow::on_cutoffSelect_currentIndexChanged(int index)
{
    cutoffType = index;
    cutoffName = mainWindow->findChild<QComboBox *>("cutoffSelect")->currentText();
    if(index>1){
        this->cutoffBox->setVisible(true);
    }else{
        this->cutoffBox->setVisible(false);
        this->cutoffBox->setText("");
    }
}
//show save graph page
void ReaderWindow::on_saveGraphButton_clicked()
{
    mainWindow->setCurrentIndex(2);
}
//actually save graph
void ReaderWindow::on_pushButton_3_clicked()
{
    int fileType = mainWindow->findChild<QWidget *>("saveGraph")->findChild<QComboBox *>("graphFileType")->currentIndex();
    int width = mainWindow->findChild<QWidget *>("saveGraph")->findChild<QLineEdit *>("graphWidth")->text().toInt();
    int height = mainWindow->findChild<QWidget *>("saveGraph")->findChild<QLineEdit *>("graphHeight")->text().toInt();

    if(fileType == 0){
        plot->savePng(QFileDialog::getSaveFileName(this,tr("Save Graph"), lastDirectory,tr("PNG (*.png )")),width,height);
    }else if (fileType == 1){
        plot->saveJpg(QFileDialog::getSaveFileName(this,tr("Save Graph"), lastDirectory,tr("JPG (*.jpg )")),width,height);
    }else if (fileType == 2){
        plot->saveBmp(QFileDialog::getSaveFileName(this,tr("Save Graph"), lastDirectory,tr("BMP (*.bmp )")),width,height);
    }else if (fileType == 3){
        plot->savePdf(QFileDialog::getSaveFileName(this,tr("Save Graph"), lastDirectory,tr("PDF (*.pdf )")),width,height);
    }


    mainWindow->setCurrentIndex(1);
}
//add a file to the scroll bar and needed list
void ReaderWindow::on_AddFile_clicked()
{

    //Remove spacer that makes things look nice
    QWidget* spacer =mainWindow->findChild<QWidget *>("FilesBox")->findChild<QWidget *>("fileSpacer");

    QLayoutItem *child;
    while ((child = spacer->layout()->takeAt(0)) != 0)  {
        delete child->widget();
        delete child;
    }
    delete spacer;

    //add file to list
    fileList.append(fileNameBox->text());
    cutoffValueList.append(cutoffBox->text().toDouble());
    cutoffTypeList.append(cutoffType);
    fileVoltageList.append(mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("fileVoltage")->text().toDouble());

    if(fileList.length() > 1){
        mainWindow->findChild<QWidget *>("InputPage")->findChild<QComboBox *>("averageCutoffType")->setVisible(true);
    }else{
        mainWindow->findChild<QWidget *>("InputPage")->findChild<QComboBox *>("averageCutoffType")->setVisible(false);
    }

    //make the needed items in the list
    QWidget*     fileBox    = new QWidget();
    QHBoxLayout* fileLayout = new QHBoxLayout();
    QPushButton* fileDelete = new QPushButton();
    QLabel*      fileLabel  = new QLabel();
    QLabel*      fileCutoff  = new QLabel();
    QLabel*      fileVoltage  = new QLabel();
    QString fileLayoutName = "file"+QString::number(fileList.length()-1);
    QString cutoff = "Cutoff: "+cutoffName+" ";

    if(cutoffType >1){
        cutoff+= cutoffBox->text();
    }

    fileVoltage->setText(QString::number(mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("fileVoltage")->text().toDouble())+" V");
    fileLabel->setText(fileNameBox->text());
    fileDelete->setText("Remove");
    fileCutoff->setText(cutoff);

    connect(fileDelete, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(fileDelete, fileLayoutName);

    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(fileCutoff);
    fileLayout->addWidget(fileVoltage);
    fileLayout->addStretch();
    fileLayout->addWidget(fileDelete);
    fileBox->setLayout(fileLayout);
    fileBox->setObjectName(fileLayoutName);

    mainWindow->findChild<QWidget *>("FilesBox")->layout()->addWidget(fileBox);

    //readd spacer
    spacer = new QWidget();
    QVBoxLayout* spacerLayout = new QVBoxLayout();

    spacerLayout->addStretch();
    spacer->setLayout(spacerLayout);
    spacer->setObjectName("fileSpacer");
    mainWindow->findChild<QWidget *>("FilesBox")->layout()->addWidget(spacer);
}

//remove the file from the list
void ReaderWindow::on_RemoveFile_clicked(QString name)
{
    QWidget* item = mainWindow->findChild<QWidget *>("FilesBox")->findChild<QWidget *>(name);
    QLayoutItem *child;
    while ((child = item->layout()->takeAt(0)) != 0)  {
        delete child->widget();
        delete child;
    }
    delete item;

    int index = name.remove(0,4).toInt();
    fileList.removeAt(index);
    cutoffValueList.removeAt(index);
    cutoffTypeList.removeAt(index);
    fileVoltageList.removeAt(index);

    if(fileList.length() > 1){
        mainWindow->findChild<QWidget *>("InputPage")->findChild<QComboBox *>("averageCutoffType")->setVisible(true);
    }else{
        mainWindow->findChild<QWidget *>("InputPage")->findChild<QComboBox *>("averageCutoffType")->setVisible(false);
    }
}

//toggle a graph when button is pressed
void ReaderWindow::on_graphToggle_clicked(QString name)
{
    QString temp = name;
    int index = name.remove(0,11).toInt();

    if(plot->graph(index)->visible()){
        mainWindow->findChild<QWidget *>("GraphPage")
            ->findChild<QWidget *>("VisibilityBox")
            ->findChild<QToolButton *>(temp)
            ->setStyleSheet("border-image:url(:/icons/Invisible.png);");
    }else
        mainWindow->findChild<QWidget *>("GraphPage")
                ->findChild<QWidget *>("VisibilityBox")
                ->findChild<QToolButton *>(temp)
                ->setStyleSheet("border-image:url(:/icons/Visible.png);");
    plot->graph(index)->setVisible(!plot->graph(index)->visible());
    plot->replot();
}

//function to add visibility buttons
void ReaderWindow::addVisibilityButtons(int graphID){

    int f = (graphID/2)-1;
    //Make buttons
    QWidget* graphBox = new QWidget();
    QHBoxLayout* graphLayout = new QHBoxLayout();
    QToolButton* cleanShow = new QToolButton();
    QLabel*      graphLabel  = new QLabel();

    QString Name ="";


    QToolButton* originalShow = new QToolButton();
    Name = "GraphToggle"+QString::number(graphID);
    originalShow->setStyleSheet("border-image:url(:/icons/Visible.png);");
    originalShow->setMinimumHeight(30);
    originalShow->setMinimumWidth(30);
    originalShow->setObjectName(Name);
    connect(originalShow, SIGNAL(clicked()), graphSignalMapper, SLOT(map()));
    graphSignalMapper->setMapping(originalShow, Name);

    Name = "GraphToggle"+QString::number(graphID+1);
    cleanShow->setStyleSheet("border-image:url(:/icons/Visible.png);");
    cleanShow->setMinimumHeight(30);
    cleanShow->setMinimumWidth(30);
    cleanShow->setObjectName(Name);
    connect(cleanShow, SIGNAL(clicked()), graphSignalMapper, SLOT(map()));
    graphSignalMapper->setMapping(cleanShow, Name);
    graphLabel->setMinimumWidth(100);

    if(graphID > 0){
        Name = "GraphLayout"+QString::number(f);
        graphLabel->setText("Graph "+QString::number(f+1));
    }else{
        Name = "GraphLayoutAverage";
        graphLabel->setText("Average");
    }
    graphLayout->setObjectName(Name);
    graphLayout->addWidget(originalShow);
    graphLayout->addWidget(cleanShow);

    graphLayout->addWidget(graphLabel);
    graphBox->setLayout(graphLayout);



    mainWindow->findChild<QWidget *>("VisibilityBox")->layout()->addWidget(graphBox);

}

void ReaderWindow::on_About_clicked(){
    QMessageBox::about(this,"About","This is a program developer for Dr. Melko's lab at UNF by John Turner \r\n\
If the program is not working or you have questions email \r\nWhinis@whinis.com\r\n\r\n\r\n V0.2");
}

//average cutoff changed
void ReaderWindow::on_averageCutoffType_currentIndexChanged(int index)
{
    if(index>1){
        mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("averageCutoff")->setVisible(true);
    }else{
        mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("averageCutoff")->setVisible(false);
        mainWindow->findChild<QWidget *>("InputPage")->findChild<QLineEdit *>("averageCutoff")->setText("");
    }
}

//hide all graphs
void ReaderWindow::on_HideAll_clicked()
{
    QString temp ="";
    //Loop through all graphs, change to them hidden, set buttons to closed eye, replot
    for(int i = 0; i<plot->graphCount() ; i++){
        temp="GraphToggle"+QString::number(i);
        QToolButton* t =mainWindow->findChild<QWidget *>("GraphPage")
            ->findChild<QWidget *>("VisibilityBox")
            ->findChild<QToolButton *>(temp);
        if(t){
            t->setStyleSheet("border-image:url(:/icons/Invisible.png);");
            plot->graph(i)->setVisible(false);
        }
    }
    plot->replot();
}


//show all graphs
void ReaderWindow::on_ShowAll_clicked()
{
    QString temp ="";
    //Loop through all graphs, change to them visible, set buttons to open eye, replot
    for(int i = 0; i<plot->graphCount() ; i++){
        temp="GraphToggle"+QString::number(i);
        QToolButton* t =mainWindow->findChild<QWidget *>("GraphPage")
            ->findChild<QWidget *>("VisibilityBox")
            ->findChild<QToolButton *>(temp);
        if(t){
            t->setStyleSheet("border-image:url(:/icons/Visible.png);");
            plot->graph(i)->setVisible(true);
        }
    }
    plot->replot();
}


//show windows for CI or an error
void ReaderWindow::on_actionSet_CI_Range_triggered(){
    QDialog dialog(this);
    // Use a layout allowing to have a label next to each field
    QFormLayout form(&dialog);

    // Add some text above the fields
    form.addRow(new QLabel("Set CI Range"));

    // Add the lineEdits with their respective labels
    QList<QLineEdit *> fields;
    for(int i = 0; i < 2; ++i) {
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        QString label = "";
        if(i==0)
            label = QString("Start Range");
        else
            label = QString("End Range");
        form.addRow(label, lineEdit);

        fields << lineEdit;
    }


    // Add some standard buttons (Cancel/Ok) at the bottom of the dialog
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    // Show the dialog as modal
    if (dialog.exec() == QDialog::Accepted) {
        // If the user didn't dismiss the dialog, do something with the fields
        ciStart =fields.at(0)->text().toDouble();
        ciEnd =fields.at(1)->text().toDouble();
    }
}

void ReaderWindow::on_actionSet_EI_Range_triggered(){


    QDialog dialog(this);
    // Use a layout allowing to have a label next to each field
    QFormLayout form(&dialog);

    // Add some text above the fields
    form.addRow(new QLabel("Set EI Range"));

    // Add the lineEdits with their respective labels
    QList<QLineEdit *> fields;
    for(int i = 0; i < 2; ++i) {
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        QString label = "";
        if(i==0)
            label = QString("Start Range");
        else
            label = QString("End Range");
        form.addRow(label, lineEdit);

        fields << lineEdit;
    }


    // Add some standard buttons (Cancel/Ok) at the bottom of the dialog
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));


    // Show the dialog as modal
    if (dialog.exec() == QDialog::Accepted) {
        // If the user didn't dismiss the dialog, do something with the fields
        eiStart =fields.at(0)->text().toDouble();
        eiEnd =fields.at(1)->text().toDouble();
        qDebug() << QString::number(EI);
    }
}

void ReaderWindow::on_actionCalculate_Ratio_triggered(){

    int vis = 0;
    int id = 0;
    for(int i = 0; i<plot->graphCount() ; i++){
        if(plot->graph(i)->visible() == true){
            vis++;
            id = i;
        }
    }
    if(vis > 1 || vis == 0){
        QMessageBox::warning(this,"Number of Graphs Invalid","This is more than one graph visible, calculation can only be done on one graph at a time");
        return;
    }

    QString GraphMessage = "\r\n";
    if(id%2 == 0){
        GraphMessage += "Orginal";
    }else{
        GraphMessage += "Cleaned";
    }
    if(id>1){
        int f = (id/2)-1;
        GraphMessage += " Graph "+QString::number(f+1);

    }else{
         GraphMessage += " Average ";
    }
    peakBox->setText(peakBox->toPlainText() + GraphMessage);
    QList<double> x = plot->graph(id)->data()->keys();
    QList<QCPData> y = plot->graph(id)->data()->values();
    double sumCI = 0;
    double sumEI = 0;
    for(int i=0; i<x.length();i++){
        double point = x.at(i);
        if(point >= eiStart && point <= eiEnd)
            sumEI+=y.at(i).value;
        if(point >= ciStart && point <= ciEnd)
            sumCI+=y.at(i).value;
    }
    peakBox->setText(peakBox->toPlainText() + "\r\n EI "+QString::number(eiStart)+ " - "+QString::number(eiEnd)+ " : "+QString::number(sumEI));
    peakBox->setText(peakBox->toPlainText() + "\r\n CI "+QString::number(ciStart)+ " - "+QString::number(ciEnd)+ " : "+QString::number(sumCI));

    double ratio = sumCI/sumEI;

    peakBox->setText(peakBox->toPlainText() + "\r\n CI/EI Ratio: "+QString::number(ratio));

}

void ReaderWindow::on_actionCalculate_All_Ratios_triggered(){
    int id = 0;
    QList<double> ratios;
    for(int i = 0; i<plot->graphCount() ; i++){
        if(i<1)
            continue;
        id = i;
        if(id%2 == 0){
            continue;
        }

        QString GraphMessage = "\r\n";
        GraphMessage += "Cleaned";
        int f = (id/2)-1;
        GraphMessage += " Graph "+QString::number(f+1);

        peakBox->setText(peakBox->toPlainText() + GraphMessage);
        QList<double> x = plot->graph(id)->data()->keys();
        QList<QCPData> y = plot->graph(id)->data()->values();
        double sumCI = 0;
        double sumEI = 0;
        for(int i=0; i<x.length();i++){
            double point = x.at(i);
            if(point >= eiStart && point <= eiEnd)
                sumEI+=y.at(i).value;
            if(point >= ciStart && point <= ciEnd)
                sumCI+=y.at(i).value;
        }
        peakBox->setText(peakBox->toPlainText() + "\r\n EI "+QString::number(eiStart)+ " - "+QString::number(eiEnd)+ " : "+QString::number(sumEI));
        peakBox->setText(peakBox->toPlainText() + "\r\n CI "+QString::number(ciStart)+ " - "+QString::number(ciEnd)+ " : "+QString::number(sumCI));

        double ratio = sumCI/sumEI;
        ratios.append(ratio);
        peakBox->setText(peakBox->toPlainText() + "\r\n CI/EI Ratio: "+QString::number(ratio));
    }
    QString Message ="\r\n Ratios:";
    for(int i =0; i<fileList.length(); i++){
        Message+="\r\n"+QString::number(fileVoltageList.at(i))+",\t"+QString::number(ratios.at(i));
    }
     peakBox->setText(peakBox->toPlainText() +Message);
}


void ReaderWindow::on_legendToggle_clicked()
{
    plot->legend->setVisible(!plot->legend->visible());
    plot->replot();
}

void ReaderWindow::on_actionSum_Peaks_triggered(){
    int vis = 0;
    int id = 0;
    for(int i = 0; i<plot->graphCount() ; i++){
        if(plot->graph(i)->visible() == true){
            vis++;
            id = i;
        }
    }
    if(vis > 1 || vis == 0){
        QMessageBox::warning(this,"Number of Graphs Invalid","This is more than one graph visible, calculation can only be done on one graph at a time");
        return;
    }

    QList<double> x = plot->graph(id)->data()->keys();
    QList<QCPData> y = plot->graph(id)->data()->values();

    double sum = 0;
    QString peak = "";

    if(id%2 == 0){
        peak += " Original";
    }else{
        peak += " Cleaned";
    }
    if(id>1){
        int f = (id/2)-1;
        peak += " Graph "+QString::number(f+1);
    }else{
        peak += " Average ";
    }
    peak +="\r\n";

    //cutoff those values
    for(int i=0; i<x.length();i++){
        double temp = y.at(i).value;
        if(temp > 0){
            if(sum == 0)
                peak.append(QString::number(x.at(i)) + " - ");
            sum += temp;
        }else{
            if( sum > 0){
                peak.append(QString::number(x.at(i-1)) + " : "+ QString::number(sum) + "\r\n");
                sum = 0;
            }
        }
        if(sum > 0 && i == (x.length()-1)){
            peak.append(QString::number(x.at(i-1)) + ":\t "+ QString::number(sum) + "\r\n");
        }
    }

    peakBox->setText(peakBox->toPlainText() + "\r\n\r\n" + peak);

}
void ReaderWindow::on_actionSum_Peaks_all_graphs_triggered(){
    int id = 0;
    for(int i = 0; i<plot->graphCount() ; i++){
        id = i;
        if(id%2 == 0){
            continue;
        }

        QList<double> x = plot->graph(id)->data()->keys();
        QList<QCPData> y = plot->graph(id)->data()->values();

        double sum = 0;
        QString peak = "Cleaned";

        if(id>1){
            int f = (id/2)-1;
            peak += " Graph "+QString::number(f+1);
        }else{
            peak += " Average";
        }
        peak +=" Peaks\r\n";

        //cutoff those values
        for(int i=0; i<x.length();i++){
            double temp = y.at(i).value;
            if(temp > 0){
                if(sum == 0)
                    peak.append(QString::number(x.at(i)) + " - ");
                sum += temp;
            }else{
                if( sum > 0){
                    peak.append(QString::number(x.at(i-1)) + " : "+ QString::number(sum) + "\r\n");
                    sum = 0;
                }
            }
            if(sum > 0 && i == (x.length()-1)){
                peak.append(QString::number(x.at(i-1)) + ":\t "+ QString::number(sum) + "\r\n");
            }
        }

        peakBox->setText(peakBox->toPlainText() + "\r\n\r\n" + peak);
    }
}

void ReaderWindow::on_pushButton_4_clicked()
{
    peakBox->setText("");
}

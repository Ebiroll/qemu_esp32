#include "mainwindow.h"

#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QScrollArea>
#include <QMessageBox>
#include <QFile>
#include <QByteArray>
#include <QSettings>
#include <QMenu>
#include <QMenuBar>
#include <QInputDialog>
#include <QTextEdit>

#include <QDebug>
#include <math.h>
#include <QLayout>


#include "qhexview.h"
#include "document/buffer/qmemorybuffer.h"
#include "document/qhexmetadata.h"


MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags):
QMainWindow(parent, flags)
{
	QToolBar *ptb = addToolBar("File");

	QAction *pactOpen = ptb -> addAction("Open...");
	pactOpen -> setShortcut(QKeySequence("Ctrl+O"));
	connect(pactOpen, SIGNAL(triggered()), SLOT(slotOpen()));

	QMenu *pmenu = menuBar() -> addMenu("&File");
	pmenu -> addAction(pactOpen);
	addAction (pactOpen);
	pmenu -> addAction("Go to offset...", this, SLOT(slotToOffset())); 
    pmenu -> addAction("Pretty Text..", this, SLOT(slotPretty()));

    pmenu -> addAction("About...", this, SLOT(slotAbout()));
	pmenu -> addAction("Exit", this, SLOT(close()));

	QHexView *pwgt = new QHexView;
    setCentralWidget(pwgt);

    connect(pwgt, SIGNAL(minimumWidthChanged(int)), this, SLOT(fixWidth(int)));
/*
    QHBoxLayout *layout = new QHBoxLayout(pwgt);
    layout->addWidget(pwgt);

    QTextEdit *textEdit=new QTextEdit();
    layout->addWidget(textEdit);

    setLayout(layout );
*/
	readCustomData();
}


void MainWindow::fixWidth(int minwidth)
{
  this->setFixedWidth(minwidth);
}

// This one does not work, double check
float QByteArrayToFloat(QByteArray f){
    bool ok;
    int sign = 1;

    f = f.toHex(); // Convert to Hex

    qDebug() << "QByteArrayToFloat: QByteArray hex = " << f;

    f = QByteArray::number(f.toLongLong(&ok, 16), 2);    // Convert hex to binary

    if(f.length() == 32) {
        if(f.at(0) == '1') sign =-1;     // If bit 0 is 1 number is negative
        f.remove(0,1);                   // Remove sign bit
    }

    QByteArray fraction = f.right(23);  // Get the fractional part
    double mantissa = 0;
    for(int i = 0; i < fraction.length(); i++){  // Iterate through the array to claculate the fraction as a decimal.
        if(fraction.at(i) == '1')
            mantissa += 1.0 / (pow(2, i+1));
    }

    int exponent = f.left(f.length() - 23).toLongLong(&ok, 2) - 127;     // Calculate the exponent

    qDebug() << "QByteArrayToFloat: float number = "<< QString::number(sign * pow(2, exponent) * (mantissa + 1.0),'f', 5);

    return (sign * pow(2, exponent) * (mantissa + 1.0));
}



typedef  union  {
     float f;
     char data[4];
 } tt;

float toFloat(QByteArray f){
    tt T;

    T.data[0]=f[0];
    T.data[1]=f[1];
    T.data[2]=f[2];
    T.data[3]=f[3];

    return (T.f);
}

void MainWindow::process(const QString &fileName)
{
	QFile file(fileName);


	if(!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::critical(this, "File opening problem", "Problem with open file `" + fileName + "`for reading");
		return;
	}

	QHexView *pcntwgt = dynamic_cast<QHexView *>(centralWidget());



    QHexDocument* document = QHexDocument::fromFile<QMemoryBuffer>(fileName,this);

    pcntwgt->setDocument(document);

    QByteArray tmp=document->read(0, 4);

     bool ok;
     QByteArray f;
    //f = tmp.toHex(); // Convert to Hex

    int num_files =  ((static_cast<unsigned int>(tmp[3]) & 0xFF) << 24)
                     + ((static_cast<unsigned int>(tmp[2]) & 0xFF) << 16)
                     + ((static_cast<unsigned int>(tmp[1]) & 0xFF) << 8)
                     + (static_cast<unsigned int>(tmp[0]) & 0xFF);
    //tmp.toInt();

     qDebug() << "number  = " << num_files;



     qDebug() << "QByteArray hex = " << f;





    QHexMetadata* hexmetadata = document->metadata();
    int pos;
    for (int i=0;i<num_files;i++) {
        pos=4+40*i;

        QByteArray name=document->read(pos, 32);
        qDebug() << "Name = " << name;

        QByteArray offset=document->read(pos+32+4, 4);
        qDebug() << "Offset = " << offset;



        int line=pos/16;
        int start=pos-16*line;
        hexmetadata->background(line, start, 16, QColor(255-i*10, i*20, 0));
        hexmetadata->background(line+1, 0, 16, QColor(255-i*10, i*20, 0));
        hexmetadata->background(line+2, 0, start, QColor(255-i*10, i*20, 0));


        int off =  ((static_cast<unsigned int>(offset[3]) & 0xFF) << 24)
                    + ((static_cast<unsigned int>(offset[2]) & 0xFF) << 16)
                    + ((static_cast<unsigned int>(offset[1]) & 0xFF) << 8)
                    + (static_cast<unsigned int>(offset[0]) & 0xFF);



         hexmetadata->comment(line+2,start,4, QString::number(off));

        qDebug() << "Off = " << off;



        QByteArray version=document->read(off, 4);
        qDebug() << "Version = " << version;
        if (version[0]==0x02 && version[1]==0x0 && version[3]==0x0 && version[3]==0x0) {
            qDebug() << "============_____________============ ";
             float x[2];
             float y[2];
             float z[2];

             printf("Off 0x%X\n",off+28);


             QByteArray minx=document->read(off+8, 4);
             QByteArray miny=document->read(off+12, 4);
             QByteArray minz=document->read(off+16, 4);


             x[0]=toFloat(minx);
             y[0]=toFloat(miny);
             z[0]=toFloat(minz);


             QByteArray maxX=document->read(off+20, 4);
             QByteArray maxY=document->read(off+24, 4);
             QByteArray maxZ=document->read(off+28, 4);

             qDebug() << "VAL \n" << maxY.toHex();




             x[1]=toFloat(maxX);
             y[1]=toFloat(maxY);
             z[1]=toFloat(maxZ);


              printf("X  %.2f - %.2f\n",x[0],x[1]);
              printf("Y  %.2f - %.2f\n",y[0],y[1]);
              printf("Z  %.2f - %.2f\n",z[0],z[1]);

        }


    }

//    hexmetadata->background(3, 0, 5, QColor(255, 0, 0));      // Highlight background to line 6, from 0 to 5
//    hexmetadata->foreground(8, 0, 15, QColor(0, 0, 255)); // Highlight foreground to line 8, from 0 to 15
//    hexmetadata->comment(9,3,3, "I'm a comment!");


    //pcntwgt->setFixedSize(800,240);
    //pcntwgt->resize(1024,680);

    //pcntwgt -> clear();

    //QByteArray arr = file.readAll();
    //pcntwgt -> setData(new QHexView::DataStorageArray(arr));

}


void MainWindow::slotOpen()
{
	QString dir = QDir::currentPath();
	QSettings settings("QHexView", "QHexView");

	if(settings.value("QHexView/PrevDir").toString().length())
		dir = settings.value("QHexView/PrevDir").toString();

	QString fileName = QFileDialog::getOpenFileName(this, "Select file", dir);
	if(!fileName.isEmpty())
	{
		process(fileName);
		QFileInfo info(fileName);
		settings.setValue("QHexView/PrevDir", info.absoluteDir().absolutePath());

		setWindowTitle(info.fileName());
	}
}


void MainWindow::slotAbout()
{
	QString message;
    message = "<center><b>Pkg hex viewer</b></center>";
    message += "<center>olof.astrand@gmail.com</center>";
	QMessageBox::about(this, "About", message);
}


void MainWindow::slotPretty() {

    QHexView *pcntwgt = dynamic_cast<QHexView *>(centralWidget());
    QHexCursor* cur = pcntwgt->document()->cursor();
    //cur->currentLine();
    int pos=cur->currentLine()*16+cur->currentColumn();

    QByteArray text=pcntwgt->document()->read(pos, 512);

    QString message=text;
    QMessageBox::about(this, "Text", message);


}




void MainWindow::slotToOffset()
{
	bool ok;
	int offset = QInputDialog::getInt(0, "Offset", "Offset:", 0, 0, 2147483647, 1, &ok);

	if(ok)
	{
		QHexView *pcntwgt = dynamic_cast<QHexView *>(centralWidget());


        QHexCursor* cur = pcntwgt->document()->cursor();
        int reminder=offset%16;
        cur->moveTo(offset/16,reminder,0);

        //pcntwgt -> showFromOffset(offset);
	}
}


void MainWindow::closeEvent(QCloseEvent *pevent)
{
	saveCustomData();
	QWidget::closeEvent(pevent);
}


void MainWindow::saveCustomData()
{
	QSettings settings("QHexView", "QHexView");
	settings.setValue("MainWindow/geometry", saveGeometry());
}


void MainWindow::readCustomData()
{
	QSettings settings("QHexView", "QHexView");
	restoreGeometry(settings.value("MainWindow/geometry").toByteArray()); 
}


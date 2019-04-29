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
#include <QSpinBox>

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

    QAction *pactDiff = ptb -> addAction("Diff.");
    pactDiff -> setShortcut(QKeySequence("Ctrl+D"));
    connect(pactDiff, SIGNAL(triggered()), SLOT(slotDiff()));

	QMenu *pmenu = menuBar() -> addMenu("&File");
	pmenu -> addAction(pactOpen);
	addAction (pactOpen);

    //QAction *pactGoto = ptb -> addAction("Goto offset.");
    //pactGoto -> setShortcut(QKeySequence("Ctrl+G"));
    //pmenu -> addAction(pactGoto);
    //addAction (pactGoto);

    pmenu -> addAction("Go to offset...", this, SLOT(slotToOffset()));
    pmenu -> addAction("Pretty Text..", this, SLOT(slotPretty()));

    pmenu -> addAction("About...", this, SLOT(slotAbout()));
	pmenu -> addAction("Exit", this, SLOT(close()));

	QHexView *pwgt = new QHexView;
    setCentralWidget(pwgt);

   // Old behaviour
   // connect(pwgt, SIGNAL(minimumWidthChanged(int)), this, SLOT(fixWidth(int)));
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



    QHexMetadata* hexmetadata = document->metadata();
    int pos=0x8000;

    QByteArray name=document->read(pos+12, 20);
    //
    int line,start;
    for (int i=0;i<10 && (name[0]!=0xff) ;i++) {
        pos=0x8000+32*i;

        name=document->read(pos+12, 20);
        //qDebug() << "Name = " << name;

        QByteArray offset0=document->read(pos+4, 1);
        QByteArray offset1=document->read(pos+5, 1);
        QByteArray offset2=document->read(pos+6, 1);
        QByteArray offset3=document->read(pos+7, 1);

        //qDebug() << "Offset = " << offset3 + offset2 + offset1 + offset0 ;

        line=pos/16;
        start=pos-16*line;
        hexmetadata->background(line, 12, 4, QColor(0, 255-i*20, 0));
        hexmetadata->background(line+1, 0, 16, QColor(0, 255-i*20, 0));


        //int off =  ((static_cast<unsigned int>(offset[3]) & 0xFF) << 24)
        //            + ((static_cast<unsigned int>(offset[2]) & 0xFF) << 16)
        //            + ((static_cast<unsigned int>(offset[1]) & 0xFF) << 8)
        //            + (static_cast<unsigned int>(offset[0]) & 0xFF);

         QString part_offset=QString ("0x") + offset3.toHex() + offset2.toHex() + offset1.toHex() + offset0.toHex();

         hexmetadata->comment(line,start+4,4, part_offset);

         //qDebug() << "Off = " << off;

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


void MainWindow::slotDiff()
{
    QString dir = QDir::currentPath();
    QSettings settings("QHexView", "QHexView");

    if(settings.value("QHexView/PrevDir").toString().length())
        dir = settings.value("QHexView/PrevDir").toString();

    QString fileName="esp32flash.sav";

    QFile file(fileName);


    if(!file.open(QIODevice::ReadOnly))
    {
        fileName = QFileDialog::getOpenFileName(this, "Select file", dir);
        if(!fileName.isEmpty())
        {
            QFileInfo info(fileName);
            settings.setValue("QHexView/PrevDir", info.absoluteDir().absolutePath());
        }

        QFile file2(fileName);

        if(!file2.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "File opening problem", "Problem with open file `" + fileName + "`for reading");
            return;
        }
        //file=file2;
    }


    QHexDocument* document = QHexDocument::fromFile<QMemoryBuffer>(fileName,this);
    QHexView *pcntwgt = dynamic_cast<QHexView *>(centralWidget());
    QHexDocument* cur_doc = pcntwgt->document();

    int pos=0,line,start;
    QHexMetadata* hexmetadata = cur_doc->metadata();
    QByteArray t1,t2;
    while(pos<cur_doc->length() && document->length()) {

         t1=document->read(pos, 1);
         t2=cur_doc->read(pos, 1);
         line=pos/16;
         start=pos%16;
         if (t1!=t2) {
             // Some bug prevents setting 1 byte and then the next
             hexmetadata->background(line, start, 2, QColor(255, 0, 0));
             hexmetadata->comment(line,start,1, t1.toHex());
         }
         pos++;
    }

    delete document;

}


void MainWindow::slotAbout()
{
	QString message;
    message = "<center><b>esp32 flash file hex viewer</b></center>";
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



static QString getHex(QWidget *parent,
                      const QString &title,
                      const QString &label,
                      int value = 0,
                      int min = -2147483647,
                      int max = 2147483647,
                      int step = 1,
                      bool *ok = Q_NULLPTR,
                      Qt::WindowFlags flags = Qt::WindowFlags()){
    QInputDialog dialog(parent, flags);
    dialog.setWindowTitle(title);
    dialog.setLabelText(label);
    dialog.setIntRange(min, max);
    dialog.setIntValue(value);
    dialog.setIntStep(step);
    QSpinBox *spinbox = dialog.findChild<QSpinBox*>();
    spinbox->setDisplayIntegerBase(16);

    bool ret = dialog.exec() == QDialog::Accepted;
    if (ok)
        *ok = ret;
    return spinbox->text();
}

void MainWindow::slotToOffset()
{
	bool ok;
    //int offset = QInputDialog::getInt(0, "Offset", "Offset:", 0, 0, 2147483647, 1, &ok);

    QString hex= getHex(Q_NULLPTR, "Offset", "hex", 0x8000, 0);

    int offset = hex.toUInt(&ok,16);
	if(ok)
	{
		QHexView *pcntwgt = dynamic_cast<QHexView *>(centralWidget());

        QHexCursor* cur = pcntwgt->document()->cursor();
        int reminder=offset%16;
        cur->moveTo(offset/16,reminder,0);

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


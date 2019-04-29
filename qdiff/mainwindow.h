#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <QMainWindow>



typedef struct pkg_entry
{
    char file[32];
    uint32_t size;
    uint32_t offset;
} pkg_entry_t;

typedef struct pkg_content
{
    uint32_t n_files;
    pkg_entry_t files[1];
} pkg_content_t;


class MainWindow: public QMainWindow
{
	Q_OBJECT
	public:
		MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
        void process(const QString &fileName);

	protected:
		virtual void closeEvent(QCloseEvent *);

	private:
		void saveCustomData();
		void readCustomData();

	private slots:
		void slotOpen();
        void slotDiff();
		void slotAbout();
        void slotPretty();
		void slotToOffset();
        void fixWidth(int minwidth);

};


#endif

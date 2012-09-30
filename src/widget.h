#ifndef WIDGET_H
#define WIDGET_H

#include <QFile>
#include <QSettings>
#include <QDateTime>
#include <QtNetwork>
#include <QNetworkAccessManager>

#include "ui_widget.h"

#define SPEED_ESTIMATION_TIME 10
#define MEGABYTE 1048576.0
#define KILOBYTE 1024.0

struct FileDescription
{
    QString fileName;
    QDateTime dateTime;
    quint32 fileSize;
};

enum iStatus { NotStarted, Installing, AbortByUser, AbortByApplication, AbortWithError, Finished };

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
	virtual ~Widget();

#if defined(Q_OS_MAC)
    static const QString xplaneFileName() { return "X-Plane.app"; }
#elif defined(Q_OS_LINUX)
    static const QString xplaneFileName() { return "X-Plane"; }
#elif defined(Q_OS_WIN32)
    static const QString xplaneFileName() { return "X-Plane.exe"; }
#endif

    static const QString defaultDir() { return QDir::homePath(); }
    static const QString defaultInstallDir() { return defaultDir() + "/Desktop/X-Plane"; }
    static const QString customSceneryDirName() { return "Custom Scenery"; }
    static const QString ruSceneryDirName() { return "ruscenery"; }
    static const QString defaultRuSceneryUrl() { return "http://www.x-plane.su/ruscenery/"; }
    static const QString defaultRuSceneryUpdateUrl() { return "http://www.x-plane.su/ruscenery/update/"; }
    static const QString ruSceneryVersionFileName() { return "ruscenery.ver"; }

    static const quint32 maxVersionFileSize = 3*1024*1024;
    static const quint32 maxDownloadFileSize = 100*1024*1024;
    
private:
    Ui::Widget ui;

    QSettings *settings;
    QNetworkReply *networkReply;
    QNetworkAccessManager networkAccessManager;

    QString xplaneDir;
    QString rusceneryDir;

    QString url;
    QString uurl;
    QString version;
    QString msg;
    QString msgTop;
    QString msgBottom;

    quint64 overallDownloadedBytesOnTimer;
    quint32 currentDownloadSpeed;
    quint32 currentDownloadSize;
    quint32 currentDownloadedBytes;
    quint64 overallDownloadSize;
    quint64 overallDownloadedBytes;

    iStatus status;
    
    QFile *file;
    QList<FileDescription> iFileList;

    QTimer *timer;
    quint32 timerCounter;


    QString prepareUrl(QString url);
    void setInstalling(bool state = false);

    void start_install();
    void start_download();
    void abort_install(iStatus s, QString e = "");

    void setUpdate(QString url = "");

    void showMessage(QString msg);
    void showError(QString e);

private slots:
    void on_toolButton_Select_clicked();
    void on_pushButton_Install_clicked();
	void on_lineEdit_textChanged(QString text);

    void on_download_readyRead();
    void on_download_finished();
    void on_download_progress(qint64 bytesReceived, qint64 bytesTotal);
    
    void on_vf_readyRead();
    void on_vf_downloaded();

    void on_timer_timeout();
};

#endif // WIDGET_H

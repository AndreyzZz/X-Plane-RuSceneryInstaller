#ifndef WIDGET_H
#define WIDGET_H

#include <QFile>
#include <QSettings>
#include <QDateTime>
#include <QtNetwork>
#include <QNetworkAccessManager>

#include "ui_widget.h"

struct FileDescription
{
    QString fileName;
    QDateTime dateTime;
    quint32 fileSize;
};

enum iStatus { NotStarted, Installing, AbortByUser, AbortByApplication, AbortWithError, Finished};

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
	virtual ~Widget();

#if defined(Q_OS_MAC)
    static const QString defaultDir() { return QDir::homePath(); }
    static const QString defaultInstallDir() { return defaultDir() + "/X-Plane.app"; }
    static const QString xplaneFileName() { return "Contents/MacOS/X-Plane"; }
    static const QString customSceneryDirName() { return "Contents/Custom Scenery"; }
#elif defined(Q_OS_LINUX)
    static const QString defaultDir() { return QDir::homePath(); }
    static const QString defaultInstallDir() { return defaultDir() + "/X-Plane"; }
    static const QString xplaneFileName() { return "X-Plane"; }
    static const QString customSceneryDirName() { return "Custom Scenery"; }
#elif defined(Q_OS_WIN32)
    static const QString defaultDir() { return QDir::rootPath(); }
    static const QString defaultInstallDir() { return defaultDir() + "X-Plane"; }
    static const QString xplaneFileName() { return "X-Plane.exe"; }
    static const QString customSceneryDirName() { return "Custom Scenery"; }
#endif

    static const QString ruSceneryDirName() { return "ruscenery"; }
    static const QString defaultRuSceneryUrl() { return "http://www.x-plane.su/ruscenery/"; }
    static const QString defaultRuSceneryUpdateUrl() { return "http://www.x-plane.su/ruscenery/update/"; }
    static const QString ruSceneryVersionFileName() { return "ruscenery.ver"; }

    static const quint32 maxVersionFileSize = 3*1024*1024;
    static const quint32 maxDownloadFileSize = 100*1024*1024;
    
private:
    Ui::Widget ui;

    QSettings settings;
    QNetworkReply *networkReply;
    QNetworkAccessManager networkAccessManager;    

    QString xplaneDir;
    QString rusceneryDir;
    QString xFileName;

    QString url;
    QString uurl;
    QString version;
    QString msg;
    QString msgTop;
    QString msgBottom;

    QString currentDownloadFileName;
    quint32 currentDownloadSize;
    quint32 currentDownloadedBytes;
    quint64 overallDownloadSize;
    quint64 overallDownloadedBytes;

    bool isInstalling;

    iStatus status;
    
    QFile *file;
    QList<FileDescription> iFileList;

    QString prepareUrl(QString url);

private slots:
	void on_pushButton_Select_clicked();
    void on_pushButton_Install_clicked();
	void on_lineEdit_textChanged(QString text);

    void setInstalling(bool state);

    void start_install();
    void abort_install(iStatus s, QString e = "");

    void start_download();
    void on_download_readyRead();
    void on_download_finished();
    void on_download_progress(qint64 bytesReceived, qint64 bytesTotal);
    
    void on_vf_readyRead();
    void on_vf_downloaded();

    void setUpdate(QString url);

    void showMessage(QString msg);
    void showError(QString e);
};

#endif // WIDGET_H

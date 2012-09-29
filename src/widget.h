#ifndef WIDGET_H
#define WIDGET_H

#include <QFile>
#include <QSettings>
#include <QStringList>
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

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
	virtual ~Widget();

#if defined(Q_OS_MAC)
    static const QString defaultDir() { return QDir::homePath(); }
    static const QString defaultInstallDir() { return defaultDir() + "/X-Plane.app"; }
    static const QString xplaneFileName() { return "MacOS/X-Plane"; }
#elif defined(Q_OS_LINUX)
    static const QString defaultDir() { return QDir::homePath(); }
    static const QString defaultInstallDir() { return defaultDir() + "/X-Plane"; }
    static const QString xplaneFileName() { return "X-Plane"; }
#elif defined(Q_OS_WIN32)
    static const QString defaultDir() { return QDir::rootPath(); }
    static const QString defaultInstallDir() { return defaultDir() + "X-Plane"; }
    static const QString xplaneFileName() { return "X-Plane.exe"; }
#endif
    
    static const QString customSceneryDirName() { return "Custom Scenery"; }
    static const QString ruSceneryDirName() { return "ruscenery"; }

    static const QString defaultRuSceneryUrl() { return "http://www.x-plane.su/ruscenery/"; }
    static const QString defaultRuSceneryUpdateUrl() { return "http://www.x-plane.su/ruscenery/update/"; }
    static const QString ruSceneryVersionFileName() { return "ruscenery.ver"; }

    static const quint32 maxVersionFileSize = 1048576;
    static const quint32 maxDownloadFileSize = 104857600;
    
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

    qreal downloadProgress;
    quint32 downloadSize;
    quint32 downloadedBytes;

    bool isInstalling;
    bool abortedByUser;
    
    QFile *file;
    QList<FileDescription> iFileList;

    QString prepareUrl(QString url);

private slots:
	void on_pushButton_Select_clicked();
    void on_pushButton_Install_clicked();
	void on_lineEdit_textChanged(QString text);

    void setInstalling(bool state);

    void start_install();
    void abort_install(bool user = false);

    void start_download();
    void on_download_readyRead();
    void on_download_finished();
    void on_download_progress(qint64 bytesReceived, qint64 bytesTotal);
    
    void on_vf_readyRead();
    void on_vf_downloaded();

    void showMessage(QString msg);
    void showError(QString e);
};

#endif // WIDGET_H

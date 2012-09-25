#include "widget.h"

#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    setWindowIcon(QIcon(":/Resources/Icons/icon"));
    setWindowTitle("RuScenery Installer " + QApplication::applicationVersion());

    setInstalling(false);
    showMessage(QString::null);

    xplaneDir = prepareUrl(settings.value("DIR", defaultInstallDir()).toString());
    url = prepareUrl(settings.value("URL", defaultRuSceneryUrl()).toString());

    ui.lineEdit->setText(xplaneDir);
}

Widget::~Widget()
{
    settings.setValue("DIR", xplaneDir);
    settings.setValue("URL", url);
}

void Widget::showError(QString e)
{
    qDebug() << QDateTime::currentDateTime() << tr("Error") << e;
    QMessageBox::critical(this, tr("Error"), e);
}

void Widget::showMessage(QString msg)
{
    ui.label_Info->setVisible(!msg.isNull() && msg != "");
    ui.label_Info->setText(msg);
}

QString Widget::prepareUrl(QString url)
{
    url.replace("\\", "/");
    if (url.right(1) != "/") url.append("/");
    return url;
}

void Widget::setInstalling(bool state)
{
    isInstalling = state;

    ui.progressBar_Current->setEnabled(state);
    ui.progressBar_Overall->setEnabled(state);
    ui.label_Download->setEnabled(state);

    ui.progressBar_Current->setVisible(state);
    ui.progressBar_Overall->setVisible(state);
    ui.label_Download->setVisible(state);

    ui.lineEdit->setEnabled(!state);
    ui.pushButton_Select->setEnabled(!state);

    ui.pushButton_Install->setEnabled(true);
    ui.pushButton_Install->setText(state ? tr("Abort"):tr("Install"));
}


//------------------ Button click handlers ---------------------

void Widget::on_pushButton_Select_clicked()
{
    QString xdir = QFileDialog::getExistingDirectory(this, tr("Select X-Plane directory"), defaultDir(),
					QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!xdir.isNull()) ui.lineEdit->setText(xdir);
}

void Widget::on_pushButton_Install_clicked()
{
    ui.pushButton_Install->setEnabled(false);

    if (isInstalling) abort_install();
    else              start_install();
}

void Widget::on_lineEdit_textChanged(QString text)
{
    ui.pushButton_Install->setEnabled(false);

    xplaneDir = prepareUrl(text);
    rusceneryDir = prepareUrl(xplaneDir + customSceneryDirName() + "/" + ruSceneryDirName());

    if (QFile::exists(xplaneDir + xplaneFileName()) &&
        QDir(xplaneDir + customSceneryDirName()).exists())
    {
        ui.pushButton_Install->setEnabled(true);
    }
}



//---------------------- Install ------------------------------
void Widget::start_install()
{
    setInstalling(true);

    qDebug() << QDateTime::currentDateTime() << tr("Installation started");

    downloadProgress = 0;
    downloadSize = 0;
    downloadedBytes = 0;

    msg = "";
    msgTop = "";
    msgBottom = "";

    version = "";

    uurl = defaultRuSceneryUpdateUrl();

    file = 0;
    iFileList.clear();

    ui.progressBar_Current->setValue(0);
    ui.progressBar_Overall->setValue(0);

    ui.label_Download->setText(tr("Downloading %1").arg(ruSceneryVersionFileName()));

    networkReply = networkAccessManager.get(QNetworkRequest(QUrl(url + ruSceneryVersionFileName(), QUrl::TolerantMode)));

    connect(networkReply, SIGNAL(readyRead()), this, SLOT(on_vf_readyRead()));
    connect(networkReply, SIGNAL(finished()), this, SLOT(on_vf_downloaded()));
    connect(networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(on_download_progress(qint64,qint64)));
}

void Widget::abort_install()
{
    if (networkReply && !networkReply->isFinished())
    {
        networkReply->abort();
        return;
    }

    if (networkReply)
    {
        networkReply->deleteLater();
        networkReply = 0;
    }

    if (file)
    {
        if (file->isOpen()) file->close();
        delete file;
        file = 0;
    }

    setInstalling(false);
    showMessage(QString::null);
}


//---------------------- Download Version File ------------------------------

void Widget::on_vf_readyRead()
{
    if (networkReply->bytesAvailable() > maxVersionFileSize || downloadedBytes > maxVersionFileSize)
    {
        showError(tr("Unable to download version file. File too big."));

        abort_install();
        return;
    }
    
    while (networkReply->canReadLine())
    {
        QByteArray array = networkReply->readLine();
        downloadedBytes += array.size();
        
        QString line = QTextCodec::codecForName("Windows-1251")->toUnicode(array).simplified();
        QStringList list = line.split(' ');
        
        if (list.size() > 0 && list.at(0) != "#")
        {
            QString cmd = list.at(0).toLower();
            
            if (cmd == ";u" && list.size() == 2) url = prepareUrl(list.at(1));
            else if (cmd == ";d" && list.size() == 2) uurl = prepareUrl(list.at(1));
            else if (cmd == ";v" && list.size() == 2) version = list.at(1);
            else if (cmd == ";s") msg = line.right(line.size() - cmd.size());
            else if (cmd == ";t") { msgTop = line.right(line.size() - cmd.size()); showMessage(msgTop); }
            else if (cmd == ";b") msgBottom = line.right(line.size() - cmd.size());
            else if (list.size() >= 4)
            {
                FileDescription fd;
                fd.fileName = list[0].replace('\\', '/');
                fd.fileSize = list.at(1).toInt();
                
                if (fd.fileSize == 0) continue;
                //fd.dateTime = QDateTime::fromString(list.at(2) + " " + list.at(3), "");

                iFileList.append(fd);
            }
        }
    }
}

void Widget::on_vf_downloaded()
{
    if (networkReply->error())
    {
        showError(networkReply->errorString());

        abort_install();
        return;
    }

    networkReply->deleteLater();
    networkReply = 0;

    for (int i = iFileList.size() - 1; i >= 0; --i)
    {
        QFile file(rusceneryDir + iFileList.at(i).fileName);
        if (file.exists() || file.size() == iFileList.at(i).fileSize)
        {
            iFileList.removeAt(i);
        }
        else
        {
            downloadSize +=  iFileList.at(i).fileSize;
        }
    }

    if (iFileList.size() > 0)
    {
        QString size = QString::number((qreal)downloadSize/(1024*1024), 'f', 2);

        int ret = QMessageBox::question(this, QApplication::applicationName(),
                  tr("It's need to download %1 MiB of files.\n Do you want to continue?").arg(size),
                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (ret == QMessageBox::Yes)
        {
            start_download();
        }
        else abort_install();
    }
    else
    {
        abort_install();
    }
}



//---------------------- Download Library Files ------------------------------
void Widget::start_download()
{
    showMessage(msg);

    QString size = QString::number((qreal)iFileList.first().fileSize/(1024*1024),'f', 2);
    ui.label_Download->setText(tr("Downloading %1 - %2 MiB").arg(iFileList.first().fileName).arg(size));

    file = new QFile(rusceneryDir + iFileList.first().fileName);

    if ((file->exists() && !file->remove()) ||
        !QDir().mkpath(QFileInfo(file->fileName()).path()) ||
        !file->open(QIODevice::WriteOnly))
    {
        showError(file->errorString());

        abort_install();
        return;
    }

    downloadedBytes = 0;

    networkReply = networkAccessManager.get(QNetworkRequest(QUrl(uurl + iFileList.first().fileName, QUrl::TolerantMode)));

    connect(networkReply, SIGNAL(readyRead()), this, SLOT(on_download_readyRead()));
    connect(networkReply, SIGNAL(finished()), this, SLOT(on_download_finished()));
    connect(networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(on_download_progress(qint64,qint64)));
    connect(networkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(on_download_error(QNetworkReply::NetworkError)));
}

void Widget::on_download_readyRead()
{
    quint32 bytes = networkReply->bytesAvailable();

    if (bytes != file->write(networkReply->readAll()))
    {
        showError(file->errorString());
        abort_install();
    }
    else
    {
        downloadedBytes += bytes;
        if (downloadedBytes > maxDownloadFileSize)
        {
            showError(file->errorString());
            abort_install();
        }
    }
}

void Widget::on_download_finished()
{
    if (networkReply->error())
    {
        showError(networkReply->errorString());

        abort_install();
        return;
    }
    else if (file->size() != iFileList.first().fileSize)
    {
        showError(networkReply->errorString());

        abort_install();
        return;
    }

    networkReply->deleteLater();
    networkReply = 0;

    file->close();
    delete file;
    file = 0;

    downloadProgress += (qreal)downloadedBytes/downloadSize*100;
    ui.progressBar_Overall->setValue(qCeil(downloadProgress));

    iFileList.removeFirst();

    if (iFileList.size() > 0)
    {
        start_download();
    }
    else
    {
        showMessage(msgBottom);

        qDebug() << QDateTime::currentDateTime() << tr("All files downloaded");

        abort_install();

        if (version != "" && version != QApplication::applicationVersion())
        {
            int ret = QMessageBox::question(this, QApplication::applicationName(),
                      tr("New %1 version availables.\n Do you want to download?").arg(version),
                      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

            if (ret == QMessageBox::Yes)
            {
                QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
            }
        }
    }
}


void Widget::on_download_progress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal == -1 && iFileList.size() > 0) bytesTotal = iFileList.first().fileSize;

    if (bytesTotal == 0) ui.progressBar_Current->setValue(0);
    else ui.progressBar_Current->setValue(qCeil(bytesReceived*100.0/bytesTotal));
}

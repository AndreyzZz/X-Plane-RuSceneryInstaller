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
    setWindowTitle("RuScenery Installer " + QApplication::applicationVersion());

    setInstalling(false);
    setUpdate("");
    showMessage("");

    xplaneDir = prepareUrl(settings.value("DIR", defaultInstallDir()).toString());
    url = prepareUrl(settings.value("URL", defaultRuSceneryUrl()).toString());

    ui.lineEdit->setText(xplaneDir);
}

Widget::~Widget()
{
    if (status == Installing) abort_install(AbortByApplication);

    if (ui.pushButton_Install->isEnabled())
    {
        settings.setValue("DIR", xplaneDir);
        settings.setValue("URL", url);
    }
}

void Widget::showError(QString e)
{
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
    return (url.right(1) != "/") ? url.append("/") : url;
}

void Widget::setInstalling(bool state)
{
    qDebug() << QDateTime::currentDateTime() << "setInstalling()" << state;

    ui.progressBar_Current->setEnabled(state);
    ui.progressBar_Overall->setEnabled(state);
    ui.label_ETA->setEnabled(state);
//    ui.label_Download->setEnabled(state);

    ui.progressBar_Current->setVisible(state);
    ui.progressBar_Overall->setVisible(state);
    ui.label_ETA->setVisible(state);
//    ui.label_Download->setVisible(state);

    ui.lineEdit->setEnabled(!state);
    ui.toolButton_Select->setEnabled(!state);

    ui.pushButton_Install->setEnabled(true);
    ui.pushButton_Install->setText(state ? tr("Abort"):tr("Install"));
}

void Widget::setUpdate(QString url)
{
    if (!url.isEmpty())
    {
        ui.label_Update->setText(tr("New Installer available") + QString(" %1 <a href=\"%2\">%2</a>").arg(version).arg(url));
    }

    ui.label_Update->setEnabled(!url.isEmpty());
    ui.label_Update->setVisible(!url.isEmpty());
}


//------------------ Button click handlers ---------------------

void Widget::on_toolButton_Select_clicked()
{
    QString xdir = QFileDialog::getExistingDirectory(this, tr("Select X-Plane directory"), defaultDir(),
					QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!xdir.isEmpty()) ui.lineEdit->setText(xdir);
}

void Widget::on_pushButton_Install_clicked()
{
    qDebug() << QDateTime::currentDateTime() << "pushButton_Install clicked";

    ui.pushButton_Install->setEnabled(false);

    if (status == Installing) abort_install(AbortByUser);
    else start_install();
}

void Widget::on_lineEdit_textChanged(QString text)
{
    ui.pushButton_Install->setEnabled(false);

    xplaneDir = prepareUrl(text);
    rusceneryDir = prepareUrl(xplaneDir + customSceneryDirName() + "/" + ruSceneryDirName());

    qDebug() << QDateTime::currentDateTime() << "Selecting directories" << xplaneDir << rusceneryDir;

    if (QFile::exists(xplaneDir + xplaneFileName()) && QDir(xplaneDir + customSceneryDirName()).exists())
    {
        ui.pushButton_Install->setEnabled(true);
    }
}



//---------------------- Install ------------------------------
void Widget::start_install()
{
    status = Installing;
    setInstalling(true);

    qDebug() << QDateTime::currentDateTime() << tr("Installation started");

    overallDownloadedBytesOnTimer = 0;
    currentDownloadSpeed = 0;
    currentDownloadSize = 0;
    currentDownloadedBytes = 0;
    overallDownloadSize = 0;
    overallDownloadedBytes = 0;

    msg = "";
    msgTop = "";
    msgBottom = "";
    version = QApplication::applicationVersion();
    uurl = defaultRuSceneryUpdateUrl();

    ui.progressBar_Current->setValue(0);
    ui.progressBar_Overall->setValue(0);
    ui.label_Download->setText(tr("Downloading %1").arg(ruSceneryVersionFileName()));

    timerCounter = 0;
    timer = new QTimer(this);
    timer->start(1000);

    file = 0;
    iFileList.clear();

    networkReply = networkAccessManager.get(QNetworkRequest(QUrl(url + ruSceneryVersionFileName(), QUrl::TolerantMode)));
    qDebug() << QDateTime::currentDateTime() << "Starting networkRequest" << networkReply->request().url();

    connect(networkReply, SIGNAL(readyRead()), this, SLOT(on_vf_readyRead()));
    connect(networkReply, SIGNAL(finished()), this, SLOT(on_vf_downloaded()));
    connect(networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(on_download_progress(qint64,qint64)));

    connect(timer, SIGNAL(timeout()), this, SLOT(on_timer_timeout()));
}

void Widget::abort_install(iStatus s, QString e)
{
    if (status != AbortByUser && status != AbortByApplication) status = s;

    if (s == AbortWithError)
    {
        qDebug() << QDateTime::currentDateTime() << e;

        if (status != AbortByUser && status != AbortByApplication)
        {
            QMessageBox::critical(this, QString("%1 %2").arg(QDateTime::currentDateTime().toString()).arg(tr("Error")), e);
            ui.label_Download->setText(e);
        }
    }
    else if (s == AbortByUser) ui.label_Download->setText(tr("Aborted by user"));
    else if (s == Finished) ui.label_Download->setText(tr("Installation finished"));


    if (timer && timer->isActive())
    {
        qDebug() << QDateTime::currentDateTime() << "Stopping estimation timer";
        timer->stop();
        delete timer;
        timer = 0;
    }

    if (networkReply && !networkReply->isFinished())
    {
        qDebug() << QDateTime::currentDateTime() << "Aboarting network request";
        networkReply->abort();
        return;
    }

    if (networkReply)
    {
        qDebug() << QDateTime::currentDateTime() << "Closing networkReply" << networkReply->url();
        networkReply->deleteLater();
        networkReply = 0;
    }

    if (file)
    {
        qDebug() << QDateTime::currentDateTime() << "Closing file" << file->fileName();
        if (file->isOpen()) file->close();
        delete file;
        file = 0;
    }

    setInstalling(false);
    showMessage(QString::null);

    status = NotStarted;
}


//---------------------- Download Version File ------------------------------

void Widget::on_vf_readyRead()
{
    if (networkReply->bytesAvailable() > maxVersionFileSize || currentDownloadedBytes > maxVersionFileSize)
    {
        abort_install(AbortWithError, tr("Invalid file size %1 url:%2").arg(ruSceneryVersionFileName()).arg(networkReply->url().toString()));
        return;
    }

    while (networkReply->canReadLine())
    {
        QByteArray array = networkReply->readLine();
        currentDownloadedBytes += array.size();
        
        QString line = QTextCodec::codecForName("Windows-1251")->toUnicode(array).simplified();
        QStringList list = line.split(' ');
        
        if (list.size() > 0 && list.at(0) != "#")
        {
            QString cmd = list.at(0).toLower();
            
            if (cmd == ";u" && list.size() == 2) url = prepareUrl(list.at(1));
            else if (cmd == ";d" && list.size() == 2) uurl = prepareUrl(list.at(1));
            else if (cmd == ";v" && list.size() == 2) version = list.at(1);
            else if (cmd == ";s") msg = line.right(line.size() - cmd.size() - 1);
            else if (cmd == ";t") { msgTop = line.right(line.size() - cmd.size() - 1); showMessage(msgTop); }
            else if (cmd == ";b") msgBottom = line.right(line.size() - cmd.size() - 1);
            else if (list.size() >= 4)
            {
                bool ok;
                FileDescription fd;
                fd.fileName = list[0].replace('\\', '/');
                fd.fileSize = list.at(1).toInt(&ok);

                if (!ok)
                {
                    qDebug() << "Parsing error" << fd.fileName << list.at(1);
                    continue;
                }
                //fd.dateTime = QDateTime::fromString(list.at(2) + " " + list.at(3), "");

                overallDownloadSize += fd.fileSize;
                QFile file(rusceneryDir + fd.fileName);
                if (file.exists() && file.size() == fd.fileSize) overallDownloadedBytes += fd.fileSize;
                else iFileList.append(fd);
            }
        }
    }
}

void Widget::on_vf_downloaded()
{
    if (networkReply->error())
    {
        qDebug() << QDateTime::currentDateTime() << networkReply->errorString() << networkReply->url();
        abort_install(AbortWithError, tr("%1 url:%2").arg(networkReply->errorString()).arg(networkReply->url().toString()));
        return;
    }

    if (networkReply)
    {
        qDebug() << QDateTime::currentDateTime() << "Closing networkReply" << networkReply->url();
        networkReply->deleteLater();
        networkReply = 0;
    }

    if (version != "" && version != QApplication::applicationVersion()) setUpdate(url);
    ui.progressBar_Overall->setValue(qCeil(overallDownloadedBytes*100.0/overallDownloadSize));
    qDebug() << QDateTime::currentDateTime() << "Calculating download size" << overallDownloadedBytes << overallDownloadSize;

    overallDownloadedBytesOnTimer = overallDownloadedBytes;

    if (iFileList.size() > 0)
    {
        QString size = QString::number((qreal)(overallDownloadSize - overallDownloadedBytes)/(1024*1024), 'f', 2);

        int ret = QMessageBox::question(this, QApplication::applicationName(),
                  tr("It's need to download %1 MiB of files.\n Do you want to continue?").arg(size),
                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (ret == QMessageBox::Yes) start_download();
        else abort_install(AbortByUser);
    }
    else abort_install(Finished);
}



//---------------------- Download Library Files ------------------------------
void Widget::start_download()
{
    showMessage(msg);

    ui.label_Download->setText(tr("Downloading %1").arg(iFileList.first().fileName));

    file = new QFile(rusceneryDir + iFileList.first().fileName);

    if ((file->exists() && !file->remove()) ||
        !QDir().mkpath(QFileInfo(file->fileName()).path()) ||
        !file->open(QIODevice::WriteOnly))
    {
        abort_install(AbortWithError, tr("%1 fileName:%2").arg(file->errorString()).arg(file->fileName()));
        return;
    }

    currentDownloadSize = iFileList.first().fileSize;
    currentDownloadedBytes = 0;
    ui.progressBar_Current->setValue(0);

    networkReply = networkAccessManager.get(QNetworkRequest(QUrl(uurl + iFileList.first().fileName, QUrl::TolerantMode)));
    qDebug() << QDateTime::currentDateTime() << "Starting networkRequest" << networkReply->request().url();

    connect(networkReply, SIGNAL(readyRead()), this, SLOT(on_download_readyRead()));
    connect(networkReply, SIGNAL(finished()), this, SLOT(on_download_finished()));
    connect(networkReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(on_download_progress(qint64,qint64)));
}

void Widget::on_download_readyRead()
{
    quint32 bytes = networkReply->bytesAvailable();

    if (bytes != file->write(networkReply->readAll()))
    {
        abort_install(AbortWithError, tr("%1 fileName:%2").arg(file->errorString().arg(file->fileName())));
    }
    else
    {
        currentDownloadedBytes += bytes;
        overallDownloadedBytes += bytes;

        if (currentDownloadedBytes > currentDownloadSize)
        {
            abort_install(AbortWithError, tr("Invalid file size %1 url:%2").arg(file->fileName()).arg(networkReply->url().toString()));
        }
    }
}

void Widget::on_download_finished()
{
    if (networkReply->error())
    {
        abort_install(AbortWithError, tr("%1 url:%2").arg(networkReply->errorString()).arg(networkReply->url().toString()));
        return;
    }
    else if (file->size() != iFileList.first().fileSize)
    {
        abort_install(AbortWithError, tr("Invalid file size %1 url:%2").arg(file->fileName()).arg(networkReply->url().toString()));
        return;
    }

    qDebug() << QDateTime::currentDateTime() << "Closing networkReply" << networkReply->url();
    networkReply->deleteLater();
    networkReply = 0;

    qDebug() << QDateTime::currentDateTime() << "Closing file" << file->fileName();
    file->close();
    delete file;
    file = 0;

    iFileList.removeFirst();

    if (iFileList.size() > 0) start_download();
    else
    {
        qDebug() << QDateTime::currentDateTime() << tr("All files downloaded");
        showMessage(msgBottom);
        abort_install(Finished);
    }
}


void Widget::on_download_progress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal == -1) bytesTotal = currentDownloadSize;
    if (bytesTotal == 0) ui.progressBar_Current->setValue(0);
    else
    {
        ui.progressBar_Current->setValue(qCeil(bytesReceived*100.0/bytesTotal));
        ui.progressBar_Current->setFormat(QString("%1 MiB / %2 MiB - %p%")
                                          .arg(QString::number((qreal)bytesReceived/(1024*1024),'f', 2))
                                          .arg(QString::number((qreal)bytesTotal/(1024*1024),'f', 2)) );
    }

    if (overallDownloadSize > 0)
    {
        ui.progressBar_Overall->setValue(qCeil(overallDownloadedBytes*100.0/overallDownloadSize));
        ui.progressBar_Overall->setFormat(QString("%1 MiB / %2 MiB - %p%")
                                          .arg(QString::number((qreal)overallDownloadedBytes/(1024*1024),'f', 2))
                                          .arg(QString::number((qreal)overallDownloadSize/(1024*1024),'f', 2)) );
    }
}

void Widget::on_timer_timeout()
{
    ++timerCounter;

    if (timerCounter >= SPEED_ESTIMATION_TIME)
    {
        timerCounter = 0;
        currentDownloadSpeed = qFloor(((overallDownloadedBytes - overallDownloadedBytesOnTimer))/(SPEED_ESTIMATION_TIME*KILOBYTE));
        overallDownloadedBytesOnTimer = overallDownloadedBytes;
    }

    if (currentDownloadSpeed > 0)
    {
        QTime ETA;
        ETA = QTime(0,0,0,0).addSecs(qCeil((overallDownloadSize - overallDownloadedBytes)/(currentDownloadSpeed*KILOBYTE)));

        ui.label_ETA->setText(QString("%1 MiB / %2 MiB - %3 KiBps ETA: %4")
                              .arg(qFloor(overallDownloadedBytes/MEGABYTE))
                              .arg(qFloor(overallDownloadSize/MEGABYTE))
                              .arg(currentDownloadSpeed)
                              .arg(ETA.toString("hh.mm.ss")));
    }
}

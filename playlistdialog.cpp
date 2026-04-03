#include "playlistdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QUrl>
#include <QDebug>

PlaylistDialog::PlaylistDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("播放列表");
    resize(600, 450);

    auto *mainLayout = new QVBoxLayout(this);

    m_listWidget = new QListWidget(this);
    mainLayout->addWidget(m_listWidget);

    auto *btnLayout = new QHBoxLayout();
    QPushButton *addFileBtn = new QPushButton("添加文件", this);
    QPushButton *addFolderBtn = new QPushButton("添加目录", this);
    addFileBtn->setFixedHeight(35);
    addFolderBtn->setFixedHeight(35);
    btnLayout->addWidget(addFileBtn);
    btnLayout->addWidget(addFolderBtn);
    mainLayout->addLayout(btnLayout);

    connect(addFileBtn, &QPushButton::clicked, this, &PlaylistDialog::addFiles);
    connect(addFolderBtn, &QPushButton::clicked, this, &PlaylistDialog::addFolder);
    connect(m_listWidget, &QListWidget::itemClicked, this, &PlaylistDialog::onItemClicked);
}

void PlaylistDialog::setPlaylist(const QList<QUrl>& playlist, int currentIndex)
{
    m_currentPlaylist = playlist;
    m_listWidget->clear();

    for (int i = 0; i < playlist.size(); ++i) {
        const QUrl &url = playlist.at(i);
        QString fileName = url.fileName();
        // 去除后缀
        if (fileName.lastIndexOf('.') > 0)
            fileName = fileName.left(fileName.lastIndexOf('.'));

        QString displayText = QString("%1. %2").arg(i+1, 2).arg(fileName);
        QListWidgetItem *item = new QListWidgetItem(displayText);
        m_listWidget->addItem(item);
    }

    if (currentIndex >= 0 && currentIndex < m_listWidget->count()) {
        m_listWidget->setCurrentRow(currentIndex);
    }
}
void PlaylistDialog::addFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "添加视频文件", QDir::homePath(),
                                                      "视频文件 (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.webm)");

    if (files.isEmpty()) return;

    for (const QString &f : files) {
        m_currentPlaylist.append(QUrl::fromLocalFile(f));
    }
    setPlaylist(m_currentPlaylist);
    emit playlistUpdated(m_currentPlaylist);
}

void PlaylistDialog::addFolder()
{
    QString folder = QFileDialog::getExistingDirectory(this, "添加目录", QDir::homePath());
    if (folder.isEmpty()) return;

    QDir dir(folder);
    QStringList filters = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv", "*.flv", "*.webm"};
    QFileInfoList list = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &info : list) {
        m_currentPlaylist.append(QUrl::fromLocalFile(info.absoluteFilePath()));
    }
    setPlaylist(m_currentPlaylist);
    emit playlistUpdated(m_currentPlaylist);
}

void PlaylistDialog::onItemClicked(QListWidgetItem *item)
{
    int index = m_listWidget->row(item);
    if (index >= 0) {
        emit playRequested(index);
        close();
    }
}

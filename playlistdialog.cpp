#include "playlistdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <QLabel>
#include <QMediaPlayer>

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
        if (fileName.lastIndexOf('.') > 0) {
            fileName = fileName.left(fileName.lastIndexOf('.'));
        }

        QWidget *itemWidget = new QWidget();
        QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(5, 2, 5, 2);
        itemLayout->setSpacing(10);

        // 显示文本的标签
        QString text = QString("%1. %2").arg(i+1, 2).arg(fileName);
        QLabel *label = new QLabel(text);

        // 时长标签
        QLabel *durationLabel = new QLabel("获取中...");
        durationLabel->setFixedWidth(80);
        durationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        durationLabel->setProperty("index", i);
        loadDurationForFile(i, url);

        // 删除按钮
        QPushButton *deleteBtn = new QPushButton("✕");
        deleteBtn->setFixedSize(24, 24);
        deleteBtn->setProperty("index", i);

        itemLayout->addWidget(label, 1);
        itemLayout->addWidget(durationLabel);
        itemLayout->addWidget(deleteBtn);

        QListWidgetItem *listItem = new QListWidgetItem();
        m_listWidget->addItem(listItem);
        m_listWidget->setItemWidget(listItem, itemWidget);

        // 连接删除按钮的信号
        connect(deleteBtn, &QPushButton::clicked, this, &PlaylistDialog::onDeleteClicked);
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

void PlaylistDialog::onDeleteClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int index = btn->property("index").toInt();

    // 从列表中删除
    if (index >= 0 && index < m_currentPlaylist.size()) {
        m_currentPlaylist.removeAt(index);

        // 更新显示
        setPlaylist(m_currentPlaylist);
        emit playlistUpdated(m_currentPlaylist);
    }
}

void PlaylistDialog::loadDurationForFile(int index, const QUrl &url)
{
    QMediaPlayer *player = new QMediaPlayer(this);
    player->setSource(url);
    m_durationLoaders[index] = player;

    connect(player, &QMediaPlayer::durationChanged, this, [this, index, player](qint64 duration) {
        if (duration > 0) {
            onDurationReady(index, duration);
        }
    });
    connect(player, &QMediaPlayer::errorOccurred, this, [this, index](QMediaPlayer::Error error) {
        Q_UNUSED(error);
        onDurationReady(index, 0);
    });
}

void PlaylistDialog::onDurationReady(int index, qint64 duration)
{
    if (index < 0 || index >= m_listWidget->count())
        return;

    m_durations[index] = duration;

    QListWidgetItem *item = m_listWidget->item(index);
    if (item) {
        QWidget *widget = m_listWidget->itemWidget(item);
        if (widget) {
            // 需要找到正确的标签 第二个QLabel
            QList<QLabel*> labels = widget->findChildren<QLabel*>();
            if (labels.size() >= 2) {
                QLabel *targetLabel = labels[1];  // 第二个标签是时长标签
                if (duration > 0) {
                    targetLabel->setText(formatTime(duration));
                } else {
                    targetLabel->setText("00:00");
                }
            }
        }
    }

    QMediaPlayer *player = m_durationLoaders[index];
    player->stop();
    player->deleteLater();
    m_durationLoaders.remove(index);

}

QString PlaylistDialog::formatTime(qint64 ms) const
{
    if (ms < 0) ms = 0;

    int totalSeconds = static_cast<int>(ms / 1000);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
        .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    } else {
        return QString("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
}


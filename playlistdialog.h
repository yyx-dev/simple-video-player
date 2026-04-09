#pragma once

#include <QDialog>
#include <QList>
#include <QUrl>
#include <QListWidget>
#include <QMediaPlayer>

class PlaylistDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PlaylistDialog(QWidget *parent = nullptr);

    void setPlaylist(const QList<QUrl>& playlist, int currentIndex = -1);
    void addFiles();
    void addFolder();

signals:
    void playlistUpdated(const QList<QUrl>& newPlaylist);
    void playRequested(int index);

private slots:
    void onItemClicked(QListWidgetItem *item);
    void onDeleteClicked();

private:
    void updatePlaylistFromWidget();
    void loadDurationForFile(int index, const QUrl &url);
    void onDurationReady(int index, qint64 duration);
    QString formatTime(qint64 ms) const;

    QListWidget *m_listWidget;
    QList<QUrl> m_currentPlaylist;

    QMap<int, QMediaPlayer*> m_durationLoaders;  // 存储每个文件的临时播放器
    QMap<int, qint64> m_durations;               // 存储每个文件的时长
};

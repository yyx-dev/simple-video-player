#pragma once

#include <QDialog>
#include <QList>
#include <QUrl>
#include <QListWidget>

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

private:
    QListWidget *m_listWidget;
    QList<QUrl> m_currentPlaylist;
};

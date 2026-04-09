#pragma once

#include "playlistdialog.h"
#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QSettings>

class QSlider;
class QLabel;
class QPushButton;
class PlaylistDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void playSingleFile(const QUrl &url);

private slots:
    void openPlaylistDialog();           // 打开播放列表对话框
    void togglePlayPause();
    void skipBackward();
    void skipForward();
    void playPrevious();                 // 上一集
    void playNext();                     // 下一集
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void seek(int position);
    void setVolume(int volume);
    void setPlaybackRate(int sliderValue);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayRequested(int index);
    void toggleFullscreen();

private:
    void playCurrentFile(int position = 0); // 播放当前播放列表中的文件
    QString formatTime(qint64 ms) const;    // 格式化时间 mm:ss
    void autoAdjustHeightToVideo();
    QIcon coloredIcon(const QIcon& icon);
    void savePlaylist();          // 保存
    void loadPlaylist();          // 加载

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QVideoWidget *m_videoWidget = nullptr;

    // 界面控件
    QSlider *m_progressSlider = nullptr;
    QLabel *m_timeLabel = nullptr;          // 当前时长 / 总时长
    QPushButton *m_playlistBtn = nullptr;
    QPushButton *m_fullscreenBtn = nullptr;  // 全屏按钮
    QPushButton *m_prevBtn = nullptr;
    QPushButton *m_skipBackBtn = nullptr;
    QPushButton *m_playPauseBtn = nullptr;
    QPushButton *m_skipForwardBtn = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel  *m_volumeLabel = nullptr;
    QSlider *m_speedSlider = nullptr;
    QLabel *m_speedLabel = nullptr;

    bool m_isFullscreen = false;

    // 播放列表
    QList<QUrl> m_playlist;
    int m_currentIndex = -1;

    int m_lastIndex;
    int m_lastPosition = 0;

    int m_normalSpeed = 100;
    QTimer *m_longPressTimer = nullptr;
    bool m_isLongPress = false;

    QSettings *m_settings = nullptr;

    PlaylistDialog *m_playlistDialog = nullptr;
};

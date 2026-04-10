#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QStyle>
#include <QUrl>
#include <QDir>
#include <QMediaMetaData>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QMimeData>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSeekForward)));
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    qApp->installEventFilter(this);

    qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    qputenv("QT_FFMPEG_HWACCEL", "1");
    qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "d3d11va,dxva2");

    // 1. 初始化播放器核心
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setAcceptDrops(true);
    m_videoWidget->installEventFilter(this);
    m_player->setVideoOutput(m_videoWidget);

    // 2. 界面布局
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // 上方大视频区域（可拉伸）
    mainLayout->addWidget(m_videoWidget, 1);

    // ==================== 中间：进度条 + 时长显示 ====================
    auto *progressLayout = new QHBoxLayout();
    m_progressSlider = new QSlider(Qt::Horizontal, this);
    m_progressSlider->setRange(0, 0);
    m_progressSlider->setTracking(true);   // 拖动时实时更新
    progressLayout->addWidget(m_progressSlider);

    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    progressLayout->addWidget(m_timeLabel);

    mainLayout->addLayout(progressLayout);

    // ==================== 下方控制栏 ====================

    auto *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(8);

    // 左侧：播放列表按钮
    m_playlistBtn = new QPushButton("播放列表", this);
    m_playlistBtn->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    m_playlistBtn->setFixedHeight(40);

    m_fullscreenBtn = new QPushButton(this);
    m_fullscreenBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton)));
    m_fullscreenBtn->setToolTip("全屏 (F)");
    m_fullscreenBtn->setFixedSize(40, 40);

    controlLayout->addWidget(m_playlistBtn);
    controlLayout->addWidget(m_fullscreenBtn);

    controlLayout->addStretch(1); // 中间左侧弹簧

    // 中间：上下集 左右跳 + 播放暂停（居中）
    m_prevBtn = new QPushButton(this);
    m_prevBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward)));
    m_prevBtn->setToolTip("上一集");
    m_nextBtn = new QPushButton(this);
    m_nextBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSkipForward)));
    m_nextBtn->setToolTip("下一集");
    m_skipBackBtn = new QPushButton(this);
    m_skipBackBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward)));
    m_skipBackBtn->setToolTip("后退 5 秒 (A)");
    m_skipBackBtn->setFixedSize(32, 32);
    m_skipForwardBtn = new QPushButton(this);
    m_skipForwardBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaSeekForward)));
    m_skipForwardBtn->setToolTip("前进 5 秒 (D)");
    m_skipForwardBtn->setFixedSize(32, 32);
    m_playPauseBtn = new QPushButton(this);
    m_playPauseBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaPlay)));
    m_skipForwardBtn->setToolTip("播放 (Space)");
    m_playPauseBtn->setFixedSize(40, 40);

    controlLayout->addSpacing(55);
    controlLayout->addWidget(m_prevBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_skipBackBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_playPauseBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_skipForwardBtn);
    controlLayout->addSpacing(10);
    controlLayout->addWidget(m_nextBtn);

    controlLayout->addStretch(1); // 中间右侧弹簧

    // 右侧：音量和倍速（垂直排列）
    auto *rightPanel = new QVBoxLayout();
    rightPanel->setSpacing(6);

    // 音量行
    auto *volLayout = new QHBoxLayout();
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(120);
    m_volumeLabel = new QLabel("100%", this);

    volLayout->addWidget(new QLabel("音量"));
    volLayout->addWidget(m_volumeSlider);
    volLayout->addWidget(m_volumeLabel);
    rightPanel->addLayout(volLayout);

    // 倍速行
    auto *speedLayout = new QHBoxLayout();
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(25, 300);   // 0.25x ~ 3.00x
    m_speedSlider->setValue(100);
    m_speedSlider->setTickInterval(25);
    m_speedSlider->setFixedWidth(120);
    m_speedLabel = new QLabel("1.00x", this);

    speedLayout->addWidget(new QLabel("倍速"));
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedLabel);
    rightPanel->addLayout(speedLayout);

    controlLayout->addLayout(rightPanel);

    mainLayout->addLayout(controlLayout);
    setCentralWidget(centralWidget);

    // 3. 信号槽连接
    connect(m_playlistBtn, &QPushButton::clicked, this, &MainWindow::openPlaylistDialog);
    connect(m_playPauseBtn, &QPushButton::clicked, this, &MainWindow::togglePlayPause);
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &MainWindow::toggleFullscreen);
    connect(m_skipBackBtn, &QPushButton::clicked, this, &MainWindow::skipBackward);
    connect(m_skipForwardBtn, &QPushButton::clicked, this, &MainWindow::skipForward);
    connect(m_prevBtn, &QPushButton::clicked, this, &MainWindow::playPrevious);
    connect(m_nextBtn, &QPushButton::clicked, this, &MainWindow::playNext);

    connect(m_progressSlider, &QSlider::sliderReleased, this, [this] { seek(m_progressSlider->value()); });
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::updatePosition);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::updateDuration);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, &MainWindow::autoAdjustHeightToVideo);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::setVolume);
    connect(m_speedSlider, &QSlider::valueChanged, this, &MainWindow::setPlaybackRate);

    // 长按倍速播放计时器
    m_longPressTimer = new QTimer(this);
    m_longPressTimer->setSingleShot(true);  // 单次触发
    connect(m_longPressTimer, &QTimer::timeout, this, [this]() {
            m_isLongPress = true;
            m_normalSpeed = m_speedSlider->value();
            setPlaybackRate(200);
        });

    // 初始音量
    m_audioOutput->setVolume(1);

    // 持久化设置
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config.ini";
    m_settings = new QSettings(configPath, QSettings::IniFormat, this);
}

void MainWindow::playSingleFile(const QUrl &url)
{
    m_playlist.clear();
    m_playlist.append(url);
    m_currentIndex = 0;
    playCurrentFile();
}

void MainWindow::openPlaylistDialog()
{
    if (!m_playlistDialog) {
        m_playlistDialog = new PlaylistDialog(this);

        connect(m_playlistDialog, &PlaylistDialog::playlistUpdated, this, [this](const QList<QUrl>& list) {
                m_playlist = list;
                if (m_currentIndex >= m_playlist.size())
                    m_currentIndex = m_playlist.size() - 1;
            });

        connect(m_playlistDialog, &PlaylistDialog::playRequested, this, &MainWindow::onPlayRequested);
    }

    m_playlistDialog->setPlaylist(m_playlist, m_currentIndex);
    m_playlistDialog->show();
}

void MainWindow::onPlayRequested(int index)
{
    if (index < 0 || index >= m_playlist.size())
        return;

    m_currentIndex = index;
    playCurrentFile();
}

void MainWindow::playCurrentFile()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size())
        return;

    const QUrl &url = m_playlist.at(m_currentIndex);
    m_player->setSource(url);
    m_player->play();

    // 更新窗口标题为当前文件名
    QString fileName = url.fileName();
    setWindowTitle(fileName.left(fileName.lastIndexOf('.')));
}

void MainWindow::togglePlayPause()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}

void MainWindow::skipBackward()
{
    qint64 pos = m_player->position() - 5000;
    if (pos < 0) pos = 0;
    m_player->setPosition(pos);
}

void MainWindow::skipForward()
{
    qint64 pos = m_player->position() + 5000;
    if (pos > m_player->duration()) pos = m_player->duration();
    m_player->setPosition(pos);
}

void MainWindow::playPrevious()
{
    if (m_playlist.isEmpty()) return;
    if (--m_currentIndex < 0) m_currentIndex = m_playlist.size() - 1;
    playCurrentFile();
}

void MainWindow::playNext()
{
    if (m_playlist.isEmpty()) return;
    if (++m_currentIndex >= m_playlist.size()) m_currentIndex = 0;
    playCurrentFile();
}

void MainWindow::updatePosition(qint64 position)
{
    // 防止拖动进度条时产生循环
    if (!m_progressSlider->isSliderDown()) {
        m_progressSlider->setValue(static_cast<int>(position));
    }

    QString current = formatTime(position);
    QString total = formatTime(m_player->duration());
    m_timeLabel->setText(current + " / " + total);
}

void MainWindow::updateDuration(qint64 duration)
{
    m_progressSlider->setMaximum(static_cast<int>(duration));
    QString current = formatTime(m_player->position());
    QString total = formatTime(duration);
    m_timeLabel->setText(current + " / " + total);
}

void MainWindow::seek(int position)
{
    m_player->setPosition(position);
}

void MainWindow::setVolume(int volume)
{
    QString text;
    if (volume == 100) text = QString("%1%").arg(volume);
    else if (volume >= 10) text = QString(" %1% ").arg(volume);
    else text = QString("  %1%  ").arg(volume);

    m_volumeLabel->setText(text);
    m_audioOutput->setVolume(volume / 100.0);
}

void MainWindow::setPlaybackRate(int value)
{
    const QList<int> speedSteps = {25, 50, 75, 100, 125, 150, 175, 200, 250, 300};

    int closest = 100;
    int minDiff = INT_MAX;

    for (int step : speedSteps) {
        int diff = std::abs(value - step);
        if (diff < minDiff) {
            minDiff = diff;
            closest = step;
        }
    }

    m_speedSlider->setValue(closest);
    double rate = closest / 100.0;
    m_player->setPlaybackRate(rate);
    m_speedLabel->setText(QString::number(rate, 'f', 2) + "x");

}

void MainWindow::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        m_playPauseBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaPause)));
        m_playPauseBtn->setToolTip("暂停 (Space)");
    } else {
        m_playPauseBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_MediaPlay)));
        m_playPauseBtn->setToolTip("播放 (Space)");
    }
}

void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        m_player->pause();
        m_player->setPosition(m_player->duration() - 100);
    }
    else if (status == QMediaPlayer::LoadedMedia) {
        // 媒体加载完成，检查是否有待跳转的位置
        if (m_pendingPosition > 0 && m_player->isSeekable()) {
            if (m_pendingPosition < m_player->duration()) {
                m_player->setPosition(m_pendingPosition);
            } else {
                m_player->setPosition(m_player->duration() - 100);
            }
            m_pendingPosition = -1;
        }
    }
}

void MainWindow::toggleFullscreen()
{
    if (m_isFullscreen) {
        // 退出全屏
        showNormal();
        m_isFullscreen = false;
        m_fullscreenBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton))); // SP_DesktopIcon
        m_fullscreenBtn->setToolTip("全屏 (F)");

        // 显示控制栏（如果需要）
        if (centralWidget()->layout()) {
            // 显示所有控制控件
            m_progressSlider->show();
            m_timeLabel->show();
            m_playlistBtn->show();
            m_fullscreenBtn->show();
            m_prevBtn->show();
            m_skipBackBtn->show();
            m_playPauseBtn->show();
            m_skipForwardBtn->show();
            m_nextBtn->show();
            m_volumeSlider->show();
            m_volumeLabel->show();
            m_speedSlider->show();
            m_speedLabel->show();
        }
    } else {
        // 进入全屏
        showFullScreen();
        m_isFullscreen = true;
        m_fullscreenBtn->setIcon(coloredIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton)));
        m_fullscreenBtn->setToolTip("退出全屏 (F)");
    }
}

QString MainWindow::formatTime(qint64 ms) const
{
    if (ms < 0) ms = 0;

    int totalSeconds = static_cast<int>(ms / 1000);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    if (hours > 0) {
        // 超过1小时：显示 HH:MM:SS
        return QString("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    } else {
        // 小于1小时：显示 MM:SS
        return QString("%1:%2")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }
}

void MainWindow::autoAdjustHeightToVideo()
{
    if (!m_videoWidget)
        return;

    QSize videoSize = m_player->metaData().value(QMediaMetaData::Resolution).toSize();
    if (videoSize.isEmpty() || videoSize.width() <= 0 || videoSize.height() <= 0)
        return;

    double aspectRatio = static_cast<double>(videoSize.width()) / videoSize.height();

    int videoWidgetWidth = m_videoWidget->width();
    int desiredVideoHeight = static_cast<int>(videoWidgetWidth / aspectRatio);

    // 根据 videoWidget 当前宽度计算最合适的高度
    resize(width(), desiredVideoHeight + 90); // 84
}

QIcon MainWindow::coloredIcon(const QIcon& icon)
{
    QPalette pal = QApplication::palette();
    QColor accentColor = pal.color(QPalette::Accent);

    QPixmap pixmap = icon.pixmap(48, 48);
    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), accentColor);
    painter.end();

    return QIcon(pixmap);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space: // 空格键：播放/暂停
        togglePlayPause();
        break;
    case Qt::Key_F:     // F键：全屏
    case Qt::Key_F11:
        toggleFullscreen();
        break;
    case Qt::Key_Escape: // ESC键：退出全屏
        if (m_isFullscreen) {
            toggleFullscreen();
        }
        break;
    case Qt::Key_D:
    case Qt::Key_Right:
        m_longPressTimer->start(200);
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    if (event->key() == Qt::Key_A || event->key() == Qt::Key_D
     || event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        m_longPressTimer->stop();

        if (m_isLongPress) {
            setPlaybackRate(m_normalSpeed);  // 恢复原来的倍速
            m_isLongPress = false;
        } else {
            if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left ) {
                skipBackward();
            } else if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right) {
                skipForward();
            }
        }
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls())
        return;

    QList<QUrl> urls = mimeData->urls();
    if (urls.isEmpty())
        return;

    bool hasNewFile = false;

    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();

            if (suffix == "mp4" || suffix == "mkv" || suffix == "avi" || suffix == "mov" ||
                suffix == "wmv" || suffix == "flv" || suffix == "webm" || suffix == "ts" ||
                suffix == "m4v" || suffix == "rmvb") {

                m_playlist.append(url);
                hasNewFile = true;
            }
        }
    }

    if (hasNewFile) {
        // 如果当前没有播放内容，则自动播放第一个新文件
        if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
            m_currentIndex = m_playlist.size() - urls.size();
        }
        playCurrentFile();

        // 刷新播放列表对话框（如果打开的话）
        if (m_playlistDialog) {
            m_playlistDialog->setPlaylist(m_playlist, m_currentIndex);
        }
    }

    event->acceptProposedAction();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::DragEnter) {
        dragEnterEvent(static_cast<QDragEnterEvent*>(event));
        return true;
    }
    else if (event->type() == QEvent::Drop) {
        dropEvent(static_cast<QDropEvent*>(event));
        return true;
    }
    else if (event->type() == QEvent::KeyPress) {
        keyPressEvent(static_cast<QKeyEvent*>(event));
        return true;
    }
    else if (event->type() == QEvent::KeyRelease) {
        keyReleaseEvent(static_cast<QKeyEvent*>(event));
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

QPair<QString, qint64> MainWindow::loadLastPlaying()
{
    // 读取保存的播放列表
    m_settings->beginGroup("Playing");
    m_lastPlayingUrl = m_settings->value("lastPlayingUrl").toString();
    m_lastPlayingPosition = m_settings->value("lastPlayingPosition", 0).toLongLong();
    m_settings->endGroup();
    return {m_lastPlayingUrl, m_lastPlayingPosition};
}

void MainWindow::playLastPlaying()
{
    if (!m_lastPlayingUrl.isEmpty()) {
        // 检查文件是否存在
        QUrl url = QUrl(m_lastPlayingUrl);
        if (QFile::exists(url.toLocalFile())) {
            m_playlist.clear();
            m_playlist.append(url);
            m_currentIndex = 0;

            // 计算恢复位置
            qint64 restorePosition = m_lastPlayingPosition - 5000;
            if (restorePosition < 0) {
                restorePosition = 0;
            }

            m_pendingPosition = restorePosition;
            playCurrentFile();
        }
    }
}


void MainWindow::saveLastPlaying()
{
    // 保存播放列表
    m_settings->beginGroup("Playing");
    m_settings->remove("");
    m_settings->setValue("lastPlayingUrl", m_player->source().toString());
    m_settings->setValue("lastPlayingPosition", m_player->position());
    m_settings->endGroup();
    m_settings->sync();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveLastPlaying();
    QMainWindow::closeEvent(event);
}
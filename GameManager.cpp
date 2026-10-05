#include "GameManager.h"
#include <QDebug>

// ---------------- ObstacleModel implementation ----------------
ObstacleModel::ObstacleModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ObstacleModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.count();
}

QVariant ObstacleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return {};
    const Obstacle &o = m_items.at(index.row());
    switch (role) {
    case XRole: return o.x;
    case YRole: return o.y;
    case WRole: return o.w;
    case HRole: return o.h;
    case SourceRole: return o.source;
    case IdRole: return o.id;
    case ScoredRole: return o.scored;
    default: return {};
    }
}

QHash<int, QByteArray> ObstacleModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[XRole] = "x";
    roles[YRole] = "y";
    roles[WRole] = "w";
    roles[HRole] = "h";
    roles[SourceRole] = "source";
    roles[IdRole] = "id";
    roles[ScoredRole] = "scored";
    return roles;
}

void ObstacleModel::addObstacle(const Obstacle &o)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(o);
    endInsertRows();
}

void ObstacleModel::updateObstacle(int row, const Obstacle &o)
{
    if (row < 0 || row >= m_items.size()) return;
    m_items[row] = o;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, { XRole, YRole, WRole, HRole, SourceRole, ScoredRole });
}

void ObstacleModel::removeFirst()
{
    if (m_items.isEmpty()) return;
    beginRemoveRows(QModelIndex(), 0, 0);
    m_items.removeFirst();
    endRemoveRows();
}

void ObstacleModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void ObstacleModel::setItems(const QList<Obstacle> &list)
{
    beginResetModel();
    m_items = list;
    endResetModel();
}

// ---------------- GameManager implementation ----------------
GameManager::GameManager(QObject *parent)
    : QObject(parent)
{
    // load high score
    QSettings s;
    m_highScore = s.value("highScore", 0).toInt();

    // tick timer ~60Hz
    connect(&m_timer, &QTimer::timeout, this, &GameManager::onTick);
    m_timer.setInterval(16);
}

void GameManager::startGame()
{
    m_score = 0;
    m_playerY = GROUND_Y;
    m_playerVy = 0.0;
    m_obstacles.clear();
    m_nextObstacleId = 1;
    m_timer.start();
    m_elapsed.restart();
    m_gameState = 1;
    emit gameStateChanged();
    emit scoreChanged();
    emit playerYChanged();

    emit playBgm();
}

void GameManager::pauseGame()
{
    if (m_gameState != 1) return;
    m_timer.stop();
    m_gameState = 2;
    emit gameStateChanged();
    emit pauseBgm();
}

void GameManager::resumeGame()
{
    if (m_gameState != 2) return;
    m_elapsed.restart();
    m_timer.start();
    m_gameState = 1;
    emit gameStateChanged();
    emit playBgm();
}

void GameManager::finishedGame()
{
    m_gameState = 0;
    m_timer.stop();
    emit gameStateChanged();
    emit pauseBgm();
}

void GameManager::triggerJump()
{
    if (m_gameState != 1) return;
    if (m_playerY >= GROUND_Y - 1.0) {
        m_playerVy = JUMP_VELOCITY;
        emit playJumpSound();
    }
}

void GameManager::setDifficulty(int d)
{
    if (d < 1) d = 1;
    if (d > 3) d = 3;
    if (m_difficulty == d) return;
    m_difficulty = d;
    emit difficultyChanged();
}

void GameManager::onTick()
{
    qint64 ms = m_elapsed.restart();
    qreal dt = ms / 1000.0;
    if (dt <= 0) dt = 0.016;
    advance(dt);
}

void GameManager::advance(qreal dt)
{
    // integrate player
    m_playerVy += GRAVITY * dt;
    m_playerY += m_playerVy * dt;
    if (m_playerY > GROUND_Y) {
        m_playerY = GROUND_Y;
        m_playerVy = 0.0;
    }
    emit playerYChanged();

    // move obstacles
    double speed = 220.0 + (m_difficulty - 1) * 80.0; // px/s
    for (int i = 0; i < m_obstacles.rowCount(); ++i) {
        QModelIndex idx = m_obstacles.index(i);
        // read current
        Obstacle o = m_obstacles.items().at(i);
        o.x -= speed * dt;
        // scored check
        if (!o.scored && o.x + o.w < 80) {
            o.scored = true;
            m_score += 10 * m_difficulty;
            emit scoreChanged();
        }
        m_obstacles.updateObstacle(i, o);
    }

    // remove off-screen obstacles from front
    while (m_obstacles.rowCount() && m_obstacles.items().first().x + m_obstacles.items().first().w < -200) {
        m_obstacles.removeFirst();
    }

    // spawn
    spawnObstacleIfNeeded();

    // collision
    for (const Obstacle &o : m_obstacles.items()) {
        if (checkCollisionWith(o)) {
            // game over
            m_timer.stop();
            m_gameState = 3;
            emit gameStateChanged();
            emit playHitSound();
            if (m_score > m_highScore) {
                m_highScore = m_score;
                QSettings s;
                s.setValue("highScore", m_highScore);
                emit highScoreChanged();
            }
            return;
        }
    }
}

void GameManager::spawnObstacleIfNeeded()
{
    // 如果模型为空或最后一个障碍到达某个位置则生成一个新障碍
    bool need = false;
    if (m_obstacles.rowCount() == 0) need = true;
    else {
        const Obstacle &last = m_obstacles.items().last();
        // 控制前后两个障碍物之间的最小 X 距离
        if (last.x < 460 + QRandomGenerator::global()->bounded(200))
            need = true;
    }
    if (!need) return;

    Obstacle o;
    o.id = m_nextObstacleId++;
    o.x = 900; // 右侧起点
    o.scored = false;

    // 根据难度决定是否允许刷出飞鸟 (简单难度只出仙人掌，普通和困难加入飞鸟)
    int maxType = (m_difficulty >= 2) ? 7 : 5;
    int type = QRandomGenerator::global()->bounded(maxType + 1);

    if (type >= 0 && type <= 5) {
        // --- 仙人掌系列 (地面障碍) ---
        if (type == 0) {
            o.w = 17; o.h = 32;
            o.source = QStringLiteral("./images/cactus1.png");
        } else if (type == 1) {
            o.w = 29; o.h = 32;
            o.source = QStringLiteral("./images/cactus2.png");
        } else if (type == 2){
            o.w = 42; o.h = 32;
            o.source = QStringLiteral("./images/cactus3.png");
        } else if (type == 3){
            o.w = 25; o.h = 44;
            o.source = QStringLiteral("./images/cactus4.png");
        } else if (type == 4){
            o.w = 40; o.h = 44;
            o.source = QStringLiteral("./images/cactus5.png");
        } else{
            o.w = 60; o.h = 44;
            o.source = QStringLiteral("./images/cactus6.png");
        }
        o.y = 260 - o.h; // 地面对齐
    } else {
        // --- 飞鸟系列 (空中障碍) ---
        o.w = 35; o.h = 30;
        o.source = QStringLiteral("./images/pterosaurs1.png");

        // 飞鸟悬空，给不同的高度让游戏更有挑战
        // 地面是 260，恐龙高度 40
        int birdHeightVariant = QRandomGenerator::global()->bounded(2);
        if (birdHeightVariant == 0) {
            o.y = 260 - o.h - 30; // 较低空（跳跃可通过）
        } else {
            o.y = 260 - o.h - 65; // 较高空
        }
    }

    qDebug() << "Spawn obstacle id=" << o.id << "src=" << o.source << "x=" << o.x << "y=" << o.y;
    m_obstacles.addObstacle(o);
}

bool GameManager::checkCollisionWith(const Obstacle &o)
{
    // simple AABB-ish check with small padding for nicer gameplay
    double dinoX = 80;
    double dinoW = 40;
    double dinoY = m_playerY;
    double dinoH = 40;

    double padX = 8;
    double padY = 8;

    bool overlapX = (dinoX + padX) < (o.x + o.w) && (dinoX + dinoW - padX) > o.x;
    bool overlapY = (dinoY + padY) < (o.y + o.h) && (dinoY + dinoH - padY) > o.y;
    return overlapX && overlapY;
}